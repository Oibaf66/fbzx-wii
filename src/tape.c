/* 
 * Copyright (C) 2012 Fabio Olimpieri
 * Copyright 2003-2009 (C) Raster Software Vigo (Sergio Costas)
 * This file is part of FBZX Wii
 *
 * FBZX Wii is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * FBZX Wii is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "z80free/Z80free.h"
#include "computer.h"
#include "emulator.h"
#include "menus.h"
#include "tape.h"
#include "tape_browser.h"

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

int elcontador=0;
int eltstado=0;
char elbit=0;

/* reads a tape file and updates the readed bit */

inline void tape_read(FILE *fichero, int tstados) {

	if(ordenador.tape_stop)
	{
		if ((ordenador.turbo_state != 0)&&(ordenador.turbo==1))
			{
			update_frequency(0); //set machine frequency
			jump_frames=0;
			ordenador.turbo_state = 0;
			ordenador.precision=ordenador.precision_old;
			}
	return;
	}
	
	//Auto ultra fast mode
	if ((ordenador.turbo_state != 1)&&(ordenador.turbo==1))
	{
		if (ordenador.tape_file_type==TAP_TAP) update_frequency(12000000);
		else update_frequency(10500000);
		jump_frames=7;
		ordenador.precision_old=ordenador.precision;
		ordenador.precision =0;
		ordenador.turbo_state = 1;
	}
	
	if(ordenador.tape_file_type == TAP_TAP)
		tape_read_tap(fichero,tstados);
	else
		tape_read_tzx(fichero,tstados);
	
}

// manages TAP files in REAL_MODE

inline void tape_read_tap (FILE * fichero, int tstados) {

	static unsigned char value, value2;
	int retval;
		
	if (fichero == NULL)
		{
		sprintf (ordenador.osd_text, "No tape selected");
		ordenador.osd_time = 100;
		ordenador.tape_stop=1; //Stop the tape
		ordenador.tape_stop_fast=1; //Stop the tape
		return;
		}

	if (!ordenador.tape_stop) {
		if (ordenador.tape_current_mode == TAP_TRASH) {		// initialize a new block						
			ordenador.tape_position=ftell(fichero);
			retval=fread (&value, 1, 1, fichero);
			retval=fread (&value2, 1, 1, fichero);	// read block longitude
			if (feof (fichero)) {
				rewind_tape(fichero,1);
				ordenador.tape_current_mode = TAP_TRASH;
				return;
			}
			ordenador.tape_byte_counter = ((unsigned int) value) + 256 * ((unsigned int) value2);
			retval=fread (&(ordenador.tape_byte), 1, 1, fichero);
			printf("TAP: Flag_byte_norm: %X en %ld\n",ordenador.tape_byte,ftell(fichero));
			ordenador.tape_bit = 0x80;
			ordenador.tape_current_mode = TAP_GUIDE;
			ordenador.tape_counter0 = 2168;
			ordenador.tape_counter1 = 2168;
			if (!(0x80 & ordenador.tape_byte))
				ordenador.tape_counter_rep = 3228;	// 4 seconds
			else
				ordenador.tape_counter_rep = 1614;	// 2 seconds
		}

		// if there's a pulse still being reproduce, just reproduce it

		if (ordenador.tape_counter0) 	{
			if (ordenador.tape_counter0 > tstados) {
				ordenador.tape_readed = 0;	// return 0
				ordenador.tape_counter0 -= tstados;	// decrement counter;
				return;
			} else {
				tstados -= ordenador.tape_counter0;
				ordenador.tape_counter0 = 0;
			}
		}

		ordenador.tape_readed = 1;	// return 1
		if (ordenador.tape_counter1) {
			if (ordenador.tape_counter1 > tstados) {
				ordenador.tape_counter1 -= tstados;	// decrement counter;
				return;
			} else {
				tstados -= ordenador.tape_counter1;
				ordenador.tape_counter1 = 0;
				ordenador.tape_readed = 0;	// return 0
			}
		}

		// pulse ended
		
		switch (ordenador.tape_current_mode) {
		case TAP_GUIDE:	// guide tone
			if (ordenador.tape_counter_rep) {	// still into guide tone
				ordenador.tape_counter_rep--;
				ordenador.tape_counter0 = 2168 - tstados;
				ordenador.tape_counter1 = 2168;	// new pulse
				return;
			} else {	// guide tone ended. send sync tone
				ordenador.tape_counter0 = 667 - tstados;
				ordenador.tape_counter1 = 735;	// new pulse
				ordenador.tape_current_mode = TAP_DATA;	// data mode
				ordenador.tape_bit = 0x80;	// from bit0 to bit7
				return;
			}
			break;
		case TAP_DATA:	// data
			if (ordenador.tape_byte & ordenador.tape_bit) {	// next bit is 1
				ordenador.tape_counter0 = 1710 - tstados;
				ordenador.tape_counter1 = 1710;
			} else {
				ordenador.tape_counter0 = 855 - tstados;
				ordenador.tape_counter1 = 855;
			}
			ordenador.tape_bit = ((ordenador.tape_bit >> 1) & 0x7F);	// from bit0 to bit7
			if (!ordenador.tape_bit) {
				ordenador.tape_byte_counter--;
				if (!ordenador.tape_byte_counter) {	// ended the block
					ordenador.tape_current_mode = TAP_PAUSE;	// pause
					ordenador.tape_readed = 0;
					ordenador.tape_counter_rep = 3500000;	// 1 seconds
					return;
				}
				ordenador.tape_bit = 0x80;
				retval=fread (&(ordenador.tape_byte), 1, 1, fichero);	// read next byte
				if (feof (fichero)) {
					rewind_tape(fichero,0);
					ordenador.tape_current_mode = TAP_STOP;	// stop tape
					return;
				}
			}
			break;
		case TAP_PAUSE:	// pause
			ordenador.tape_readed = 0;
			if (ordenador.tape_counter_rep > tstados) {
				ordenador.tape_counter_rep -= tstados;
			} else {
				ordenador.tape_counter_rep = 0;
				ordenador.tape_current_mode = TAP_TRASH;	// read new block
			}
			return;
			break;
		case TAP_STOP:
			ordenador.tape_current_mode = TAP_TRASH;	// initialize
			ordenador.tape_stop = 1;	// pause it
			ordenador.tape_stop_fast = 1;	// pause it
			break;
		default:
			break;
		}
	}
}

// manages TZX files

inline void tape_read_tzx (FILE * fichero, int tstados) {

	static unsigned char value, value2,value3,value4,done;
	static unsigned int bucle,bucle2, byte_position;
	int retval;
	char block_jump[2];
	
	if (fichero == NULL)
		{
		sprintf (ordenador.osd_text, "No tape selected");
		ordenador.osd_time = 100;
		ordenador.tape_stop=1; //Stop the tape
		ordenador.tape_stop_fast = 1;	// pause it
		return;
		}
	
	if (ordenador.tape_stop)
		return;

	if (ordenador.tape_current_mode == TAP_TRASH) {		// initialize a new block
		done = 0;
		do {
			ordenador.tape_position=ftell(fichero);
			retval=fread(&value,1,1,fichero); // read block ID
			printf("TZX:ID_normal: %X en %ld\n",value,ftell(fichero));
			if(feof(fichero))
				done = 1;
			else
				switch(value) {
				case 0x10: // classic tape block
					done = 1;
					bucle = 0;
					ordenador.tape_current_bit = 0;
					ordenador.tape_bit0_level = 855;
					ordenador.tape_bit1_level = 1710;
					ordenador.tape_bits_at_end = 8;
					ordenador.tape_block_level = 2168;
					ordenador.tape_sync_level0 = 667;
					ordenador.tape_sync_level1 = 735;
					
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // pause length
					ordenador.tape_pause_at_end = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					if(ordenador.tape_pause_at_end==0)
						ordenador.tape_pause_at_end=10; // to avoid problems
					ordenador.tape_pause_at_end *= 3500;
					retval=fread (&value, 1, 1, fichero);
					
					retval=fread (&value2, 1, 1, fichero);	// read block longitude
					if (feof (fichero)) {
						rewind_tape(fichero,1);
						ordenador.tape_current_bit = 0;
						ordenador.tape_current_mode = TAP_TRASH;						
						for(bucle=0;bucle<10;bucle++)
							retval=fread(&value3,1,1,fichero); // jump over the header
						return;
					}
					ordenador.tape_byte_counter = ((unsigned int) value) + 256 * ((unsigned int) value2);
					retval=fread (&(ordenador.tape_byte), 1, 1, fichero);
					ordenador.tape_bit = 0x80;
					ordenador.tape_current_mode = TAP_GUIDE;
					ordenador.tape_counter0 = 2168;
					ordenador.tape_counter1 = 2168;
					if (!(0x80 & ordenador.tape_byte))
						ordenador.tape_counter_rep = 3228;	// 4 seconds
					else
						ordenador.tape_counter_rep = 1614;	// 2 seconds					
					break;

				case 0x11: // turbo tape block					
					done = 1;
					bucle = 0;
					
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_block_level = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_sync_level0 = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_sync_level1 = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_bit0_level = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_bit1_level = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_counter_rep = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					ordenador.tape_counter_rep /=2;
					retval=fread(&value2,1,1,fichero);
					ordenador.tape_bits_at_end = value2;					
					
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // pause length
					ordenador.tape_pause_at_end = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					
			
					if(ordenador.tape_pause_at_end==0)
						ordenador.tape_pause_at_end=10; // to avoid problems
					ordenador.tape_pause_at_end *= 3500;
					
					retval=fread (&value, 1, 1, fichero);
					retval=fread (&value2, 1, 1, fichero);
					retval=fread (&value3, 1, 1, fichero);	// read block longitude
					ordenador.tape_byte_counter = ((unsigned int) value) + 256 * ((unsigned int) value2) + 65536* ((unsigned int) value3);
					
					if (feof (fichero)) {
						rewind_tape(fichero,1);
						ordenador.tape_current_bit = 0;
						ordenador.tape_current_mode = TAP_TRASH;
						return;
					}
					
					retval=fread (&(ordenador.tape_byte), 1, 1, fichero);
					ordenador.tape_bit = 0x80;
					ordenador.tape_current_mode = TAP_GUIDE;
					ordenador.tape_counter0 = ordenador.tape_block_level;
					ordenador.tape_counter1 = ordenador.tape_block_level;
					ordenador.tape_current_bit = 0;
					break;
					
				case 0x12: // pure tone
					done = 1;
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // length of pulse in T-states
					ordenador.tape_block_level = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					ordenador.tape_counter0 = ordenador.tape_block_level;
					ordenador.tape_counter1 = 0;
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // number of pulses
					ordenador.tape_counter_rep = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					if(ordenador.tape_counter_rep == 0)
						done = 0;
					ordenador.tape_current_mode = TZX_PURE_TONE;
					break;

				case 0x13: // multiple pulses
					done=1;
					retval=fread(&value2,1,1,fichero); // number of pulses
					ordenador.tape_counter_rep = ((unsigned int) value2);
					if(ordenador.tape_counter_rep == 0)
						done = 0;
					else {
						retval=fread(&value2,1,1,fichero);
						retval=fread(&value3,1,1,fichero); // length of pulse in T-states
						ordenador.tape_counter0 = ((unsigned int) value2) + 256 * ((unsigned int) value3);
						ordenador.tape_counter1 = 0;
						ordenador.tape_current_mode = TZX_SEQ_PULSES;
					}
					break;
				
				case 0x14: // turbo tape block					
					done = 1;
					bucle = 0;
					ordenador.tape_current_bit = 0;
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_bit0_level = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_bit1_level = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					retval=fread(&value2,1,1,fichero);
					ordenador.tape_bits_at_end = value2;
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // pause length
					ordenador.tape_pause_at_end = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					/*if(ordenador.tape_pause_at_end==0)
						ordenador.tape_pause_at_end=10;*/ // to avoid problems
					ordenador.tape_pause_at_end *= 3500;
					retval=fread (&value, 1, 1, fichero);					
					retval=fread (&value2, 1, 1, fichero);
					retval=fread (&value3,1,1,fichero);	// read block longitude
					if (feof (fichero)) {
						rewind_tape(fichero,1);
						ordenador.tape_current_bit = 0;
						ordenador.tape_current_mode = TAP_TRASH;						
						return;
					}
					ordenador.tape_byte_counter = ((unsigned int) value) + 256 * ((unsigned int) value2) + 65536*((unsigned int)value3);					
					ordenador.tape_bit = 0x80;
					retval=fread (&(ordenador.tape_byte), 1, 1, fichero);	// read next byte
					if((ordenador.tape_byte_counter==1)&&(ordenador.tape_bits_at_end!=8)) { // last byte
						for(bucle=ordenador.tape_bits_at_end;bucle<8;bucle++) {
							ordenador.tape_byte=((ordenador.tape_byte>>1)&0x7F);
							ordenador.tape_bit = ((ordenador.tape_bit>>1)&0x7F);
						}
					}
					ordenador.tape_current_mode = TAP_DATA;
					ordenador.tape_counter0 = 0;
					ordenador.tape_counter1 = 0;
					ordenador.tape_counter_rep = 0;					
					break;

				case 0x20: // pause
					done = 1;
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // pause length
					ordenador.tape_counter_rep = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					ordenador.tape_readed = 0;
					ordenador.tape_counter0 = 0;				
					ordenador.tape_counter1 = 0; // 1ms of inversed pulse					
					if(ordenador.tape_counter_rep == 0) {
						ordenador.tape_current_mode = TAP_PAUSE2;	// initialize
						ordenador.tape_byte_counter = 31500;
						break;
					}
					ordenador.tape_counter_rep *= 3500;
					ordenador.tape_current_mode = TAP_PAUSE;									
					break;
					
				case 0x21: // group start
					retval=fread(&value2,1,1,fichero);
					bucle2=(unsigned int) value2;
					for(bucle=0;bucle<bucle2;bucle++)
						retval=fread(&value2,1,1,fichero);
					break;
					
				case 0x22: // group end
					break;
					
				case 0x23: // jump to block
					retval=fread(block_jump,1,2,fichero);
					jump_to_block(fichero, (int) block_jump[0] + 256*((int) block_jump[1]));
				break;
				
				case 0x24: // loop start
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					ordenador.tape_loop_counter = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					ordenador.tape_loop_pos = ftell(fichero);
					break;
				
				case 0x25: // loop end
					if(ordenador.tape_loop_counter) {
						ordenador.tape_loop_counter--;
						fseek(fichero,ordenador.tape_loop_pos,SEEK_SET);
					}
					break;
				
				case 0x28: // select block
					byte_position=ftell(fichero);
					retval=select_block(fichero);
					if (retval)
					{
						fseek(fichero, byte_position, SEEK_SET);
						retval=fread(&value2,1,1,fichero);
						retval=fread(&value3,1,1,fichero); 
						bucle2 = ((unsigned int) value2) + 256 * ((unsigned int) value3);
						for(bucle=0;bucle<bucle2;bucle++)
							retval=fread(&value3,1,1,fichero);
					}	
					break;
				
				case 0x2A: // pause if 48K
					for(bucle=0;bucle<4;bucle++)
						retval=fread(&value,1,1,fichero);
					if(ordenador.mode128k==0) {
						ordenador.tape_stop = 1;
						ordenador.tape_stop_fast = 1;	// pause it
						return;
					}
					break;
					
				case 0x30: // text description
					retval=fread(&value2,1,1,fichero); // length
					for(bucle=0;bucle<((unsigned int)value2);bucle++)
						retval=fread(&value3,1,1,fichero);
					break;
					
				case 0x31: // show text
					retval=fread(&value2,1,1,fichero);
					if (value2 < 11) ordenador.osd_time=value2*50; else ordenador.osd_time=500; //max 10 sec	
					retval=fread(&value2,1,1,fichero); // length
					for(bucle=0;bucle<((unsigned int)value2);bucle++)
					{
					retval=fread(&value3,1,1,fichero);
					if (bucle<199) ordenador.osd_text[bucle] = value3;
					}
					if (bucle>199) ordenador.osd_text[199]=0; else ordenador.osd_text[bucle]=0;
					break;
					
				case 0x32: // archive info
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero); // pause length
					bucle2 = ((unsigned int) value2) + 256 * ((unsigned int) value3);
					for(bucle=0;bucle<bucle2;bucle++)
						retval=fread(&value3,1,1,fichero);
					break;
				
				case 0x33: // hardware info
					retval=fread(&value2,1,1,fichero);					
					bucle2 = ((unsigned int) value2) *3;
					for(bucle=0;bucle<bucle2;bucle++)
						retval=fread(&value3,1,1,fichero);
					break;
					
				case 0x34: // emulation info					
					for(bucle=0;bucle<8;bucle++)
						retval=fread(&value3,1,1,fichero);
					break;
					
				case 0x35: // custom info					
					for(bucle=0;bucle<16;bucle++)
						retval=fread(&value3,1,1,fichero);
					retval=fread(&value,1,1,fichero);
					retval=fread(&value2,1,1,fichero);
					retval=fread(&value3,1,1,fichero);
					retval=fread(&value4,1,1,fichero);
					bucle2 = ((unsigned int) value) + 256 * ((unsigned int) value2) + 65536*((unsigned int) value3) + 16777216*((unsigned int) value4);
					for(bucle=0;bucle<bucle2;bucle++)
						retval=fread(&value3,1,1,fichero);
					break;
					
				default: // not supported
					sprintf(ordenador.osd_text,"Unsupported TZX. Contact FBZX autor. %X",value);
					ordenador.osd_time=200;
					rewind_tape(fichero,1); // rewind and stop
					ordenador.tape_current_mode = TAP_TRASH;
					return;					
					break;
				}
		} while (!done);			
	}

	if (feof (fichero)) {
		rewind_tape(fichero,1);
		ordenador.tape_current_bit = 0;
		ordenador.tape_current_mode = TAP_TRASH;		
		return;
	}
	
	// if there's a pulse still being reproduced, just play it

	if (ordenador.tape_counter0) 	{
		if (ordenador.tape_counter0 > tstados) {
			ordenador.tape_readed = ordenador.tape_current_bit;	// return current
			ordenador.tape_counter0 -= tstados;	// decrement counter;
			return;
		} else {
			tstados -= ordenador.tape_counter0;
			ordenador.tape_counter0 = 0;
		}
	}
	
	ordenador.tape_readed = 1 - ordenador.tape_current_bit;	// return oposite current
	if (ordenador.tape_counter1) {		
		if (ordenador.tape_counter1 > tstados) {
			ordenador.tape_counter1 -= tstados;	// decrement counter;
			return;
		} else {
			tstados -= ordenador.tape_counter1;
			ordenador.tape_counter1 = 0;
			ordenador.tape_readed = ordenador.tape_current_bit;	// return current
		}
	}

	// pulse ended

	switch (ordenador.tape_current_mode) {
	case TAP_FINAL_BIT:
		ordenador.tape_current_mode = TAP_TRASH;
		ordenador.next_block= NOBLOCK;
		break;
	case TAP_GUIDE:	// guide tone
		if (ordenador.tape_counter_rep) {	// still into guide tone
			ordenador.tape_counter_rep--;
			ordenador.tape_counter0 = ordenador.tape_block_level - tstados;
			ordenador.tape_counter1 = ordenador.tape_block_level;	// new pulse
			return;
		} else {	// guide tone ended. send sync tone			
			ordenador.tape_counter0 = ordenador.tape_sync_level0 - tstados;
			ordenador.tape_counter1 = ordenador.tape_sync_level0;	// new pulse
			ordenador.tape_current_mode = TAP_DATA;	// data mode
			ordenador.tape_bit = 0x80;	// from bit0 to bit7
			if((ordenador.tape_byte_counter==1)&&(ordenador.tape_bits_at_end!=8)) { // last byte
				for(bucle=ordenador.tape_bits_at_end;bucle<8;bucle++) {
					ordenador.tape_byte=((ordenador.tape_byte>>1)&0x7F);
					ordenador.tape_bit = ((ordenador.tape_bit>>1)&0x7F);
				}
			}
			return;
		}
		break;
	case TAP_DATA:	// data
		if (ordenador.tape_byte & ordenador.tape_bit) {	// next bit is 1
			ordenador.tape_counter0 = ordenador.tape_bit1_level - tstados;
			ordenador.tape_counter1 = ordenador.tape_bit1_level;
		} else {
			ordenador.tape_counter0 = ordenador.tape_bit0_level - tstados;
			ordenador.tape_counter1 = ordenador.tape_bit0_level;
		}
		ordenador.tape_bit = ((ordenador.tape_bit >> 1) & 0x7F);	// from bit0 to bit7
		if (!ordenador.tape_bit) {			
			ordenador.tape_byte_counter--;
			if (!ordenador.tape_byte_counter) {	// ended the block
				if(ordenador.tape_pause_at_end) {					
					ordenador.tape_current_mode = TAP_PAUSE3;	// pause					
					ordenador.tape_counter_rep = ordenador.tape_pause_at_end;
				} else
					ordenador.tape_current_mode = TAP_FINAL_BIT;					
				return;
			}
			ordenador.tape_bit = 0x80;
			retval=fread (&(ordenador.tape_byte), 1, 1, fichero);	// read next byte
			if (feof (fichero)) {
				rewind_tape (fichero,0);
				ordenador.tape_current_bit = 0;				
				ordenador.tape_current_mode = TAP_STOP;	// stop tape
				return;
			}
			if((ordenador.tape_byte_counter==1)&&(ordenador.tape_bits_at_end!=8)) { // last byte
				for(bucle=ordenador.tape_bits_at_end;bucle<8;bucle++) {
					ordenador.tape_byte=((ordenador.tape_byte>>1)&0x7F);
					ordenador.tape_bit = ((ordenador.tape_bit>>1)&0x7F);
				}
			}
		}
		break;
		
	case TAP_PAUSE3: // one pulse of 1 ms for ending the bit
		ordenador.tape_counter0 = 3500; // 1 ms
		ordenador.tape_counter1 = 0;		
		ordenador.tape_current_mode = TAP_PAUSE;
		break;
		
	case TAP_PAUSE:	// pause
		ordenador.tape_readed = 0;
		ordenador.tape_current_bit = 0;
		if (ordenador.tape_counter_rep > tstados) {
			ordenador.tape_counter_rep -= tstados;
		} else {
			ordenador.tape_counter_rep = 0;
			ordenador.tape_current_mode = TAP_TRASH;	// read new block
			ordenador.next_block= NOBLOCK;
		}
		break;
	case TAP_PAUSE2:	// pause and stop
		ordenador.tape_readed = 0;
		ordenador.tape_current_bit = 0;
		if (ordenador.tape_counter_rep > tstados) {
			ordenador.tape_counter_rep -= tstados;
		} else {
			ordenador.tape_counter_rep = 0;
			ordenador.tape_current_mode = TAP_TRASH;	// read new block
			ordenador.next_block= NOBLOCK;
			ordenador.tape_stop = 1;
			ordenador.tape_stop_fast = 1;	// pause it
		}
		break;
	case TZX_PURE_TONE:
		ordenador.tape_counter_rep--;
		ordenador.tape_current_bit = 1 - ordenador.tape_current_bit; // invert current bit
		if (ordenador.tape_counter_rep) {	// still into guide tone			
			ordenador.tape_counter0 = ordenador.tape_block_level - tstados;
			ordenador.tape_counter1 = 0; // new pulse			
		} else
			ordenador.tape_current_mode = TAP_TRASH;	// next ID
			ordenador.next_block= NOBLOCK;
		break;
	case TZX_SEQ_PULSES:
		ordenador.tape_current_bit = 1 - ordenador.tape_current_bit; // invert current bit
		ordenador.tape_counter_rep--;
		if(ordenador.tape_counter_rep) {
			retval=fread(&value2,1,1,fichero);
			retval=fread(&value3,1,1,fichero); // length of pulse in T-states
			ordenador.tape_counter0 = ((unsigned int) value2) + 256 * ((unsigned int) value3);
			ordenador.tape_counter1 = 0;
		} else
			ordenador.tape_current_mode = TAP_TRASH;	// next ID		
			ordenador.next_block= NOBLOCK;		
		break;
			
	case TAP_STOP:
		ordenador.tape_current_bit = 0;
		ordenador.tape_current_mode = TAP_TRASH;	// initialize
		ordenador.next_block= NOBLOCK;
		ordenador.tape_stop = 1;	// pause it
		ordenador.tape_stop_fast = 1;	// pause it
		break;
	default:
		break;
	}
}

void rewind_tape(FILE *fichero,unsigned char pause) {

	int thebucle;
	unsigned char value;
	int retval;
	
	rewind (fichero);
	if(ordenador.tape_file_type==TAP_TZX)
		for(thebucle=0;thebucle<10;thebucle++)
			retval=fread(&value,1,1,ordenador.tap_file); // jump over the header
	ordenador.tape_position=ftell(fichero);
	ordenador.next_block= NOBLOCK;		
	ordenador.tape_stop=pause;
	ordenador.tape_stop_fast=pause;
	if (pause) ordenador.tape_start_countdwn=0; //Stop tape play countdown
}

unsigned char file_empty(FILE *fichero) {
	
	long position,position2;
	
	position=ftell(fichero);
	fseek(fichero,0,SEEK_END); // set the pointer at end
	position2=ftell(fichero);
	fseek(fichero,position,SEEK_SET); // return the pointer to the original place
	if(position2==0)
		return 1; // empty file
	else
		return 0;	
}

void save_file(FILE *fichero) {

	long position;
	unsigned char xor,salir_s;
	byte dato;
	int longitud;
			
	position=ftell(fichero); // store current position
	fseek(fichero,0,SEEK_END); // put position at end
	xor=0;
	
	longitud=(int)(procesador.Rm.wr.DE);
	longitud+=2;
	
	dato=(byte)(longitud%256);
	fprintf(fichero,"%c",dato);
	dato=(byte)(longitud/256);
	fprintf(fichero,"%c",dato); // file length

	fprintf(fichero,"%c",procesador.Rm.br.A); // flag

	xor^=procesador.Rm.br.A;

	salir_s = 0;
	do {
		if (procesador.Rm.wr.DE == 0)
			salir_s = 2;
		if (!salir_s) {
			dato=Z80free_Rd_fake(procesador.Rm.wr.IX); // read data
			fprintf(fichero,"%c",dato);
			xor^=dato;
			procesador.Rm.wr.IX++;			
			procesador.Rm.wr.DE--;			
		}
	} while (!salir_s);
	fprintf(fichero,"%c",xor);
	procesador.Rm.wr.IX++;
	procesador.Rm.wr.IX++;
	fseek(fichero,position,SEEK_SET); // put position at end
	ordenador.tape_position = position;
	
	if(ordenador.tape_fast_load==1) //if we want fast load, we assume we want fast save too
		ordenador.other_ret = 1;	// next instruction must be RET
	
	return;
}

unsigned int min(unsigned int x,unsigned int y)
{
	if (x<y) return x; else return y;
}

void fastload_block_tap (FILE * fichero) {

	/*Frome Fuse On exit:
   *  A = calculated parity byte if parity checked, else 0 (CHECKME)
   *  F : if parity checked, all flags are modified
   *      else carry only is modified (FIXME)
   *  B = 0xB0 (success) or 0x00 (failure)
   *  C = 0x01 (confirmed), 0x21, 0xFE or 0xDE (CHECKME)
   * DE : decremented by number of bytes loaded or verified
   *  H = calculated parity byte or undefined
   *  L = last byte read, or 1 if none
   * IX : incremented by number of bytes loaded or verified
   * A' = unchanged on error + no flag byte, else 0x01
   * F' = 0x01      on error + no flag byte, else 0x45
   *  R = no point in altering it :-)
   * Other registers unchanged.
   */
   
	unsigned int longitud, longitud_block, bucle, number_bytes;
	unsigned char value[65536], empty, parity;	
	int retval;

	//ordenador.other_ret = 1;	// next instruction must be RET
	procesador.PC=0x5e2;

	if (!(procesador.Ra.br.F & F_C)) { // if Carry=0, is VERIFY, so return OK
		procesador.Rm.br.F |= F_C;	 // verify OK
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		return;
	}
	
	procesador.Rm.br.B=0;
	procesador.Rm.br.L=0x01;

	empty=file_empty(fichero);

	
	if ((fichero == NULL)||(empty)) {
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		if(empty)
			sprintf (ordenador.osd_text, "Tape file empty");
		else
			sprintf (ordenador.osd_text, "No tape selected");
		ordenador.osd_time = 100;
		return;
	}
	ordenador.tape_position=ftell(fichero);
	retval=fread (value, 1, 2, fichero);	// read length of current block
		if (feof (fichero))	{			// end of file?
			sprintf (ordenador.osd_text, "Rewind tape");			
			ordenador.osd_time = 100;
			rewind_tape(fichero, 1);
			return;
			}
			
	longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
	longitud_block=longitud;
		
	retval=fread (value, 1,1, fichero); //Flag Byte
	if (retval!=1)
		{
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		printf("TAP: Read file error\n");
		return;
		}

	longitud--;
	printf("TAP: Flag_byte_fast: %X en %ld\n",value[0],ftell(fichero));
	
	if (value[0] != procesador.Ra.br.A) // different flag
		{
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		retval=fread (value, 1,longitud, fichero); //read the remaining bytes
		printf("TAP: Flag byte error, expected %X\n", procesador.Ra.br.A);
		return;
		}
			
	parity=(byte) value[0];	
		
	if ((longitud-1)!=procesador.Rm.wr.DE) 
		{
		printf("TAP: length block error\n");
		printf("TAP: expected by system %d\n", procesador.Rm.wr.DE);
		printf("TAP: expected by file %d\n", longitud-1);
		}
	
	number_bytes = min(procesador.Rm.wr.DE,longitud-1);
	
	retval=fread (value, 1,number_bytes+1, fichero); //read also checksum byte
	if (retval!=(number_bytes+1))
		{
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		printf("TAP: Read file error\n");
		return;
		}

	for(bucle=0;bucle<number_bytes; bucle++) 
		{	
			Z80free_Wr_fake (procesador.Rm.wr.IX, (byte) value[bucle]);	// store the byte
			procesador.Rm.wr.IX++;
			procesador.Rm.wr.DE--;
			longitud--;
			parity^=(byte) value[bucle];
		}

	parity^=(byte) value[number_bytes]; // checksum
	
	if (parity) printf("TAP: Parity error\n");
	longitud--;
	if (longitud>0) retval=fread (value, 1,longitud, fichero); //read the remaining bytes
	
	procesador.Rm.br.C=0x01;
	procesador.Rm.br.H=parity;
	procesador.Rm.br.L=value[number_bytes-1];
	
	if (procesador.Rm.wr.DE==0) //OK
	{
		procesador.Rm.br.A=parity;
		//CP 01
		Z80free_doArithmetic(&procesador,procesador.Rm.br.A,0x01,0,1);
		Z80free_adjustFlags(&procesador,0x01);
		procesador.Rm.br.B=0xB0;
		procesador.Ra.br.A=0x01;
		procesador.Ra.br.F=0x45;
		procesador.Rm.br.F |= F_C;	// Load OK
	}
	else // Load error
	{
		procesador.Rm.br.B=0;
		procesador.Rm.br.A=0;
		procesador.Rm.br.F &= (~F_C);	// Load error
	}
	
	if (ordenador.pause_instant_load) 
		{
			if ((ordenador.turbo==0)||(longitud_block==6914)) ordenador.pause_fastload_countdwn=2000/20+1; //tap pause for screen and norma turbo
			else ordenador.pause_fastload_countdwn=1000/20+1;
		}
		
	return;
}

void fastload_block_tzx (FILE * fichero) {

	/* From Fuse - On exit:
   *  A = calculated parity byte if parity checked, else 0 (CHECKME)
   *  F : if parity checked, all flags are modified
   *      else carry only is modified (FIXME)
   *  B = 0xB0 (success) or 0x00 (failure)
   *  C = 0x01 (confirmed), 0x21, 0xFE or 0xDE (CHECKME)
   * DE : decremented by number of bytes loaded or verified
   *  H = calculated parity byte or undefined
   *  L = last byte read, or 1 if none
   * IX : incremented by number of bytes loaded or verified
   * A' = unchanged on error + no flag byte, else 0x01
   * F' = 0x01      on error + no flag byte, else 0x45
   *  R = no point in altering it :-)
   * Other registers unchanged.
   */
   
	unsigned int longitud, len, bucle, number_bytes, byte_position, byte_position2, retorno;
	unsigned char value[65536], empty, blockid, parity, pause[2],flag_byte;	
	int retval;
	char block_jump[2];
	
	longitud =0;
	pause[0]=pause[1]=0;

	empty=file_empty(fichero);

	if ((fichero == NULL)||(empty)) {
		procesador.PC=0x5e2;
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		if(empty)
			sprintf (ordenador.osd_text, "Tape file empty");
		else
			sprintf (ordenador.osd_text, "No tape selected");
		ordenador.osd_time = 100;
		return;
	}
		
	do	{
		retorno=0;
		ordenador.tape_position=ftell(fichero);
		byte_position=ftell(fichero);
		retval=fread (&blockid, 1, 1, fichero); //Read id block
		if (feof (fichero)) // end of file?
			{
			sprintf (ordenador.osd_text, "Rewind tape");			
			ordenador.osd_time = 100;
			rewind_tape(fichero, 1);
			return;
			}
		printf("TZX: ID_fast: %X en %ld\n",blockid,ftell(fichero));
		switch(blockid) {
				case 0x10: // classic tape block
				retval=fread (pause, 1, 2, fichero); //pause lenght
				retval=fread (value, 1, 2, fichero);	// read length of current block
				byte_position2=ftell(fichero);
				retval=fread (&flag_byte, 1, 1, fichero);
				if ((retval!=1))
					{			
					procesador.Rm.br.F &= (~F_C);	// Load error
					procesador.Rm.wr.IX += procesador.Rm.wr.DE;
					procesador.Rm.wr.DE = 0;
					printf("TZX: Read file error\n");
					return;
					}	
				longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					
					switch(flag_byte)
					{
					case 0x00: //header
						retval=fread (value, 1, 18, fichero);
						if (retval!=18) {procesador.Rm.br.F &= (~F_C);return;}
						switch(value[0]	) //Type
							{
							case 0x00:
								ordenador.next_block=PROG;
							break;
							case 0X01:
							case 0X02:
							case 0X03:
								ordenador.next_block=DATA;
							break;
							default: //Custom header
								ordenador.next_block=NOBLOCK;
							break;
							}
					break;
					case 0xFF: //data
						ordenador.next_block=NOBLOCK;
					break;
					default: //Custom data
							ordenador.next_block=NOBLOCK;
					break;
					}
				retorno=1;		
				fseek(fichero, byte_position2, SEEK_SET);	
				break;	
					case 0x11: // turbo 
					retorno=2;
				break;
					
				case 0x12: // pure tone
					retorno=2;
				break;

				case 0x13: // multiple pulses
					retorno=2;
				break;
				
				case 0x14: // turbo tape block					
					retorno=2;	
				break;

				case 0x20: // pause
					retval=fread(value,1,2,fichero);
					if (retval!=2) {procesador.Rm.br.F &= (~F_C);return;} 
					if (!value[0]&&!value[1]) {retorno=2;} //stop the tape
				break;
					
				case 0x21: // group start
					retval=fread(value,1,1,fichero);
					if (retval!=1) {procesador.Rm.br.F &= (~F_C);return;} 
					len = (unsigned int) value[0];
					retval=fread(value,1,len,fichero);	
				break;
					
				case 0x22: // group end
				break;
				
				case 0x23: // jump to block
					retval=fread(block_jump,1,2,fichero);
					if (retval!=2) {procesador.Rm.br.F &= (~F_C);return;} 
					jump_to_block(fichero, (int) block_jump[0] + 256*((int) block_jump[1]));
				break;
				
				case 0x24: // loop start
					retorno=2;
				break;
				
				case 0x25: // loop end
					retorno=2;
				break;
				
				case 0x28: // select block
					byte_position2=ftell(fichero);
					retval=select_block(fichero);
					if (retval)
					{
						fseek(fichero, byte_position2, SEEK_SET);
						retval=fread(value,1,2,fichero);
						if (retval!=2) {procesador.Rm.br.F &= (~F_C);return;} 
						len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
						retval=fread(value,1,len,fichero);
					}
				break;
				
				case 0x2A: // pause if 48K
					ordenador.tape_stop_fast=1;
					fseek(fichero, byte_position, SEEK_SET);
					return;
				break;
					
				case 0x30: // text description
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {procesador.Rm.br.F &= (~F_C);return;} 
					len = (unsigned int) value[0] ;
					retval=fread(value,1,len,fichero);
				break;
					
				case 0x31: // show text
					retval=fread(value,1,1,fichero);
					if (retval!=1) {procesador.Rm.br.F &= (~F_C);return;} 
					if (value[0] < 11) ordenador.osd_time=value[0]*50; else ordenador.osd_time=500;//max 10 sec	
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {procesador.Rm.br.F &= (~F_C);return;} 
					len = (unsigned int) value[0];
					for(bucle=0;bucle<len;bucle++)
					{
					retval=fread(value,1,1,fichero);
					if (retval!=1) {procesador.Rm.br.F &= (~F_C);return;} 
					if (bucle<199) ordenador.osd_text[bucle] = value[0];
					}
					if (bucle>199) ordenador.osd_text[199]=0; else ordenador.osd_text[bucle]=0;
				break;
					
				case 0x32: // archive info
					retval=fread(value,1,2,fichero); // length
					if (retval!=2) {procesador.Rm.br.F &= (~F_C);return;} 
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread(value,1,len,fichero);
				break;
				
				case 0x33: // hardware info
					retval=fread(value,1,1,fichero);
					if (retval!=1) {procesador.Rm.br.F &= (~F_C);return;} 
					len = (unsigned int) value[0] *3;
					retval=fread(value,1,len,fichero);
				break;
					
				case 0x34: // emulation info					
					retval=fread(value,1,8,fichero);
				break;
					
				case 0x35: // custom info					
					retval=fread(value,1,16,fichero);
					retval=fread(value,1,4,fichero);
					if (retval!=4) {procesador.Rm.br.F &= (~F_C);return;} 
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]) + 65536*((unsigned int) value[2]);// + 16777216*((unsigned int) value[3]);
					retval=fread(value,1,len,fichero);
				break;
					
				default: // not supported
					retorno=2;				
				break;
			}
		} while ((retorno==0)&&(!feof(fichero)));

		if (feof(fichero)) {retorno=2;}
		
		if (retorno==2)
		{
			fseek(fichero, byte_position, SEEK_SET);
			ordenador.tape_stop=0; //Start the tape
		return;
		}
		
		//Fast load routine
	
	procesador.PC=0x5e2;
	
	if (!(procesador.Ra.br.F & F_C)) { // if Carry=0, is VERIFY, so return OK
		procesador.Rm.br.F |= F_C;	 // verify OK
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		return;
	}
	
	procesador.Rm.br.B=0;
	procesador.Rm.br.L=0x01;	
		
	retval=fread (&flag_byte, 1,1, fichero); //Flag Byte
	if (retval!=1)
		{
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		printf("TZX: Read file error\n");
		return;
		}

	longitud--;
	printf("TZX: Flag_byte_fast: %X en %ld\n",flag_byte,ftell(fichero));
	
	if (flag_byte != procesador.Ra.br.A) // different flag
		{
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		retval=fread (value, 1,longitud, fichero); //read the remaining bytes
		printf("TZX Flag byte error, expected %X\n", procesador.Ra.br.A);
		return;
		}
			
	parity=(byte) flag_byte;	
		
	if ((longitud-1)!=procesador.Rm.wr.DE) 
		{
		printf("TZX: length block error\n");
		printf("TZX: expected by system %d\n", procesador.Rm.wr.DE);
		printf("TZX: expected by file %d\n", longitud-1);
		}
	
	number_bytes = min(procesador.Rm.wr.DE,longitud-1);
 
	retval=fread (value, 1,number_bytes+1, fichero); //read also checksum byte
	if (retval!=(number_bytes+1))
		{
		procesador.Rm.br.F &= (~F_C);	// Load error
		procesador.Rm.wr.IX += procesador.Rm.wr.DE;
		procesador.Rm.wr.DE = 0;
		printf("TZX: Read file error\n");
		return;
		}

	for(bucle=0;bucle<number_bytes; bucle++) 
		{	
			Z80free_Wr_fake (procesador.Rm.wr.IX, (byte) value[bucle]);	// store the byte
			procesador.Rm.wr.IX++;
			procesador.Rm.wr.DE--;
			longitud--;
			parity^=(byte) value[bucle];
		}
	
	parity^=(byte) value[number_bytes]; // checksum
	
	if (parity) printf("TZX: Parity error\n");
	longitud--;
	if (longitud>0) retval=fread (value, 1,longitud, fichero); //read the remaining bytes
	
	procesador.Rm.br.C=0x01;
	procesador.Rm.br.H=parity;
	procesador.Rm.br.L=value[number_bytes-1];
	
	if (procesador.Rm.wr.DE==0) //OK
	{
		procesador.Rm.br.A=parity;
		//CP 01
		Z80free_doArithmetic(&procesador,procesador.Rm.br.A,0x01,0,1);
		Z80free_adjustFlags(&procesador,0x01);
		procesador.Rm.br.B=0xB0;
		procesador.Ra.br.A=0x01;
		procesador.Ra.br.F=0x45;
		procesador.Rm.br.F |= F_C;	// Load OK
	}
	else // Load error
	{
		procesador.Rm.br.B=0;
		procesador.Rm.br.A=0;
		procesador.Rm.br.F &= (~F_C);	// Load error
	}
	
	byte_position=ftell(fichero);
	
	retval=fread (&blockid, 1, 1, fichero); //Read next id block

	if (!feof(fichero)) 
	{
		if (blockid==0x10) 
		{
		retval=fread (value, 1, 5, fichero); //read till flag byte
		if (retval==5)
			if ((value[4]!=0x0)&&(value[4]!=0xFF)) blockid=0x1; //custom data
			if ((value[4]==0x0)&&((value[2]+value[3]*256)!=0x13)) blockid=0x1; //custom data
			if ((value[4]==0xFF)&&(ordenador.next_block==NOBLOCK)) blockid=0x2; //standard data
			//printf("TZX: ID_fast2: %X en %d\n",blockid,byte_position+1);
			//printf("TZX: next block: %X \n",ordenador.next_block);
		}
		if (blockid!=0x10)
		{
			//Anticipate auto ultra fast mode
			if ((ordenador.turbo_state!= 1)&&(ordenador.turbo==1))
			{
			update_frequency(10500000);
			jump_frames=7;
			ordenador.turbo_state=4;
			}
		ordenador.tape_start_countdwn=((unsigned int)pause[0]+256*(unsigned int)pause[1])/20+1; //autoplay countdown
		if (ordenador.tape_start_countdwn<10) ordenador.tape_start_countdwn=10; //in case the pause is too short 
		}
		else if (ordenador.pause_instant_load) 
		{
			if (ordenador.turbo==0) ordenador.pause_fastload_countdwn=((unsigned int)pause[0]+256*(unsigned int)pause[1])/20; //tzx pause
			else ordenador.pause_fastload_countdwn=((unsigned int)pause[0]+256*(unsigned int)pause[1])/60;
		}
		
		fseek(fichero, byte_position, SEEK_SET);
	}
	return;

}
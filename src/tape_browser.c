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
#include "tape_browser.h"
#include "tape.h"
#include "menu_sdl.h"

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

struct browser *browser_list[MAX_BROWSER_ITEM+1];
struct tape_select *block_select_list[MAX_SELECT_ITEM+1];

void create_browser_tzx (FILE * fichero) {

	unsigned int longitud, len, bucle, byte_position, retorno, block_number;
	unsigned char value[65536], empty, blockid, pause[2], flag_byte;
	char block_jump[2];
	int retval, retval2;
	
	longitud =0;
	pause[0]=pause[1]=0;
	
	blockid=0;
	flag_byte=0;
	
	//Free the browser list
	for(bucle=0; ((browser_list[bucle]!=NULL)&&(bucle<MAX_BROWSER_ITEM)); bucle++)
	{
		free (browser_list[bucle]);
		browser_list[bucle]=NULL;
	}
	
	if (fichero == NULL) return;
	
	empty=file_empty(fichero);
	
	if (empty) return;
	
	rewind_tape(fichero,1);

	retorno=0;
	block_number=0;
	do	{
		byte_position=ftell(fichero);
		retval=fread (&blockid, 1, 1, fichero); //Read id block
		if (retval!=1) {retorno=1;break;}
		browser_list[block_number]=(struct browser *)malloc(sizeof(struct browser));
		browser_list[block_number]->position=byte_position;
		strcpy(browser_list[block_number]->info, "                         ");
		
		printf("TZX browser: %X en %d\n",blockid, byte_position+1);
		
		switch(blockid) {
				case 0x10: // classic tape block
				strcpy(browser_list[block_number]->block_type,"Standard Speed Data");
				retval=fread (pause, 1, 2, fichero); //pause lenght
				retval2=fread (value, 1, 2, fichero);	// read length of current block
				if ((retval!=2)||(retval2!=2)) {retorno=1;break;}
				longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
				retval=fread (&flag_byte, 1, 1, fichero);
				if (retval!=1) {retorno=1;break;}
				switch(flag_byte)
					{
					case 0x00: //header
						if (longitud!=19) 
						{
							sprintf(browser_list[block_number]->info,"Custom Data %d bytes", longitud);
							if (longitud>1)
							{
								retval=fread (value, 1, longitud-1, fichero);
								if (retval!=(longitud-1)) {retorno=1;break;}
							}
						}
						else
						{
						retval=fread (value, 1, 18, fichero);
						if (retval!=18) {retorno=1;break;}
						value[11]=0;
						for(bucle=1;bucle<11;bucle++)
							if (value[bucle]<' ') value[bucle]='?'; //not printable character
							
						switch(value[0]	) //Type
							{
							case 0x00: //PROG
								sprintf(browser_list[block_number]->info,"Program: %s", value+1);
							break;
							case 0X01: //DATA
								sprintf(browser_list[block_number]->info,"Number array: %s", value+1);
							case 0X02:
								sprintf(browser_list[block_number]->info,"Character array: %s", value+1);
							case 0X03:
								sprintf(browser_list[block_number]->info,"Code: %s", value+1);
							break;
							default: //Custom header
								sprintf(browser_list[block_number]->info,"Custom header: %s", value+1);
							break;
							}
						}	
					break;
					case 0xFF: //data
						sprintf(browser_list[block_number]->info,"Standard Data %d bytes", longitud);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
							//if ((longitud==6914) || (longitud==49154)) save_scr_browser(value+1);
						}
					break;
					default: //Custom data
						sprintf(browser_list[block_number]->info,"Custom Data %d bytes", longitud);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
						}
					break;
					}	
				break;	
					case 0x11: // turbo
					strcpy(browser_list[block_number]->block_type,"Turbo Speed Data   ");
					retval=fread(value,1,0x0F, fichero);
					retval=fread (value, 1,3 ,fichero);	// read length of current block
					if (retval!=3) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1])+ 65536 * ((unsigned int) value[2]);
					sprintf(browser_list[block_number]->info,"%d bytes", longitud);
					for(bucle=0;bucle<longitud;bucle++)
						retval=fread(value,1,1, fichero);
				break;
					
				case 0x12: // pure tone
					strcpy(browser_list[block_number]->block_type,"Pure Tone          ");
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;}
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					longitud = ((unsigned int) value[2]) + 256 * ((unsigned int) value[3]);
					sprintf(browser_list[block_number]->info,"%d pulses of %d Ts", longitud, len);
				break;

				case 0x13: // multiple pulses
					strcpy(browser_list[block_number]->block_type,"Pulse sequence     ");
					retval=fread(value,1,1,fichero); // number of pulses
					if (retval!=1) {retorno=1;break;} 
					sprintf(browser_list[block_number]->info,"%d pulses", value[0]);
					if(value[0] != 0)
						{
						retval=fread(&value,1,2*value[0],fichero); // length of pulse in T-states
						}
				break;
				
				case 0x14: // Pure data
					strcpy(browser_list[block_number]->block_type,"Pure Data          ");
					retval=fread(value,1,0x07, fichero);
					retval=fread (value, 1, 3, fichero);	// read length of current block
					if (retval!=3) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1])+ 65536 * ((unsigned int) value[2]);
					sprintf(browser_list[block_number]->info,"%d bytes", longitud);
					for(bucle=0;bucle<longitud;bucle++)
						retval=fread(value,1,1, fichero);
				break;

				case 0x20: // pause
					retval=fread(value,1,2,fichero);
					if (retval!=2) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					if (longitud==0) strcpy(browser_list[block_number]->block_type,"Stop the Tape      ");
					else
						{ 
						strcpy(browser_list[block_number]->block_type,"Pause              ");
						sprintf(browser_list[block_number]->info,"%d ms", longitud);
						}
				break;
					
				case 0x21: // group start
					strcpy(browser_list[block_number]->block_type,"Group start        ");
					retval=fread(value,1,1,fichero);
					if (retval!=1) {retorno=1;break;} 
					len = (unsigned int) value[0];
					retval=fread(value,1,len,fichero);
					if (len>35) len=35;
					value[len]=0;
					strcpy(browser_list[block_number]->info,value);
				break;
					
				case 0x22: // group end
					strcpy(browser_list[block_number]->block_type,"Group end          ");
				break;
				
				case 0x23: // jump to block
					strcpy(browser_list[block_number]->block_type,"Jump to block      ");
					retval=fread(block_jump,1,2,fichero);
					if (retval!=2) {retorno=1;break;}
					sprintf(browser_list[block_number]->info,"Block: %d", ((int) block_jump[0]) + 256*((int) block_jump[1]) + ((int) block_number));
				break;

				case 0x24: // loop start
					strcpy(browser_list[block_number]->block_type,"Loop start         ");
					retval=fread(value,1,2, fichero);
				break;
				
				case 0x25: // loop end
					strcpy(browser_list[block_number]->block_type,"Loop end           ");
				break;
				
				case 0x28: // select block
					strcpy(browser_list[block_number]->block_type,"Select             ");
					retval=fread(value,1,2,fichero);
					if (retval!=2) {retorno=1;break;}
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread(value,1,len,fichero);
					sprintf(browser_list[block_number]->info,"%d selections", value[0]);
				break;
				
				case 0x2A: // pause if 48K
					strcpy(browser_list[block_number]->block_type,"Stop tape if 48K mode");
					retval=fread(value,1,4,fichero);
				break;
					
				case 0x30: // text description
					strcpy(browser_list[block_number]->block_type,"Text description:  ");
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {retorno=1;break;} 
					len = (unsigned int) value[0] ;
					retval=fread(value,1,len,fichero);
					if (retval!=len) {retorno=1;break;}
					if (len>35) len=35;
					value[len]=0;
					for(bucle=0;bucle<len;bucle++)
							if (value[bucle]<' ') value[bucle]=' '; //not printable character
					strcpy(browser_list[block_number]->info,value);
				break;
					
				case 0x31: // show text
					strcpy(browser_list[block_number]->block_type,"Show message:      ");
					retval=fread(value,1,1,fichero); //Time lengt
					if (retval!=1) {retorno=1;break;}	
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {retorno=1;break;} 
					len = (unsigned int) value[0];
					retval=fread(value,1,len,fichero);
					if (retval!=len) {retorno=1;break;}
					if (len>35) len=35;
					value[len]=0;
					for(bucle=0;bucle<len;bucle++)
							if (value[bucle]<' ') value[bucle]=' '; //not printable character
					strcpy(browser_list[block_number]->info,value);
				break;
					
				case 0x32: // archive info
					strcpy(browser_list[block_number]->block_type,"Archive info       ");
					retval=fread(value,1,2,fichero); // length
					if (retval!=2) {retorno=1;break;} 
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread(value,1,len,fichero);
				break;
				
				case 0x33: // hardware info
					strcpy(browser_list[block_number]->block_type,"Hardware info      ");
					retval=fread(value,1,1,fichero);
					if (retval!=1) {retorno=1;break;}
					len = (unsigned int) value[0] *3;
					retval=fread(value,1,len,fichero);
				break;
					
				case 0x34: // emulation info
					strcpy(browser_list[block_number]->block_type,"Emulation info     ");
					retval=fread(value,1,8,fichero);
				break;
					
				case 0x35: // custom info
					strcpy(browser_list[block_number]->block_type,"Custom info        ");
					retval=fread(value,1,16,fichero);
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;} 
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]) + 65536*((unsigned int) value[2]) + 16777216*((unsigned int) value[3]);
					for(bucle=0;bucle<len;bucle++)
						retval=fread(value,1,1, fichero);
				break;
					
				default: // not supported
					strcpy(browser_list[block_number]->block_type,"Not supported");
					retorno=1; //Error				
				break;
			}
			block_number++;
		} while ((!feof(fichero))&&(block_number<MAX_BROWSER_ITEM)&&(retorno==0));

rewind_tape (fichero,1);
browser_list[block_number]=NULL;
}


void create_browser_tap (FILE * fichero) {
   
	unsigned int longitud, bucle, block_number, byte_position ;
	unsigned char value[65536], empty, flag_byte;	
	int retval, retorno;
	
	//Free the browser list
	for(bucle=0; ((browser_list[bucle]!=NULL)&&(bucle<MAX_BROWSER_ITEM)); bucle++)
	{
		free (browser_list[bucle]);
		browser_list[bucle]=NULL;
	}
	
	if (fichero == NULL) return;
	
	empty=file_empty(fichero);
	
	if (empty) return;
	
	rewind_tape(fichero,1);
	
	flag_byte=0;
	retorno=0;
	block_number=0;
	do	{
		byte_position=ftell(fichero);
		retval=fread (value, 1, 2, fichero);	// read length of current block
		if (retval!=2) {retorno=1;break;}
		longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
		retval=fread (&flag_byte, 1, 1, fichero);
		if (retval!=1) {retorno=1;break;}
		browser_list[block_number]=(struct browser *)malloc(sizeof(struct browser));
		browser_list[block_number]->position=byte_position;
		strcpy(browser_list[block_number]->info, "                         ");
		strcpy(browser_list[block_number]->block_type,"Standard Speed Data");
		printf("TAP browser: flag byte %X en %ld\n",flag_byte, ftell(fichero));
			switch(flag_byte)
			{
				case 0x00: //header
				if (longitud!=19) 
						{
							sprintf(browser_list[block_number]->info,"Custom Data %d bytes", longitud);
							if (longitud>1)
							{
								retval=fread (value, 1, longitud-1, fichero);
								if (retval!=(longitud-1)) {retorno=1;break;}
							}
						}
						else
						{
						retval=fread (value, 1, 18, fichero);
						if (retval!=18) {retorno=1;break;}
						value[11]=0;
						for(bucle=1;bucle<11;bucle++)
							if (value[bucle]<' ') value[bucle]='?'; //not printable character
							
						switch(value[0]	) //Type
							{
							case 0x00: //PROG
								sprintf(browser_list[block_number]->info,"Program: %s", value+1);
							break;
							case 0X01: //DATA
								sprintf(browser_list[block_number]->info,"Number array: %s", value+1);
							case 0X02:
								sprintf(browser_list[block_number]->info,"Character array: %s", value+1);
							case 0X03:
								sprintf(browser_list[block_number]->info,"Code: %s", value+1);
							break;
							default: //Custom header
								sprintf(browser_list[block_number]->info,"Custom header: %s", value+1);
							break;
							}
						}	
					break;
					case 0xFF: //data
						sprintf(browser_list[block_number]->info,"Standard Data %d bytes", longitud);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
							//if ((longitud==6914) || (longitud==49154)) save_scr_browser(value+1);
						}
					break;
					default: //Custom data
						sprintf(browser_list[block_number]->info,"Custom Data %d bytes", longitud);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
						}
					break;
					}
			block_number++;
		} while ((!feof(fichero))&&(block_number<MAX_BROWSER_ITEM)&&(retorno==0));

rewind_tape (fichero,1);
browser_list[block_number]=NULL;
}

int select_block(FILE * fichero)
{
	unsigned int longitud, bucle, block_number,nblocks,offset, blk_sel_pos;
	unsigned char value[64], len_text;	
	int retval;
	
	block_number=0;
	
	//Free the block list
	for(bucle=0; ((block_select_list[bucle]!=NULL)&&(bucle<MAX_SELECT_ITEM)); bucle++)
	{
		free (block_select_list[bucle]);
		block_select_list[bucle]=NULL;
	}
	
	//search for select block position
	for(blk_sel_pos=0; 
	((blk_sel_pos<MAX_BROWSER_ITEM)&&(browser_list[blk_sel_pos]!=NULL)&&(browser_list[blk_sel_pos]->position!=ordenador.tape_position)); 
	blk_sel_pos++);
	
	if (browser_list[blk_sel_pos]==NULL) return -1;
	
	if (browser_list[blk_sel_pos]->position!=ordenador.tape_position) return -1;
	
	retval=fread(value,1,3,fichero);
	if (retval!=3) {return -1;} 
	longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
	nblocks=value[2];

	if (nblocks>MAX_SELECT_ITEM) {return -1;}
	
	for(block_number=0;block_number<nblocks;block_number++)
	{
		retval=fread (value, 1, 2, fichero); //Read offset
		if (retval!=2) return -1;
		offset = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
		
		retval=fread (&len_text, 1, 1, fichero); //Read len text
		if (retval!=1) return -1;
		
		if (len_text>64) return -1;
		retval=fread (value, 1, len_text, fichero); //Read text
		if (retval!=len_text) return -1;
		if (len_text>31) len_text=31;
		value[len_text]=0;
		
		block_select_list[block_number]=(struct tape_select *)malloc(sizeof(struct tape_select));
		block_select_list[block_number]->offset = offset;
		strcpy(block_select_list[block_number]->info, value);
		} 
		
	block_select_list[block_number]=NULL;	
	if (feof(fichero)) return -1;
	
	unsigned int block_n_int;
	const char *row_selected; 
	char block_n[3];
	
	row_selected = menu_select_tape_block();
	
	if (row_selected==NULL) // Aborted
		return -1; 
	
	strncpy(block_n, row_selected,2);
	
	block_n[2]=0;
	
	block_n_int=atoi(block_n);
	
	if ((block_n_int >(MAX_SELECT_ITEM-1))||block_select_list[block_n_int]==NULL) return -1;
	
	if ((block_select_list[block_n_int]->offset+blk_sel_pos) > (MAX_BROWSER_ITEM-1)) return -1;
	
	if (browser_list[block_select_list[block_n_int]->offset+blk_sel_pos]==NULL) return -1;
	
	ordenador.tape_position=browser_list[block_select_list[block_n_int]->offset+blk_sel_pos]->position;
 
	ordenador.tape_current_bit=0;
	ordenador.tape_current_mode=TAP_TRASH;
	ordenador.next_block= NOBLOCK;
	
	fseek(ordenador.tap_file, ordenador.tape_position, SEEK_SET);
	free((void*)row_selected);
	
	return 0;
}

int jump_to_block(FILE * fichero, int blocks_to_jump)
{

	int blk_sel_pos, dest_block;

	
	//search for block position
	for(blk_sel_pos=0; 
	((blk_sel_pos<MAX_BROWSER_ITEM)&&(browser_list[blk_sel_pos]!=NULL)&&(browser_list[blk_sel_pos]->position!=ordenador.tape_position)); 
	blk_sel_pos++);
	
	if (browser_list[blk_sel_pos]==NULL) return -1;
	
	if (browser_list[blk_sel_pos]->position!=ordenador.tape_position) return -1;
	
	dest_block=blk_sel_pos+blocks_to_jump;
	
	if ((dest_block<0)||(dest_block>MAX_BROWSER_ITEM)) return -1;
	
	if (browser_list[dest_block]==NULL) return -1;
	
	fseek(fichero, browser_list[dest_block]->position, SEEK_SET);
	
	ordenador.tape_position = browser_list[dest_block]->position;

	return 0;
}

void free_browser()
{
	unsigned int bucle; 
	
	//Free the browser list
	for(bucle=0; ((browser_list[bucle]!=NULL)&&(bucle<MAX_BROWSER_ITEM)); bucle++)
	{
		free (browser_list[bucle]);
		browser_list[bucle]=NULL;
	}
	
	//Free the block list
	for(bucle=0; ((block_select_list[bucle]!=NULL)&&(bucle<MAX_SELECT_ITEM)); bucle++)
	{
		free (block_select_list[bucle]);
		block_select_list[bucle]=NULL;
	}
	
}

/*
void save_scr_browser(unsigned char* zx_screen)
{ 
	//const char *tape = ordenador.last_selected_file;
	const char *tape =ordenador.current_tap;
	char *ptr;
	FILE *fichero;
	char db[MAX_PATH_LENGTH];
	char fb[81];
	int retorno,retval;
	
	 
	// Name (for saves) - TO CHECK
	if (tape && strrchr(tape, '/'))
		strncpy(fb, strrchr(tape, '/') + 1, 80);
	else
		strcpy(fb, "unknown");
		
	//remove the extension
	ptr = strrchr (fb, '.');
		if (ptr) *ptr = 0;
					
	
	// Save SCR 1 file		
	snprintf(db, MAX_PATH_LENGTH-1, "%s/%s.scr", path_scr1, fb);
	
		
	fichero=fopen(db,"rb");
	
	if(fichero!=NULL)
	{	
		fclose(fichero);
		return; // file already exists
	}
	
	fichero=fopen(db,"wb"); // create for write
		
	if(fichero==NULL)
		retorno=-1;
	else {
		retval=fwrite(zx_screen,6912,1,fichero); // save screen
		//if (ordenador.ulaplus!=0) {
			//retval=fwrite(ordenador.ulaplus_palete,64,1,fichero); // save ULAPlus palete
			//}
		fclose(fichero);
		retorno=0;
		}

	switch(retorno) {
	case 0:
		msgInfo("Screen saved",3000,NULL);
		break;
	case -1:
		msgInfo("Can't create screen file",3000,NULL);
	break;
	default:
	break;
	}
	return;
}
*/

int extract_screen_tap (char *screen, FILE * fichero) {
   
	unsigned int longitud;
	unsigned char value[65536], empty, flag_byte;	
	int retval, retorno;
	
	if (fichero == NULL) return -1;
	
	empty=file_empty(fichero);
	
	if (empty) return -1;
	
	flag_byte=0;
	retorno=0;
	do	{
		retval=fread (value, 1, 2, fichero);	// read length of current block
		if (retval!=2) {retorno=1;break;}
		longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
		retval=fread (&flag_byte, 1, 1, fichero);
		if (retval!=1) {retorno=1;break;}
		//printf("TAP browser: flag byte %X en %ld\n",flag_byte, ftell(fichero));
			switch(flag_byte)
			{
					case 0x00: //header
					if (longitud!=19)  //custom data
						{
							if (longitud>1)
							{
								retval=fread (value, 1, longitud-1, fichero);
								if (retval!=(longitud-1)) {retorno=1;break;}
								if ((longitud==6914) || (longitud==49154)) {memcpy(screen, value+1, 6912);rewind_tape (fichero,1);return 0;}
							}
						}
						else
						{
						retval=fread (value, 1, 18, fichero);
						if (retval!=18) {retorno=1;break;}
						}	
					break;
					case 0xFF: //data
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
							if ((longitud==6914) || (longitud==49154)) {memcpy(screen, value+1, 6912);rewind_tape (fichero,1);return 0;}
						}
					break;
					default: //Custom data
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
							if ((longitud==6914) || (longitud==49154)) {memcpy(screen, value+1, 6912);rewind_tape (fichero,1);return 0;}
						}
					break;
					}
		} while ((!feof(fichero))&&(retorno==0));

rewind_tape (fichero,1);
return -1;
}

int extract_screen_tzx (char *screen, FILE * fichero) 
{

	unsigned int longitud, bucle, byte_position, retorno;
	unsigned char value[65536], empty, blockid, pause[2], flag_byte;
	char block_jump[2];
	int retval, retval2; 
	//int found_screen;
	
	longitud =0;
	pause[0]=pause[1]=0;
	
	blockid=0;
	flag_byte=0;
	//found_screen=0;
	
	if (fichero == NULL) return -1;
	
	empty=file_empty(fichero);
	
	if (empty) return -1;

	retorno=0;
	do	{
		byte_position=ftell(fichero);
		retval=fread (&blockid, 1, 1, fichero); //Read id block
		if (retval!=1) {retorno=1;break;}
		
		//printf("TZX browser: %X en %d\n",blockid, byte_position+1);
		
		switch(blockid) {
				case 0x10: // classic tape block
					retval=fread (pause, 1, 2, fichero); //pause lenght
					retval2=fread (value, 1, 2, fichero);	// read length of current block
					if ((retval!=2)||(retval2!=2)) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread (&flag_byte, 1, 1, fichero);
					if (retval!=1) {retorno=1;break;}
					switch(flag_byte)
					{
						case 0x00: //header
						if (longitud!=19) //custom data
						{
							if (longitud>1)
							{
								retval=fread (value, 1, longitud-1, fichero);
								if (retval!=(longitud-1)) {retorno=1;break;}
								if ((longitud==6914) || (longitud==49154)) {memcpy(screen, value+1, 6912);rewind_tape (fichero,1);return 0;}
							}
						}
						else
						{
						retval=fread (value, 1, 18, fichero);
						if (retval!=18) {retorno=1;break;}
						}	
						break;
						case 0xFF: //data
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
							if ((longitud==6914) || (longitud==49154)) {memcpy(screen, value+1, 6912);rewind_tape (fichero,1);return 0;}
						}
						break;
						default: //Custom data
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
							if ((longitud==6914) || (longitud==49154)) {memcpy(screen, value+1, 6912);rewind_tape (fichero,1);return 0;}
						}
						break;
					}	
				break;
	
				case 0x11: // turbo
					fread(value,1,0x0F, fichero);
					retval=fread (value, 1,3 ,fichero);	// read length of current block
					if (retval!=3) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1])+ 65536 * ((unsigned int) value[2]);
					if (longitud< 65536) 
						{retval=fread (value, 1, longitud, fichero); if (retval!=longitud) {retorno=1;break;}}
					else
					for(bucle=0;bucle<longitud;bucle++)
						fread(value,1,1, fichero);
				//if (longitud==6144) {printf("Possible screen (6144) in turbo data\n");}
				//if (longitud==6912) {printf("Possible screen (6912) in turbo data\n");}
				break;
					
				case 0x12: // pure tone
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;}
				break;

				case 0x13: // multiple pulses
					retval=fread(value,1,1,fichero); // number of pulses
					if (retval!=1) {retorno=1;break;} 
					if(value[0] != 0)
						{
						fread(&value,1,2*value[0],fichero); // length of pulse in T-states
						}
				break;
				
				case 0x14: // Pure data
					fread(value,1,0x07, fichero);
					retval=fread (value, 1, 3, fichero);	// read length of current block
					if (retval!=3) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1])+ 65536 * ((unsigned int) value[2]);
					if (longitud< 65536) 
						{retval=fread (value, 1, longitud, fichero); if (retval!=longitud) {retorno=1;break;}}
					else
					for(bucle=0;bucle<longitud;bucle++)
						fread(value,1,1, fichero);
					//if (longitud==6144) {memcpy(screen, value, 6144);found_screen=1;}
					//if ((longitud==768) && (found_screen)) {memcpy(screen+6144, value, 768);rewind_tape (fichero,1);return 0;}
					//if (longitud==6912) {memcpy(screen, value, 6912);rewind_tape (fichero,1);return 0;}
				break;

				case 0x20: // pause
					retval=fread(value,1,2,fichero);
					if (retval!=2) {retorno=1;break;}
				break;
					
				case 0x21: // group start
					retval=fread(value,1,1,fichero);
					if (retval!=1) {retorno=1;break;} 
					longitud = (unsigned int) value[0];
					retval=fread(value,1,longitud,fichero);
					if (retval!=longitud) {retorno=1;break;}
				break;
					
				case 0x22: // group end
				break;
				
				case 0x23: // jump to block
					retval=fread(block_jump,1,2,fichero);
					if (retval!=2) {retorno=1;break;}
				break;

				case 0x24: // loop start
					retval=fread(value,1,2, fichero);
					if (retval!=2) {retorno=1;break;}
				break;
				
				case 0x25: // loop end
				break;
				
				case 0x28: // select block
					retval=fread(value,1,2,fichero);
					if (retval!=2) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread(value,1,longitud,fichero);
					if (retval!=longitud) {retorno=1;break;}
				break;
				
				case 0x2A: // pause if 48K
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;}
				break;
					
				case 0x30: // text description
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {retorno=1;break;} 
					longitud = (unsigned int) value[0] ;
					retval=fread(value,1,longitud,fichero);
					if (retval!=longitud) {retorno=1;break;}
				break;
					
				case 0x31: // show text
					retval=fread(value,1,1,fichero); //Time lengt
					if (retval!=1) {retorno=1;break;}	
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {retorno=1;break;} 
					longitud = (unsigned int) value[0];
					retval=fread(value,1,longitud,fichero);
					if (retval!=longitud) {retorno=1;break;}
				break;
					
				case 0x32: // archive info
					retval=fread(value,1,2,fichero); // length
					if (retval!=2) {retorno=1;break;} 
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread(value,1,longitud,fichero);
					if (retval!=longitud) {retorno=1;break;}
				break;
				
				case 0x33: // hardware info
					retval=fread(value,1,1,fichero);
					if (retval!=1) {retorno=1;break;}
					longitud = (unsigned int) value[0] *3;
					retval=fread(value,1,longitud,fichero);
					if (retval!=longitud) {retorno=1;break;}
				break;
					
				case 0x34: // emulation info
					retval=fread(value,1,8,fichero);
					if (retval!=8) {retorno=1;break;}
				break;
					
				case 0x35: // custom info
					fread(value,1,16,fichero);
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;} 
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]) + 65536*((unsigned int) value[2]) + 16777216*((unsigned int) value[3]);
					if (longitud< 65536) 
						{retval=fread (value, 1, longitud, fichero); if (retval!=longitud) {retorno=1;break;}}
					else
					for(bucle=0;bucle<longitud;bucle++)
						fread(value,1,1, fichero);
				break;
					
				default: // not supported
					retorno=1; //Error				
				break;
			}
		} while ((!feof(fichero))&&(retorno==0));

rewind_tape (fichero,1);
return -1;
}

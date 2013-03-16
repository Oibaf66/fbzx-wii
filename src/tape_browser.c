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

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

struct browser *browser_list[MAX_BROWSER_ITEM+1];

void browser_tzx (FILE * fichero) {

	unsigned int longitud, len, bucle, byte_position, retorno, block_number;
	unsigned char value[65536], empty, blockid, pause[2], flag_byte;	
	int retval, retval2;
	
	longitud =0;
	pause[0]=pause[1]=0;
	
	blockid=0;
	flag_byte=0;
	empty=file_empty(fichero);
	
	if (fichero == NULL) 
	{
		sprintf (ordenador.osd_text, "No tape selected");
		ordenador.osd_time = 100;
		return;
	}
		
	if(empty)
	{
		sprintf (ordenador.osd_text, "Tape file empty");
		return;
	}
			
	//Free the browser list
	for(bucle=0; ((browser_list[bucle]!=NULL)&&(bucle<MAX_BROWSER_ITEM)); bucle++)
	{
		free (browser_list[bucle]);
		browser_list[bucle]=NULL;
	}
	
	retorno=0;
	block_number=0;
	do	{
		byte_position=ftell(fichero);
		retval=fread (&blockid, 1, 1, fichero); //Read id block
		if (retval!=1) {retorno=1;break;}
		browser_list[block_number]=(struct browser *)malloc(sizeof(struct browser));
		//if (browser_list[block_number]==NULL) retorno=1;
		browser_list[block_number]->position=byte_position;
		strcpy(browser_list[block_number]->info, "                        ");
		
		printf("TZX: browser: %X en %d\n",blockid, byte_position);
		
		switch(blockid) {
				case 0x10: // classic tape block
				strcpy(browser_list[block_number]->block_type,"Standard speed data");
				retval=fread (pause, 1, 2, fichero); //pause lenght
				retval2=fread (value, 1, 2, fichero);	// read length of current block
				if ((retval!=2)||(retval2!=2)) {retorno=1;break;}
				longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
				retval=fread (&flag_byte, 1, 1, fichero);
				if (retval!=1) {retorno=1;break;}
				switch(flag_byte)
					{
					case 0x00: //header
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
							default: //??
								strcpy(browser_list[block_number]->info,"???");
							break;
							}
					break;
					case 0xFF: //data
						sprintf(browser_list[block_number]->info,"Standard Data: %d bytes", longitud);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
						}
					break;
					default: //Custom data
						sprintf(browser_list[block_number]->info,"Custom Data: %d bytes", longitud);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
						}
					break;
					}	
				break;	
					case 0x11: // turbo
					strcpy(browser_list[block_number]->block_type,"Turbo speed data   ");
					retval=fread(value,1,0x0F, fichero);
					retval=fread (value, 1,3 ,fichero);	// read length of current block
					if (retval!=3) {retorno=1;break;}
					longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1])+ 65536 * ((unsigned int) value[2]);
					sprintf(browser_list[block_number]->info,"%d bytes", longitud);
					for(bucle=0;bucle<longitud;bucle++)
						retval=fread(value,1,1, fichero);
				break;
					
				case 0x12: // pure tone
					strcpy(browser_list[block_number]->block_type,"Pure tone          ");
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;}
					longitud = ((unsigned int) value[2]) + 256 * ((unsigned int) value[3]);
					sprintf(browser_list[block_number]->info,"%d pulses", longitud);
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
					strcpy(browser_list[block_number]->block_type,"Pure data          ");
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
					if (longitud==0) strcpy(browser_list[block_number]->block_type,"Stop tape          ");
					else
						{ 
						strcpy(browser_list[block_number]->block_type,"Pause:             ");
						sprintf(browser_list[block_number]->info,"%d ms", longitud);
						}
				break;
					
				case 0x21: // group start
					strcpy(browser_list[block_number]->block_type,"Group start        ");
					retval=fread(value,1,1,fichero);
					if (retval!=1) {retorno=1;break;} 
					len = (unsigned int) value[0];
					retval=fread(value,1,len,fichero);
					if (len>31) len=31;
					value[len]=0;
					strcpy(browser_list[block_number]->info,value);
				break;
					
				case 0x22: // group end
					strcpy(browser_list[block_number]->block_type,"Group end          ");
				break;
				
				case 0x24: // loop start
					strcpy(browser_list[block_number]->block_type,"Loop start         ");
					retval=fread(value,1,2, fichero);
				break;
				
				case 0x25: // loop end
					strcpy(browser_list[block_number]->block_type,"Loop end           ");
				break;
				
				case 0x28: // select block
					strcpy(browser_list[block_number]->block_type,"Select block       ");
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
					strcpy(browser_list[block_number]->block_type,"Text:              ");
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {retorno=1;break;} 
					len = (unsigned int) value[0] ;
					retval=fread(value,1,len,fichero);
					if (retval!=len) {retorno=1;break;}
					if (len>31) len=31;
					value[len]=0;
					for(bucle=0;bucle<len;bucle++)
							if (value[bucle]<' ') value[bucle]=' '; //not printable character
					strcpy(browser_list[block_number]->info,value);
				break;
					
				case 0x31: // show text
					strcpy(browser_list[block_number]->block_type,"Show text:         ");
					retval=fread(value,1,1,fichero); //Time lengt
					if (retval!=1) {retorno=1;break;}	
					retval=fread(value,1,1,fichero); // length
					if (retval!=1) {retorno=1;break;} 
					len = (unsigned int) value[0];
					retval=fread(value,1,len,fichero);
					if (retval!=len) {retorno=1;break;}
					if (len>31) len=31;
					value[len]=0;
					for(bucle=0;bucle<len;bucle++)
							if (value[bucle]<' ') value[bucle]=' '; //not printable character
					strcpy(browser_list[block_number]->info,value);
				break;
					
				case 0x32: // archive info
					strcpy(browser_list[block_number]->block_type,"Archive info:      ");
					retval=fread(value,1,2,fichero); // length
					if (retval!=2) {retorno=1;break;} 
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
					retval=fread(value,1,len,fichero);
				break;
				
				case 0x33: // hardware info
					strcpy(browser_list[block_number]->block_type,"Hardware info:     ");
					retval=fread(value,1,1,fichero);
					if (retval!=1) {retorno=1;break;}
					len = (unsigned int) value[0] *3;
					retval=fread(value,1,len,fichero);
				break;
					
				case 0x34: // emulation info
					strcpy(browser_list[block_number]->block_type,"Emulation info:    ");
					retval=fread(value,1,8,fichero);
				break;
					
				case 0x35: // custom info
					strcpy(browser_list[block_number]->block_type,"Custom info:       ");
					retval=fread(value,1,16,fichero);
					retval=fread(value,1,4,fichero);
					if (retval!=4) {retorno=1;break;} 
					len = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]) + 65536*((unsigned int) value[2]) + 16777216*((unsigned int) value[3]);
					for(bucle=0;bucle<len;bucle++)
						retval=fread(value,1,1, fichero);
				break;
					
				default: // not supported
					strcpy(browser_list[block_number]->block_type,"Not supported");
					retorno=1; //Tape error				
				break;
			}
			block_number++;
		} while ((!feof(fichero))&&(block_number<MAX_BROWSER_ITEM)&&(retorno==0));

rewind_tape (fichero,1);
browser_list[block_number]=NULL;
}


void browser_tap (FILE * fichero) {
   
	unsigned int longitud, bucle, block_number, byte_position ;
	unsigned char value[65536], empty, flag_byte;	
	int retval, retorno;

	empty=file_empty(fichero);

	
	if (fichero == NULL) 
	{
		sprintf (ordenador.osd_text, "No tape selected");
		ordenador.osd_time = 100;
		return;
	}
		
	if(empty)
	{
		sprintf (ordenador.osd_text, "Tape file empty");
		return;
	}
	
	//Free the browser list
	for(bucle=0; ((browser_list[bucle]!=NULL)&&(bucle<MAX_BROWSER_ITEM)); bucle++)
	{
		free (browser_list[bucle]);
		browser_list[bucle]=NULL;
	}
	
	flag_byte=0;
	retorno=0;
	block_number=0;
	do	{
		byte_position=ftell(fichero);
		browser_list[block_number]=(struct browser *)malloc(sizeof(struct browser));
		//if (browser_list[block_number]==NULL) retorno=1;
		browser_list[block_number]->position=byte_position;
		printf("TAP: browser: %X en %d\n",10, byte_position);
		strcpy(browser_list[block_number]->block_type,"Standard speed data");
		retval=fread (value, 1, 2, fichero);	// read length of current block
		if (retval!=2) {retorno=1;break;}
		longitud = ((unsigned int) value[0]) + 256 * ((unsigned int) value[1]);
		retval=fread (&flag_byte, 1, 1, fichero);
		if (retval!=1) {retorno=1;break;}
			switch(flag_byte)
			{
				case 0x00: //header
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
							default: //??
								strcpy(browser_list[block_number]->info,"???");
							break;
							}
					break;
					case 0xFF: //data
						sprintf(browser_list[block_number]->info,"Standard Data: %d bytes", longitud-2);
						if (longitud>1)
						{
							retval=fread (value, 1, longitud-1, fichero);
							if (retval!=(longitud-1)) {retorno=1;break;}
						}
					break;
					default: //Custom data
						sprintf(browser_list[block_number]->info,"Custom Data: %d bytes", longitud-2);
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

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
#include "cargador.h"
#include "menu_sdl.h"
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

void uncompress_z80(FILE *fichero,int length,unsigned char *memo) {

	unsigned char byte_loaded,EDfound,counter;
	int position,retval;

	counter=0;
	byte_loaded=0;
	EDfound=0;
	position=0;
  
	printf("Descomprimo de longitud %d\n",length);
	
	do {
		if(counter) {
			memo[position++]=byte_loaded;
			counter--;
			continue;
		} else
			retval=fread(&byte_loaded,1,1,fichero);
    
		if(EDfound==2) { // we have two EDs
			counter=byte_loaded;
			retval=fread(&byte_loaded,1,1,fichero); //ED byte 
			EDfound=0;
			continue;
		}
    
		if(byte_loaded==0xED) {
			if (position+1<length) EDfound++; else memo[position++]=0xED;	//If ED is the last byte of the block
		} else {
			if(EDfound==1) { // we found ED xx. We write ED and xx
				memo[position++]=0xED;
				EDfound=0;
			}
			memo[position++]=byte_loaded;
		}
	} while(position<length);

}


int count_repetition(unsigned int read_position, unsigned char byte_read, unsigned char *memo_in)
{
	int count = 1;

    while (read_position < 16384 && count < 255 && byte_read == memo_in[read_position++])
	{
		count++;
	}

 return count;
 }



//Compress block of 16384 bytes
int compress_z80 (unsigned char *memo_in, unsigned char *memo_out)
{
 unsigned int read_position, write_position, loop;
 unsigned char byte_read, nrepeats;
 
 write_position = 0;
 read_position =0; 
 
 
 while (read_position < 16384) {
            byte_read = memo_in[read_position];
            nrepeats = count_repetition(read_position+1, byte_read, memo_in);
            
			if (byte_read == 0xED) 
			{
                if (nrepeats == 1) {
                    //The byte which follows ED is never compressed
                    memo_out[write_position++] = 0xED;
					read_position++;
                    if (read_position < 16384) memo_out[write_position++] = memo_in[read_position++];
					else printf("Last ED in block found\n");
                } else {
                    //Consecutive ED are always compressed even if less than 5
                    memo_out[write_position++] =  0xED;
                    memo_out[write_position++] =  0xED;
                    memo_out[write_position++] =  nrepeats;
                    memo_out[write_position++] =  0xED;
                    read_position += nrepeats;
                }
            } 
			else 
			{
                if (nrepeats < 5) {
                    //Less than 5 equal values are not compressed
					for(loop=0; loop < nrepeats; loop++)
						memo_out[write_position++] =  byte_read;
					
                } else {
                    memo_out[write_position++] =  0xED;
                    memo_out[write_position++] =  0xED;
                    memo_out[write_position++] =  nrepeats;
                    memo_out[write_position++] =  byte_read; 
                }
				read_position += nrepeats;
            }
        }
		
	return write_position;
}

int save_z80(char *filename, int overwrite) {

  FILE *fichero;
  unsigned char value,bucle;
  int retval;
  unsigned int length_compress;
  unsigned char memo_out[16384];

  fichero=fopen(filename,"rb");
  if((fichero!=NULL)&&(!overwrite)) {
    fclose(fichero);
    return -1; // file already exists
  }

  fichero=fopen(filename,"wb");

  if(fichero==NULL)
    return -2; // can't create file

  fprintf(fichero,"%c%c%c%c%c%c",procesador.Rm.br.A,procesador.Rm.br.F,procesador.Rm.br.C,procesador.Rm.br.B,procesador.Rm.br.L,procesador.Rm.br.H); // AF, BC and HL

  fprintf(fichero,"%c%c",0,0); // V 2.0

  fprintf(fichero,"%c%c",procesador.Rm.br.P,procesador.Rm.br.S); // SP
  fprintf(fichero,"%c%c%c",procesador.I,procesador.R,(((procesador.R2>>7)&0x01)|((ordenador.border<<1)&0x0E)|0x20)); // I, R and border color , compressed

  fprintf(fichero,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c",procesador.Rm.br.E,procesador.Rm.br.D,procesador.Ra.br.C,procesador.Ra.br.B,procesador.Ra.br.E,procesador.Ra.br.D,procesador.Ra.br.L,procesador.Ra.br.H,procesador.Ra.br.A,procesador.Ra.br.F,procesador.Rm.br.IYl,procesador.Rm.br.IYh,procesador.Rm.br.IXl,procesador.Rm.br.IXh);

  if (procesador.IFF1)
    value=1;
  else
    value=0;
  fprintf(fichero,"%c",value);
  if (procesador.IFF2)
    value=1;
  else
    value=0;
  fprintf(fichero,"%c",value);
  value=procesador.IM;
  if(ordenador.issue==2)
    value|=4;
  switch (ordenador.joystick[0]) { //Only one Joystick in Z80 file
  case 1:
  	value|=64;
  	break;
  case 2:
  	value|=128;
  	break;
  case 3:
  	value|=192;
  	break;
  }
  fprintf(fichero,"%c",value);
  
  //Additional header values of v2.01
  fprintf(fichero,"%c%c",23,0); // Z80 file v2.01
  fprintf(fichero,"%c%c",(byte)(procesador.PC&0x0FF),(byte)((procesador.PC>>8)&0x0FF)); // PC
  if (ordenador.mode128k==0) fprintf(fichero,"%c",0); //48k
	else fprintf(fichero,"%c",3); // 128k
  fprintf(fichero,"%c",ordenador.mport1); // content of 0x7FFD latch
  fprintf(fichero,"%c",0); // no If1
  fprintf(fichero,"%c",ordenador.ay_emul<<2); // Ay emulation
  fprintf(fichero,"%c",ordenador.ay_latch); // last selected AY register
  for(bucle=0;bucle<16;bucle++)
    fprintf(fichero,"%c",ordenador.ay_registers[bucle]); // AY registers
  
  printf("Compressing and saving z80 file ...\n");
  
  if (ordenador.mode128k==0)
  {
	length_compress = compress_z80(ordenador.memoria+(16384*5)+65536, memo_out);
	printf("Length %d page %d\n", length_compress, 8);
    fprintf(fichero,"%c%c",(byte) (length_compress), (byte) (length_compress>>8)); 
	fprintf(fichero,"%c",8); // page number
    retval=fwrite(memo_out,length_compress,1,fichero); // store page
	
	length_compress = compress_z80(ordenador.memoria+(16384*2)+65536, memo_out);
	printf("Length %d page %d\n", length_compress, 4);
    fprintf(fichero,"%c%c",(byte) (length_compress), (byte) (length_compress>>8)); 
	fprintf(fichero,"%c",4); // page number
    retval=fwrite(memo_out,length_compress,1,fichero); // store page
	
	length_compress = compress_z80(ordenador.memoria+(16384*3)+65536, memo_out);
	printf("Length %d page %d\n", length_compress, 5);
    fprintf(fichero,"%c%c",(byte) (length_compress), (byte) (length_compress>>8)); 
	fprintf(fichero,"%c",5); // page number
    retval=fwrite(memo_out,length_compress,1,fichero); // store page
  }
  else
  {
	for(bucle=0;bucle<8;bucle++) 
	{
		length_compress = compress_z80(ordenador.memoria+(16384*bucle)+65536, memo_out);
		printf("Length %d page %d\n", length_compress, bucle+3);
		fprintf(fichero,"%c%c",(byte) (length_compress), (byte) (length_compress>>8)); 
		fprintf(fichero,"%c",bucle+3); // page number
		retval=fwrite(memo_out,length_compress,1,fichero); // store page
	}
  }
  fclose(fichero);
  return 0;
}

int load_z80(char *filename) {

	struct z80snapshot *snap;
	unsigned char tempo[30],tempo2[57],type,compressed,page,byte_read[3];
	unsigned char *memo;
	FILE *fichero;
	int longitud=0,longitud2,bucle,retval;

	memo=(unsigned char *)malloc(49152);
	snap=(struct z80snapshot *)malloc(sizeof(struct z80snapshot));

	longitud=strlen(filename);
	if ((longitud>4)&&(0==strcasecmp(".sna",filename+(longitud-4)))) {
		printf("Read SNA file\n");
		free(memo);
		free(snap);
		return load_sna(filename);
	}

	printf("Read Z80 file\n");

	for(bucle=0;bucle<16;bucle++)
		snap->ay_regs[bucle]=0;

	fichero=fopen(filename,"rb");
	if(fichero==NULL) {
		free(memo);
		free(snap);
		return -1; // error
	}

	printf("Read header (first 30 bytes)\n");
	retval=fread(tempo,1,30,fichero);

	if((tempo[6]==0)&&(tempo[7]==0)) { // extended Z80
		printf("It's an extended Z80 file\n");
		type=1; // new type
		
		retval=fread(tempo2,1,2,fichero); // read the length of the extension
 
		longitud=((int)tempo2[0])+256*((int)tempo2[1]);
		if(longitud>55) {
			fclose(fichero);
			printf("Not suported Z80 file\n");
			free(memo);
			free(snap);
			return -3; // not a supported Z80 file
		}
		printf("Length: %d\n",longitud);
		retval=fread(tempo2+2,1,longitud,fichero);

		if(longitud==23) // z80 ver 2.01
			switch(tempo2[4]) {
			case 0:
			case 1:
				snap->type=0; // 48K
				break;
			case 3:
			case 4:
				snap->type=1; // 128K
			break;
			case 12: //+2
				snap->type=2; // +2
				break;
			case 7: //3+
			case 13: //+2a
				snap->type=3; // +3
				break;
			case 9:
				printf("z80: warning Pentagon 128k, using ZX 128k model emulation\n");
				msgInfo("RZX recorded with Pentagon 128k", 3000, NULL);
				snap->type=1; // 128K
				break;
			default:
				fclose(fichero);
				printf("z80: model not suported: %d\n", tempo2[4]);
				free(memo);
				free(snap);
				return -3; // not a supported Z80 file
			break;
			}
		else // z80 ver 3.0x
			switch(tempo2[4]) {
			case 0:
			case 1:
			case 3:
				snap->type=0; // 48K
				break;
			case 4:
			case 5:
			case 6:
				snap->type=1; // 128K
				break;
			case 12: //+2
				snap->type=2; // +2
				break;
			case 7: //3+
			case 13: //+2a
				snap->type=3; // +3
				break;
			case 9:
				printf("z80: warning Pentagon 128k, using ZX 128k model emulation\n");
				msgInfo("RZX recorded with Pentagon 128k", 3000, NULL);
				snap->type=1; // 128K
				break;
			default:
				fclose(fichero);
				printf("z80: model not suported: %d\n", tempo2[4]);
				free(memo);
				free(snap);
				return -3; // not a supported Z80 file
				break;
			}      
	} else {
		printf("Old type z80\n");
		type=0; // old type
		snap->type=0; // 48k
	}

	if(tempo[29]&0x04) {
		printf("Issue 2\n");
		snap->issue=2; // issue2
	} else {
		printf("Issue 3\n");
		snap->issue=3; // issue3
	}

	snap->A=tempo[0];
	snap->F=tempo[1];
	snap->C=tempo[2];
	snap->B=tempo[3];
	snap->L=tempo[4];
	snap->H=tempo[5];
	if(type) {
		snap->PC=((word)tempo2[2])+256*((word)tempo2[3]);
		for(bucle=0;bucle<16;bucle++)
			snap->ay_regs[bucle]=tempo2[9+bucle];
		snap->ay_latch=tempo2[8];
	} else {
		snap->PC=((word)tempo[6])+256*((word)tempo[7]);
	}

	snap->SP=((word)tempo[8])+256*((word)tempo[9]);
	snap->I=tempo[10];
	snap->R=(tempo[11]&0x7F);

	if(tempo[12]==255) {
		printf("Byte 12 is 255! doing 1\n");
		tempo[12]=1;
	}

	if(tempo[12]&0x01)
		snap->R|=0x80;

	snap->border=(tempo[12]>>1)&0x07;

	if(tempo[12]&32)
		compressed=1;
	else
		compressed=0;

	snap->E=tempo[13];
	snap->D=tempo[14];
	snap->CC=tempo[15];
	snap->BB=tempo[16];
	snap->EE=tempo[17];
	snap->DD=tempo[18];
	snap->LL=tempo[19];
	snap->HH=tempo[20];
	snap->AA=tempo[21];
	snap->FF=tempo[22];
	snap->IY=((word)tempo[23])+256*((word)tempo[24]);
	snap->IX=((word)tempo[25])+256*((word)tempo[26]);

	if(tempo[27]!=0)
		snap->IFF1=1;
	else
		snap->IFF1=0;

	if(tempo[28]!=0)
		snap->IFF2=1;
	else
		snap->IFF2=0;
	
	switch(tempo[29]&0x03) {
	case 0:
		snap->Imode=0;
		break;
	case 1:
		snap->Imode=1;
		break;
	case 2:
		snap->Imode=2;
		break;
	}

	snap->joystick=((tempo[29]>>6)&0x03);

	if(type)
	{
		snap->pager=tempo2[5];
		if (longitud==55) snap->pager2=tempo2[56];
		snap->emulation=tempo2[7];
	}	
		
	if(type) { // extended z80
		if(snap->type!=0) { // 128K, +2, +2A or 3+ snapshot

			while(!feof(fichero)) {
				retval=fread(byte_read,3,1,fichero);
				if(feof(fichero))
					break;
				longitud2=((int)byte_read[0])+256*((int)byte_read[1]);
				switch(byte_read[2]) {
				case 3:
					page=0;
					break;
				case 4:
					page=1;
					break;
				case 5:
					page=2;
					break;
				case 6:
					page=3;
					break;
				case 7:
					page=4;
					break;
				case 8:
					page=5;
					break;
				case 9:
					page=6;
					break;
				case 10:
					page=7;
					break;
				default:
					page=255;
					break;
				}
				printf("128k: Loading page %d of length %d\n",page,longitud2);
				if (page==255) //Discard the page
					{
					if(longitud2==0xFFFF) // uncompressed raw data
						retval=fread(memo,16384,1,fichero);
					else
						uncompress_z80(fichero,16384,memo);
					}	
				else
					{
					if(longitud2==0xFFFF) // uncompressed raw data
						retval=fread(snap->page[page],16384,1,fichero);
					else
						uncompress_z80(fichero,16384,snap->page[page]);
					}
			}

		} else { //48k snapshot
			while(!feof(fichero)) {
				retval=fread(byte_read,3,1,fichero);
				if(feof(fichero))
					break;
				longitud2=((int)byte_read[0])+256*((int)byte_read[1]);
				switch(byte_read[2]) {
				case 8:
					page=0;
					break;
				case 4:
					page=1;
					break;
				case 5:
					page=2;
					break;
				default:
					page=255;
					break;
				}
				printf("48k: Loading page %d of length %d\n",page,longitud2);
				if (page==255) //Discard the page. It should never happen
					{
					if(longitud2==0xFFFF) // uncompressed raw data
						retval=fread(memo,16384,1,fichero);
					else
						uncompress_z80(fichero,16384,memo);
					}	
				else
					{
					if(longitud2==0xFFFF) // uncompressed raw data
						retval=fread(snap->page[page],16384,1,fichero);
					else
						uncompress_z80(fichero,16384,snap->page[page]);
					}
			}
		}
	} else {

		printf("48k model: Loading page of old type z80\n");
		if(compressed) {
			// 48k compressed z80 loader
     
			// we uncompress first the data
      
			uncompress_z80(fichero,49152,memo);
      
			memcpy(snap->page[0],memo,16384);
			memcpy(snap->page[1],memo+16384,16384);
			memcpy(snap->page[2],memo+32768,16384);
     
		} else {
			// 48k uncompressed z80 loader
      
			retval=fread(snap->page[0],16384,1,fichero);
			retval=fread(snap->page[1],16384,1,fichero);
			retval=fread(snap->page[2],16384,1,fichero);
		}
		
	}

	load_snap(snap);
	fclose(fichero);
	free(memo);
	free(snap);
	return 0; // all right
}


int load_sna(char *filename) {

	unsigned char *tempo;
	unsigned char *tempo2;
	unsigned char type=0;
	FILE *fichero;
	struct z80snapshot *snap;
	unsigned char v1,v2;
	int addr,loop;

	tempo=(unsigned char *)malloc(49179);
	tempo2=(unsigned char *)malloc(16384*5+4);
	snap=(struct z80snapshot *)malloc(sizeof(struct z80snapshot));
	
	//Some inits
	for(loop=0;loop<16;loop++)
		snap->ay_regs[loop]=0;
	snap->ay_latch=0;
	snap->issue=3;
	snap->joystick=1; //kempston
	
	printf("Loading SNA file\n");
	
	fichero=fopen(filename,"rb");
	if(fichero==NULL) {
		free(tempo);
		free(tempo2);
		free(snap);
		return -1; // error
	}

	if (1!=fread(tempo,49179,1,fichero)) {
		free(tempo);
		free(tempo2);
		free(snap);
		return -1;
	}
	
	if (0==fread(tempo2,1,16384*5+4,fichero)) {
		printf("48K SNA\n");
		type=0;
	} else {
		printf("128K SNA\n");
		type=1;
	}
	
	snap->type=type;
	
	snap->I=tempo[0];
	snap->LL=tempo[1];
	snap->HH=tempo[2];
	snap->EE=tempo[3];
	snap->DD=tempo[4];
	snap->CC=tempo[5];
	snap->BB=tempo[6];
	snap->FF=tempo[7];
	snap->AA=tempo[8];
	
	snap->L=tempo[9];
	snap->H=tempo[10];
	snap->E=tempo[11];
	snap->D=tempo[12];
	snap->C=tempo[13];
	snap->B=tempo[14];
	
	snap->IY=((word)tempo[15])+256*((word)tempo[16]);
	snap->IX=((word)tempo[17])+256*((word)tempo[18]);
	
	if (tempo[19]&0x04) {
		snap->IFF1=1;
		snap->IFF2=1;
	} else {
		snap->IFF1=0;
		snap->IFF2=0;
	}
	
	snap->R=tempo[20];
	snap->F=tempo[21];
	snap->A=tempo[22];
	snap->SP=((word)tempo[23])+256*((word)tempo[24]);
	snap->Imode=tempo[25];
	snap->border=tempo[26];
	
	if (type==0) {	//48k
			
		v1=tempo[23];
		v2=tempo[24];
		addr=((int)v1)+256*((int)v2);
		if ((addr<16384)||(addr>=65534)) {
			free(tempo);
			free(tempo2);
			free(snap);
			printf("Error loading SNA file. Return address in ROM.\n");
			return -1;
		}
		addr-=16384;
		addr+=27;
		snap->PC=((word)tempo[addr])+256*((word)tempo[addr+1]);
		tempo[addr]=0;
		tempo[addr+1]=0;
		snap->SP+=2;
		snap->IFF1=snap->IFF2;
		memcpy(snap->page[0],tempo+27,16384);
		memcpy(snap->page[1],tempo+16411,16384);
		memcpy(snap->page[2],tempo+32795,16384);
	} else { 	//128k
		snap->PC=((word)tempo2[0])+256*((word)tempo2[1]);
		memcpy(snap->page[5],tempo+27,16384);
		memcpy(snap->page[2],tempo+16411,16384);
		v1=tempo2[2];
		snap->pager=v1;
		v1&=0x07;
		memcpy(snap->page[v1],tempo+32795,16384);
		addr=4;
		for (loop=0;loop<8;loop++) {
			if ((loop==2)||(loop==5)||(loop==((int)v1))) {
				continue;
			}
			memcpy(snap->page[loop],tempo2+addr,16384);
			addr+=16384;
		}
	}
	
	load_snap(snap);
	free(tempo);
	free(tempo2);
	free(snap);
	return 0;
	
}


void load_snap(struct z80snapshot *snap) {

  int bucle;

	printf("Loading SnapShot\n");
	ordenador.last_selected_poke_file[0]='\0';

  switch(snap->type) {
  case 0: // 48k
  	printf("Mode 48K\n");
    ordenador.mode128k=0; // 48K mode
    ordenador.issue=snap->issue;
    ResetComputer();
    break;
  case 1: // 128k
  	printf("Mode 128K\n");
    ordenador.mode128k=1; // 128k mode
    ordenador.issue=3;
	ordenador.videosystem=0;
    ResetComputer();
    printf("Pager: %X\n",snap->pager);
    Z80free_Out_fake(0x7FFD,snap->pager);
    break;
  case 2: // +2
  	printf("Mode +2\n");
    ordenador.mode128k=2; // +2 mode
    ordenador.issue=3;
	ordenador.videosystem=0;
    ResetComputer();
    printf("Pager: %X\n",snap->pager);
    Z80free_Out_fake(0x7FFD,snap->pager);
    break;	
  case 3: // +2A +3
  	printf("Mode +3\n");
    ordenador.mode128k=3; // +2A and +3 mode
    ordenador.issue=3;
	ordenador.videosystem=0;
    ResetComputer();
    printf("Pager: %X\n",snap->pager);
    Z80free_Out_fake(0x7FFD,snap->pager);
	printf("Pager2: %X\n",snap->pager2);
	Z80free_Out_fake (0x1FFD, snap->pager2);
    break;	
  default:
    break;
  }
  
  if (ordenador.ignore_z80_joy_conf==0) ordenador.joystick[0]=snap->joystick; //Only one Joystick in Z80 file

  procesador.Rm.br.A=snap->A;
  procesador.Rm.br.F=snap->F;
  procesador.Rm.br.B=snap->B;
  procesador.Rm.br.C=snap->C;
  procesador.Rm.br.D=snap->D;
  procesador.Rm.br.E=snap->E;
  procesador.Rm.br.H=snap->H;
  procesador.Rm.br.L=snap->L;
  printf("A:%x F:%x B:%x C:%x D:%x E:%x H:%x L:%x\n",snap->A,snap->F,snap->B,snap->C,snap->D,snap->E,snap->H,snap->L);
  procesador.Ra.br.A=snap->AA;
  procesador.Ra.br.F=snap->FF;
  procesador.Ra.br.B=snap->BB;
  procesador.Ra.br.C=snap->CC;
  procesador.Ra.br.D=snap->DD;
  procesador.Ra.br.E=snap->EE;
  procesador.Ra.br.H=snap->HH;
  procesador.Ra.br.L=snap->LL;
  printf("A:%x F:%x B:%x C:%x D:%x E:%x H:%x L:%x\n",snap->AA,snap->FF,snap->BB,snap->CC,snap->DD,snap->EE,snap->HH,snap->LL);
  procesador.Rm.wr.IX=snap->IX;
  procesador.Rm.wr.IY=snap->IY;
  procesador.Rm.wr.SP=snap->SP;
  procesador.PC=snap->PC;
  printf("IX:%x IY:%x SP:%x PC:%x\n",snap->IX,snap->IY,snap->SP,snap->PC);
  procesador.I=snap->I;
  procesador.R=snap->R;
  procesador.R2=snap->R;
  printf("I:%x R:%x\n",snap->I,snap->R);

  if(snap->IFF1) {
    procesador.IFF1=1;
  } else {
    procesador.IFF1=0;
  }
  if(snap->IFF2) {
    procesador.IFF2=1;
  } else {
    procesador.IFF2=0;
  }
	printf("IFF1:%x IFF2:%x\n",snap->IFF1,snap->IFF1);
  procesador.IM=snap->Imode;
  printf("IM:%x\n",snap->Imode);
  Z80free_Out_fake(0xFFFE,((snap->border&0x07)|0x10));

  switch(snap->type) {
  case 0: // 48K

    for(bucle=0;bucle<16384;bucle++) {
      ordenador.memoria[bucle+147456]=snap->page[0][bucle];
      ordenador.memoria[bucle+98304]=snap->page[1][bucle];
      ordenador.memoria[bucle+114688]=snap->page[2][bucle];
    }
    
    ordenador.ay_emul=((snap->emulation)&0x04)>>2;
    break;
  case 1: // 128K
  case 2: // +2
  case 3: // +2A/+3

    for(bucle=0;bucle<16384;bucle++) {
      ordenador.memoria[bucle+65536]=snap->page[0][bucle];
      ordenador.memoria[bucle+81920]=snap->page[1][bucle];
      ordenador.memoria[bucle+98304]=snap->page[2][bucle];
      ordenador.memoria[bucle+114688]=snap->page[3][bucle];
      ordenador.memoria[bucle+131072]=snap->page[4][bucle];
      ordenador.memoria[bucle+147456]=snap->page[5][bucle];
      ordenador.memoria[bucle+163840]=snap->page[6][bucle];
      ordenador.memoria[bucle+180224]=snap->page[7][bucle];
    }
    ordenador.ay_emul=1;
	ordenador.currah_active = 0;
    for(bucle=0;bucle<16;bucle++)
      ordenador.ay_registers[bucle]=snap->ay_regs[bucle];
    ordenador.ay_latch=snap->ay_latch;
    break;
  default:
    break;
  }
}

int extract_screen_sna (char *screen_memory, FILE * fichero)  {

	unsigned char *tempo;
	unsigned char *tempo2;
	unsigned char type=0;
	unsigned char v1;
	
	if(fichero==NULL) return -1; // error
	
	tempo=(unsigned char *)malloc(49179);
	tempo2=(unsigned char *)malloc(16384*5+4);
	
	if (1!=fread(tempo,49179,1,fichero)) {
		free(tempo);
		free(tempo2);
		return -1;
	}
	
	if (0==fread(tempo2,1,16384*5+4,fichero)) {
		printf("48K SNA\n");
		type=0;
	} else {
		printf("128K SNA\n");
		type=1;
	}
	
	if (type==0) {	//48k
	
		memcpy(screen_memory,tempo+27,6912);

	} else { 	//128k

		v1=tempo2[2];
		//printf("v1= %d\n",(int) v1);
		if ((v1&8)==0) memcpy(screen_memory,tempo+27,6912); //screen in bank 5
		else //Screen in bank 7
		{
			v1&=0x07;
				if (v1==7) memcpy(screen_memory,tempo+27+49152,6912); //screen in bank 7 paged-in
			else 
				memcpy(screen_memory,tempo2+4+16384*4,6912); //Screen in bank 7 not paged-in
		}
	}
	
	free(tempo);
	free(tempo2);
	return 0;
}

int extract_screen_z80 (char *screen_memory, FILE * fichero)  {

	unsigned char tempo[30],tempo2[57],type,compressed,pager, byte_read[3];
	unsigned char *memo;
	int longitud=0,longitud2,model_type;

	if(fichero==NULL) return -1; // error
	
	memo=(unsigned char *)malloc(49152);

	printf("Read Z80 file\n");

	printf("Read header (first 30 bytes)\n");
	fread(tempo,1,30,fichero);

	if((tempo[6]==0)&&(tempo[7]==0)) { // extended Z80
		printf("It's an extended Z80 file\n");
		type=1; // new type
		
		fread(tempo2,1,2,fichero); // read the length of the extension
 
		longitud=((int)tempo2[0])+256*((int)tempo2[1]);
		if(longitud>55) {
			printf("Not suported Z80 file\n");
			free(memo);
			return -3; // not a supported Z80 file
		}
		printf("Length: %d\n",longitud);
		fread(tempo2+2,1,longitud,fichero);

		if(longitud==23) // z80 ver 2.01
			switch(tempo2[4]) {
			case 0:
			case 1:
				model_type=0; // 48K
				break;
			case 3:
			case 4:
				model_type=1; // 128K
			break;
			case 12:
				model_type=2; // +2
				break;
			case 7: //+3
			case 13: //+2A
				model_type=3; // +2A and +3
				break;
			case 9:
				printf("z80: warning Pentagon 128k, using ZX 128k model emulation\n");
				model_type=1; // 128K
				break;
			default:
				printf("z80: model not suported: %d\n", tempo2[4]);
				free(memo);
				return -3; // not a supported Z80 file
			break;
			}
		else // z80 ver 3.0x
			switch(tempo2[4]) {
			case 0:
			case 1:
			case 3:
				model_type=0; // 48K
				break;
			case 4:
			case 5:
			case 6:
				model_type=1; // 128K
				break;
			case 12:
				model_type=2; // +2
				break;
			case 7: //+3
			case 13: //+2A
				model_type=3; // +2A and +3
				break;
			case 9:
				printf("z80: warning Pentagon 128k, using ZX 128k model emulation\n");
				model_type=1; // 128K
				break;
			default:
				printf("z80: model not suported: %d\n", tempo2[4]);
				free(memo);
				return -3; // not a supported Z80 file
				break;
			}      
	} else {
		printf("Old type z80\n");
		type=0; // old type
		model_type=0; // 48k
	}

	if(tempo[12]&32)
		compressed=1;
	else
		compressed=0;
		
	if (type) pager=tempo2[5];	
		

	if(type) { // extended z80
		if(model_type!=0) { // 128K, +2, + 2A or +3 snapshot 

			while(!feof(fichero)) {
				fread(byte_read,3,1,fichero);
				if(feof(fichero))
					break;
				longitud2=((int)byte_read[0])+256*((int)byte_read[1]);
				
				printf("128k model: Loading page %d of length %d\n",byte_read[2]-3,longitud2);
				
				if (((byte_read[2] == 8) && ((pager&8)==0))|| //Page 5 and no shadow screen
					((byte_read[2] == 10) && (pager&8))) //Page 7 and shadow screen
				{
					if(longitud2==0xFFFF) // uncompressed raw data
						fread(memo,6912,1,fichero);
					else
						uncompress_z80(fichero,6912,memo);
					memcpy(screen_memory, memo,6912);
					break;
				}
				if(longitud2==0xFFFF) longitud2 =16384; // uncompressed raw data					
				fread(memo,longitud2,1,fichero);
			}

		} else { //48k snapshot
			while(!feof(fichero)) {
				fread(byte_read,3,1,fichero);
				if(feof(fichero))
					break;
				longitud2=((int)byte_read[0])+256*((int)byte_read[1]);		
				printf("48k model: Loading page of length %d\n",longitud2);
				
				if (byte_read[2] == 8) //Page 0
				{
					if(longitud2==0xFFFF) // uncompressed raw data
						fread(memo,6912,1,fichero);
					else
						uncompress_z80(fichero,6912,memo);
					memcpy(screen_memory, memo,6912);
					break;
				}
				if(longitud2==0xFFFF) longitud2 =16384; // uncompressed raw data					
				fread(memo,longitud2,1,fichero);	
			}
		}
	} else { //Old type z80
	
		printf("48k model: Loading page of old type z80\n");
		if(compressed) {
			// 48k compressed z80 loader
     
			// we uncompress first the data
			uncompress_z80(fichero,6912,memo); //uncompress only the screen
      
			memcpy(screen_memory,memo,6912);
     
		} else {
			// 48k uncompressed z80 loader
      
			fread(screen_memory,6912,1,fichero);
		}
		
	}

	free(memo);
	return 0; // all right
}
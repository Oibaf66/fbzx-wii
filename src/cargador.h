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

struct z80snapshot {

  byte A,F,B,C,D,E,H,L,AA,FF,BB,CC,DD,EE,HH,LL,R,I,IFF1,IFF2,Imode,issue;
  word PC,IX,IY,SP;
  byte type; // bit 0/1: 48K/128K/+3
  byte border; // border color
  byte pager; // content of pagination register in 128K mode
  unsigned char page[8][16384];
  //unsigned int found_pages; // bit=1: page exists. bit=0: page don't exists.
  unsigned char ay_regs[16];
  unsigned char ay_latch;
  unsigned char joystick;

};

int save_z80(char *, int);
int load_z80(char *);
int load_sna(char *);
void load_snap(struct z80snapshot *);
void uncompress_z80(FILE *,int,unsigned char *);
int extract_screen_sna (char *screen, FILE * fichero);
int extract_screen_z80 (char *screen, FILE * fichero);

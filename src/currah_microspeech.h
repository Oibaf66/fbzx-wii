/* 
 * Copyright (C) 2014 Fabio Olimpieri
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
 
#ifndef H_CURRAH_MICROSPEECH
#define H_CURRAH_MICROSPEECH

#ifndef INTONATION_INCREASE 
	#define INTONATION_INCREASE 32 //Is this value rigth?
#endif


#define ALLOPHONES 64

extern int allophone_lenght[ALLOPHONES];
extern unsigned char *allophone_buffer[ALLOPHONES];

void currah_microspeech_init();
void currah_microspeech_reset();
void currah_microspeech_fini();

#endif

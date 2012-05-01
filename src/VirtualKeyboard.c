/*********************************************************************
 * Copyright (C) 2012,  Fabio Olimpieri
 * Copyright (C) 2009,  Simon Kagstrom
 *
 * Filename:      VirtualKeyboard.c
 * 
 * Description:   A virtual keyboard
 *
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
 ********************************************************************/
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "z80free/Z80free.h"
#include "computer.h"
#include "VirtualKeyboard.h"
#include "menu_sdl.h"


#define K(name, sdl_code) \
  { name, "SDLK_"name, sdl_code, 0 ,0,0}
#define N(name, key_name, sdl_code) \
  { name, "SDLK_"key_name, sdl_code, 0,0,0 }
#define KNL() \
  { NULL, NULL, 0, 0 ,0,0}


#define KEY_COLS 10
#define KEY_ROWS 5

extern struct computer ordenador; 

//TO DO Key_name and name are not necessary
static virtkey_t keys[KEY_COLS * KEY_ROWS] = {
  K("1",49),K("2",50), K("3",51), K("4",52), K("5",53), K("6",54), K("7",55), K("8",56), K("9",57), K("0",48),
  K("Q",113), K("W",119), K("E",101), K("R",114), K("T",116), K("Y",121), K("U",117), K("I",105), K("O",111), K("P",112),
  K("A",97), K("S",115), K("D",100), K("F",102), K("G",103), K("H",104), K("J",106), K("K",107), K("L",108),N("Enter","RETURN",13),
  N("Caps","LSHIFT",304),K("Z",122),K("X",120),K("C",99), K("V",118), K("B",98), K("N",110), K("M",109), N("Sym","LCTRL",306),N("Space","SPACE",32),
  N("Ext","TAB",9), K("None",0),N("Del","BACKSPACE",8),K(",",44),K(".",46),N("Fire","LALT",308), K("UP",273),K("DOWN",274), K("LEFT",276),K("RIGHT",275)};

void VirtualKeyboard_init(SDL_Surface *screen, TTF_Font *font)
{
	VirtualKeyboard.screen = screen;
	VirtualKeyboard.font = font;
	VirtualKeyboard.sel_x = 0;
	VirtualKeyboard.sel_y = 0;

	memset(VirtualKeyboard.buf, 0, sizeof(VirtualKeyboard.buf));
}

void draw()
{
	int y,x;
	int screen_w = VirtualKeyboard.screen->w;
	int screen_h = VirtualKeyboard.screen->h;
	int key_w = 54;
	int key_h = 36;
	int border_x = (screen_w - (key_w * KEY_COLS)) / 2;
	int border_y = (screen_h - (key_h * KEY_ROWS)) / 2;
	SDL_Rect bg_rect = {border_x, border_y,
			key_w * KEY_COLS, key_h * KEY_ROWS};

	SDL_FillRect(VirtualKeyboard.screen, &bg_rect,
			SDL_MapRGB(ordenador.screen->format, 0xff, 0xff, 0xff));

	for (y = 0; y < KEY_ROWS; y++ )
	{
		for (x = 0; x < KEY_COLS; x++ )
		{
			int which = y * KEY_COLS + x;
			virtkey_t key = keys[which];
			int r = 64, g = 64, b = 64;
			const char *what = key.name;

			/* Skip empty positions */
			if (key.name == NULL)
				continue;

			if ( key.is_done )
				r = 255;
			if ( (x == VirtualKeyboard.sel_x && y == VirtualKeyboard.sel_y))
				g = 200;

			menu_print_font(VirtualKeyboard.screen, r, g, b,
					x * key_w + border_x, y * key_h + border_y,
					what, 16);
		}
	}
}

void select_next_kb(int dx, int dy)
{
	int next_x = (VirtualKeyboard.sel_x + dx) % KEY_COLS;
	int next_y = (VirtualKeyboard.sel_y + dy) % KEY_ROWS;
	virtkey_t key;

	if (next_x < 0)
		next_x = KEY_COLS + next_x;
	if (next_y < 0)
		next_y = KEY_ROWS + next_y;
	VirtualKeyboard.sel_x = next_x;
	VirtualKeyboard.sel_y = next_y;

	key = keys[ next_y * KEY_COLS + next_x ];

	/* Skip the empty spots */
	if (key.name == NULL)
	{
		if (dy != 0) /* Look left */
			select_next_kb(-1, 0);
		else
			select_next_kb(dx, dy);
	}
}

struct virtkey *get_key_internal()
{
	while(1)
	{
		uint32_t k;

		draw();
		SDL_Flip(VirtualKeyboard.screen);

		k = menu_wait_key_press();

		if (k & KEY_UP)
			select_next_kb(0, -1);
		else if (k & KEY_DOWN)
			select_next_kb(0, 1);
		else if (k & KEY_LEFT)
			select_next_kb(-1, 0);
		else if (k & KEY_RIGHT)
			select_next_kb(1, 0);
		else if (k & KEY_ESCAPE)
			return NULL;
		else if (k & KEY_SELECT)
		{
			virtkey_t *key = &keys[ VirtualKeyboard.sel_y * KEY_COLS + VirtualKeyboard.sel_x ];
			
			if ((key->sdl_code == 304) && !keys[3 * KEY_COLS + 8 ].is_done)
			keys[3 * KEY_COLS + 0 ].is_done = !keys[3 * KEY_COLS + 0 ].is_done; //Caps Shit
			else if ((key->sdl_code == 306) && !keys[3 * KEY_COLS + 0 ].is_done) 
			keys[3 * KEY_COLS + 8 ].is_done = !keys[3 * KEY_COLS + 8 ].is_done; //Sym Shift
			else {
			key->caps_on = keys[3 * KEY_COLS + 0 ].is_done;
			key->sym_on = keys[3 * KEY_COLS + 8 ].is_done;
			return key;
			}
		}
	}

	return NULL;
}

struct virtkey* get_key()
{
	virtkey_t *key;
	SDL_Rect rect = {32, 128, FULL_DISPLAY_X-64, FULL_DISPLAY_Y-272};
	
	keys[3 * KEY_COLS + 0 ].is_done = 0; //Caps Shit
	keys[3 * KEY_COLS + 8 ].is_done = 0; //Sym Shift

	SDL_FillRect(VirtualKeyboard.screen, &rect, SDL_MapRGB(ordenador.screen->format, 0xff, 0xff, 0xff));
	
	key = get_key_internal();

	return key;
}


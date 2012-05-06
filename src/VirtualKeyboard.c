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
  { name, sdl_code, 0 ,0, 0}
#define KNL() \
  { NULL, 0, 0 ,0, 0}


#define KEY_COLS 10
#define KEY_ROWS 6

extern struct computer ordenador;
void clean_screen(); 

//TO DO Key_name and name are not necessary
static virtkey_t keys[KEY_COLS * KEY_ROWS] = {
  K(" 1",SDLK_1),K(" 2",SDLK_2), K(" 3",SDLK_3), K(" 4",SDLK_4), K(" 5",SDLK_5), K(" 6",SDLK_6), K(" 7",SDLK_7), K(" 8",SDLK_8), K(" 9",SDLK_9), K(" 0",SDLK_0),
  K(" Q",SDLK_q), K(" W",SDLK_w), K(" E",SDLK_e), K(" R",SDLK_r), K(" T",SDLK_t), K(" Y",SDLK_y), K(" U",SDLK_u), K(" I",SDLK_i), K(" O",SDLK_o), K(" P",SDLK_p),
  K(" A",SDLK_a), K(" S",SDLK_s), K(" D",SDLK_d), K(" F",SDLK_f), K(" G",SDLK_g), K(" H",SDLK_h), K(" J",SDLK_j), K(" K",SDLK_k), K(" L",SDLK_l),K("Enter",SDLK_RETURN),
  K("Caps",SDLK_LSHIFT),K(" Z",SDLK_z),K(" X",SDLK_x),K(" C",SDLK_c), K(" V",SDLK_v), K(" B",SDLK_b), K(" N",SDLK_n), K(" M",SDLK_m), K("Sym",SDLK_LCTRL),K("Space",SDLK_SPACE),
  K("Ext",SDLK_TAB),K(" ,",SDLK_COMMA),K(" .",SDLK_PERIOD), K(" ;",SDLK_SEMICOLON), K(" \"",SDLK_QUOTEDBL),KNL(),K(" Up",SDLK_UP),K("Down",SDLK_DOWN), K("Left",SDLK_LEFT),K("Right",SDLK_RIGHT),
  K("None",0),K("Done",1),K("Fire",SDLK_LALT),K("Del",SDLK_BACKSPACE),KNL(),KNL(),KNL(),KNL(),KNL(),KNL()};

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
	SDL_Rect rect = {32, 120, FULL_DISPLAY_X-64, FULL_DISPLAY_Y-250};
	
	keys[3 * KEY_COLS + 0 ].is_done = 0; //Caps Shit
	keys[3 * KEY_COLS + 8 ].is_done = 0; //Sym Shift

	SDL_FillRect(VirtualKeyboard.screen, &rect, SDL_MapRGB(ordenador.screen->format, 0xff, 0xff, 0xff));
	
	key = get_key_internal();
	
	clean_screen();

	return key;
}


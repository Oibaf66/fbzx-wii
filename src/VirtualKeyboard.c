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
#include<SDL/SDL_image.h>
#include <wiiuse/wpad.h>


#define K(name, sdl_code) \
  { name, sdl_code, 0 ,0, 0}
#define KNL() \
  { NULL, 0, 0 ,0, 0}


#define KEY_COLS 10
#define KEY_ROWS 5

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

static SDL_Surface *image_kbd, *image_sym, *image_caps,*image_kbd_small, *image_sym_small, *image_caps_small ;
static int vkb_is_init;

extern struct computer ordenador;
void clean_screen(); 

static virtkey_t keys[KEY_COLS * KEY_ROWS] = {
  K(" 1",SDLK_1),K(" 2",SDLK_2), K(" 3",SDLK_3), K(" 4",SDLK_4), K(" 5",SDLK_5), K(" 6",SDLK_6), K(" 7",SDLK_7), K(" 8",SDLK_8), K(" 9",SDLK_9), K(" 0",SDLK_0),
  K(" Q",SDLK_q), K(" W",SDLK_w), K(" E",SDLK_e), K(" R",SDLK_r), K(" T",SDLK_t), K(" Y",SDLK_y), K(" U",SDLK_u), K(" I",SDLK_i), K(" O",SDLK_o), K(" P",SDLK_p),
  K(" A",SDLK_a), K(" S",SDLK_s), K(" D",SDLK_d), K(" F",SDLK_f), K(" G",SDLK_g), K(" H",SDLK_h), K(" J",SDLK_j), K(" K",SDLK_k), K(" L",SDLK_l),K("Enter",SDLK_RETURN),
  K("Caps",SDLK_LSHIFT),K(" Z",SDLK_z),K(" X",SDLK_x),K(" C",SDLK_c), K(" V",SDLK_v), K(" B",SDLK_b), K(" N",SDLK_n), K(" M",SDLK_m), K("Sym",SDLK_LCTRL),K("Space",SDLK_SPACE),
  K("Ext",SDLK_TAB),K("Edit",SDLK_INSERT),K("Del",SDLK_BACKSPACE),K("None",0), K("Done",1), K("Fire",SDLK_LALT) ,K(" Up",SDLK_UP),K("Down",SDLK_DOWN), K("Left",SDLK_LEFT),K("Right",SDLK_RIGHT)};

void VirtualKeyboard_init(SDL_Surface *screen)
{
	VirtualKeyboard.screen = screen;
	VirtualKeyboard.sel_x = 64;
	VirtualKeyboard.sel_y = 100;
	vkb_is_init = -1;
	
	image_kbd=IMG_Load("/fbzx-wii/fbzx/Spectrum_keyboard.png");
	image_sym=IMG_Load("/fbzx-wii/fbzx/symbol_shift.png");
	image_caps=IMG_Load("/fbzx-wii/fbzx/caps_shift.png");
	
	image_kbd_small=IMG_Load("/fbzx-wii/fbzx/Spectrum_keyboard_small.png");
	image_sym_small=IMG_Load("/fbzx-wii/fbzx/symbol_shift_small.png");
	image_caps_small=IMG_Load("/fbzx-wii/fbzx/caps_shift_small.png");
	

	if (image_kbd == NULL) {printf("Impossible to load keyboard image\n"); return;}
	if (image_sym == NULL) {printf("Impossible to load symbol shift image\n"); return;}
	if (image_caps == NULL) {printf("Impossible to load caps shift image\n"); return;}
	
	if (image_kbd_small == NULL) {printf("Impossible to load keyboard small image\n"); return;}
	if (image_sym_small == NULL) {printf("Impossible to load symbol small shift image\n"); return;}
	if (image_caps_small == NULL) {printf("Impossible to load caps shift small image\n"); return;}
	

	memset(VirtualKeyboard.buf, 0, sizeof(VirtualKeyboard.buf));
	vkb_is_init = 1;
}

void draw()
{
	
	SDL_Rect dst_rect = {VirtualKeyboard.sel_x/RATIO, VirtualKeyboard.sel_y/RATIO, 0, 0};
	
	if (RATIO == 1) SDL_BlitSurface(image_kbd, NULL, ordenador.screen, &dst_rect); 
				else SDL_BlitSurface(image_kbd_small, NULL, ordenador.screen, &dst_rect);
	
	dst_rect.x = (VirtualKeyboard.sel_x+10)/RATIO;
	dst_rect.y = (VirtualKeyboard.sel_y+200)/RATIO; 
	if (keys[3 * KEY_COLS + 0 ].is_on) 
		{if (RATIO == 1) SDL_BlitSurface(image_caps, NULL, ordenador.screen, &dst_rect); 
					else SDL_BlitSurface(image_caps_small, NULL, ordenador.screen, &dst_rect);}
	
	
	dst_rect.x = (VirtualKeyboard.sel_x+402)/RATIO;
	dst_rect.y = (VirtualKeyboard.sel_y+200)/RATIO; 
	if (keys[3 * KEY_COLS + 8 ].is_on) 
		{if (RATIO == 1) SDL_BlitSurface(image_sym, NULL, ordenador.screen, &dst_rect);
				else SDL_BlitSurface(image_sym_small, NULL, ordenador.screen, &dst_rect);} 
}


struct virtkey *get_key_internal()
{
	while(1)
	{
		uint32_t k;
		int x,y,xm, ym, i=0;
		int key_w = 50/RATIO;
		int key_h = 64/RATIO;
		int border_x = VirtualKeyboard.sel_x/RATIO;
		int border_y = VirtualKeyboard.sel_y/RATIO;

		draw();
		SDL_Flip(VirtualKeyboard.screen);
		
		SDL_ShowCursor(SDL_ENABLE);

		k = menu_wait_key_press();
		
		SDL_ShowCursor(SDL_DISABLE);

		if (k & KEY_ESCAPE) return NULL;
		else if (k & KEY_SELECT)
		{
			
			SDL_GetMouseState(&xm, &ym);
			x = (xm-border_x);
			y = (ym-border_y);
			if ((x<0)||(x>= KEY_COLS*key_w)) continue;
			if ((y<0)||(y>= KEY_ROWS*key_h)) continue;
			
			i = y/key_h*KEY_COLS + x/key_w;
			
			
			WPAD_Rumble(0, 1);
			SDL_Delay(50);
			WPAD_Rumble(0, 0);
			
			virtkey_t *key = &keys[i];
			
			if ((key->sdl_code == 304) && !keys[3 * KEY_COLS + 8 ].is_on)
			keys[3 * KEY_COLS + 0 ].is_on = !keys[3 * KEY_COLS + 0 ].is_on; //Caps Shit
			else if ((key->sdl_code == 306) && !keys[3 * KEY_COLS + 0 ].is_on) 
			keys[3 * KEY_COLS + 8 ].is_on = !keys[3 * KEY_COLS + 8 ].is_on; //Sym Shift
			else {
			key->caps_on = keys[3 * KEY_COLS + 0 ].is_on;
			key->sym_on = keys[3 * KEY_COLS + 8 ].is_on;
			return key;
			}
		}
	}

	return NULL;
}

struct virtkey* get_key()
{
	virtkey_t *key;
	
	if (vkb_is_init != 1) return NULL;
	
	keys[3 * KEY_COLS + 0 ].is_on = 0; //Caps Shit
	keys[3 * KEY_COLS + 8 ].is_on = 0; //Sym Shift
	
	key = get_key_internal();

	return key;
}


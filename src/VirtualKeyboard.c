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
#include "emulator.h"
#include<SDL/SDL_image.h>

#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif

#ifdef HW_DOL
#include <ogc/pad.h>
#endif

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

static SDL_Surface *image_kbd, *image_sym, *image_caps,*image_kbd_small, *image_sym_small, *image_caps_small, *tmp_surface ;
static int vkb_is_init;
static int key_code;
VirtualKeyboard_struct VirtualKeyboard; 

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
	char *image_path;
	
	image_path=myfile("fbzx/Spectrum_keyboard.png");
	tmp_surface=IMG_Load(image_path);
	free(image_path);
	if (tmp_surface == NULL) {printf("Impossible to load keyboard image\n"); return;}
	image_kbd=SDL_DisplayFormat(tmp_surface);
	SDL_FreeSurface (tmp_surface);
	
	image_path=myfile("fbzx/symbol_shift.png");
	tmp_surface=IMG_Load(image_path);
	free(image_path);
	if (tmp_surface == NULL) {printf("Impossible to load symbol shift image\n"); return;}
	image_sym=SDL_DisplayFormat(tmp_surface);
	SDL_FreeSurface (tmp_surface);
	
	image_path=myfile("fbzx/caps_shift.png");
	tmp_surface=IMG_Load(image_path);
	free(image_path);
	if (tmp_surface == NULL) {printf("Impossible to load caps shift image\n"); return;}
	image_caps=SDL_DisplayFormat(tmp_surface);
	SDL_FreeSurface (tmp_surface);
	
	image_path=myfile("fbzx/Spectrum_keyboard_small.png");
	tmp_surface=IMG_Load(image_path);
	free(image_path);
	if (tmp_surface == NULL) {printf("Impossible to load keyboard small image\n"); return;}
	image_kbd_small=SDL_DisplayFormat(tmp_surface);
	SDL_FreeSurface (tmp_surface);
	
	image_path=myfile("fbzx/symbol_shift_small.png");
	tmp_surface=IMG_Load(image_path);
	free(image_path);
	if (tmp_surface == NULL) {printf("Impossible to load symbol shift small image\n"); return;}
	image_sym_small=SDL_DisplayFormat(tmp_surface);
	SDL_FreeSurface (tmp_surface);
	
	image_path=myfile("fbzx/caps_shift_small.png");
	tmp_surface=IMG_Load(image_path);
	free(image_path);
	if (tmp_surface == NULL) {printf("Impossible to load caps shift small image\n"); return;}
	image_caps_small=SDL_DisplayFormat(tmp_surface);
	SDL_FreeSurface (tmp_surface);


	memset(VirtualKeyboard.buf, 0, sizeof(VirtualKeyboard.buf));
	vkb_is_init = 1;
}

void VirtualKeyboard_fini()
{
	if (vkb_is_init != 1) return;
	
	SDL_FreeSurface (image_kbd);
	SDL_FreeSurface (image_sym);
	SDL_FreeSurface (image_caps);
	SDL_FreeSurface (image_kbd_small);
	SDL_FreeSurface (image_sym_small);
	SDL_FreeSurface (image_caps_small);
	vkb_is_init = -1;
	ordenador.vk_is_active=0;
}

void draw_vk()
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

		draw_vk();
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
			
			#ifdef HW_RVL
			if (ordenador.vk_rumble) WPAD_Rumble(0, 1);
			SDL_Delay(90);
			if (ordenador.vk_rumble) WPAD_Rumble(0, 0);
			#endif
			
			#ifdef HW_DOL
			if (ordenador.vk_rumble) PAD_ControlMotor(0,PAD_MOTOR_RUMBLE);
			SDL_Delay(90);
			if (ordenador.vk_rumble) PAD_ControlMotor(0,PAD_MOTOR_STOP);
			#endif
			
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

void virtkey_ir_run(void)
{ 
	int x,y,xm, ym, i=0;
	int key_w = 50/RATIO;
	int key_h = 64/RATIO;
	int border_x = VirtualKeyboard.sel_x/RATIO;
	int border_y = VirtualKeyboard.sel_y/RATIO;
	int key_sel = 0;
	SDL_Joystick *joy;
	static int joy_bottons_last[5];
	static char countdown_rumble=0;
	
	#ifdef HW_RVL
	if (countdown_rumble > 0) {countdown_rumble--; if ((countdown_rumble==0)&&(ordenador.vk_rumble)) WPAD_Rumble(0, 0);}
	#endif
	
	#ifdef HW_DOL
	if (countdown_rumble > 0) {countdown_rumble--; if ((countdown_rumble==0)&&(ordenador.vk_rumble)) PAD_ControlMotor(0,PAD_MOTOR_STOP);}
	#endif
	
	joy = ordenador.joystick_sdl[0];
				
	if ((SDL_JoystickGetButton(joy, 0) && !joy_bottons_last[0]) ||      /* A */
		(SDL_JoystickGetButton(joy, 3) && !joy_bottons_last[1]) ||  /* 2 */
		(SDL_JoystickGetButton(joy, 9) && !joy_bottons_last[2]) ||  /* CA */
		(SDL_JoystickGetButton(joy, 10) && !joy_bottons_last[3])   /* CB */
	#ifndef GEKKO //Win	
		||((SDL_GetMouseState(NULL, NULL)&SDL_BUTTON(1))&& !joy_bottons_last[4])//mouse left button
	#endif	
		) key_sel = KEY_SELECT;
	
	
	if ((!SDL_JoystickGetButton(joy, 0) && joy_bottons_last[0]) ||      /* A */
		(!SDL_JoystickGetButton(joy, 3) && joy_bottons_last[1]) ||  /* 2 */
		(!SDL_JoystickGetButton(joy, 9) && joy_bottons_last[2]) ||  /* CA */
		(!SDL_JoystickGetButton(joy, 10) && joy_bottons_last[3])   /* CB */
	#ifndef GEKKO //Win	
		||(!(SDL_GetMouseState(NULL, NULL)&SDL_BUTTON(1)) && joy_bottons_last[4]) //mouse left button
	#endif	
		) key_sel = KEY_DESELECT;
	
	
				
	joy_bottons_last[0]=SDL_JoystickGetButton(joy, 0) ;   /* A */
	joy_bottons_last[1]	=SDL_JoystickGetButton(joy, 3) ;  /* 2 */
	joy_bottons_last[2]	=SDL_JoystickGetButton(joy, 9) ;  /* CA */
	joy_bottons_last[3]	=SDL_JoystickGetButton(joy, 10) ; /* CB */
	#ifndef GEKKO //Win
	joy_bottons_last[4] =SDL_GetMouseState(NULL, NULL)&SDL_BUTTON(1); //mouse left button
	#endif
	
	if (key_sel==KEY_SELECT)
	{	
		SDL_GetMouseState(&xm, &ym);
		x = (xm-border_x);
		y = (ym-border_y);
		if ((x>0)&&(x< KEY_COLS*key_w)&&(y>0)&&(y< KEY_ROWS*key_h)) 
		{
			#ifdef HW_RVL
			if (ordenador.vk_rumble) WPAD_Rumble(0, 1);
			#endif
			
			#ifdef HW_DOL
			if (ordenador.vk_rumble) PAD_ControlMotor(0,PAD_MOTOR_RUMBLE);
			#endif
			
			countdown_rumble=5;
			
			i = y/key_h*KEY_COLS + x/key_w;
			
			virtkey_t *key = &keys[i];
				
			if ((key->sdl_code == SDLK_LSHIFT) && !keys[3 * KEY_COLS + 8 ].is_on)
				keys[3 * KEY_COLS + 0 ].is_on = !keys[3 * KEY_COLS + 0 ].is_on; //Caps Shit
			else if ((key->sdl_code == SDLK_LCTRL) && !keys[3 * KEY_COLS + 0 ].is_on) 
				keys[3 * KEY_COLS + 8 ].is_on = !keys[3 * KEY_COLS + 8 ].is_on; //Sym Shift
			else {
				key->caps_on = keys[3 * KEY_COLS + 0 ].is_on;
				key->sym_on = keys[3 * KEY_COLS + 8 ].is_on;
			}
	
			key_code = key->sdl_code;
	
			if ((key_code!=SDLK_LSHIFT)&&(key_code!=SDLK_LCTRL))
			{
				if 	(key->caps_on) joybutton_matrix[0][SDLK_LSHIFT]=1;
				else if (key->sym_on) joybutton_matrix[0][SDLK_LCTRL]=1;
				joybutton_matrix[0][key_code]=1;
			}
			
			printf ("Push Event: keycode %d\n", key_code);
			
			SDL_ShowCursor(SDL_DISABLE);
			draw_vk();
			SDL_ShowCursor(SDL_ENABLE);
		}
	}
	
	if (key_sel==KEY_DESELECT)
	{	
		joybutton_matrix[0][key_code]=0;
		joybutton_matrix[0][SDLK_LSHIFT]=0;
		joybutton_matrix[0][SDLK_LCTRL]=0; 
		
		if ((key_code!=SDLK_LSHIFT)&&(key_code!=SDLK_LCTRL))
		{
			keys[3 * KEY_COLS + 0 ].is_on = 0; //Caps Shit
			keys[3 * KEY_COLS + 8 ].is_on = 0; //Sym Shift
		}
		
		SDL_ShowCursor(SDL_DISABLE);
		draw_vk();
		SDL_ShowCursor(SDL_ENABLE);
		
	key_code=0;
	}
}

void virtkey_ir_activate(void)	
{
	ordenador.vk_is_active=1;
	VirtualKeyboard.sel_x = 64;
	VirtualKeyboard.sel_y = 90;
	draw_vk();
	SDL_ShowCursor(SDL_ENABLE);
}
		
void virtkey_ir_deactivate(void)
{
	ordenador.vk_is_active=0;
	SDL_ShowCursor(SDL_DISABLE);
	
	joybutton_matrix[0][key_code]=0;
	joybutton_matrix[0][SDLK_LSHIFT]=0;
	joybutton_matrix[0][SDLK_LCTRL]=0; 	
	keys[3 * KEY_COLS + 0 ].is_on = 0; //Caps Shit
	keys[3 * KEY_COLS + 8 ].is_on = 0; //Sym Shift
	key_code=0;
	clean_screen();
}

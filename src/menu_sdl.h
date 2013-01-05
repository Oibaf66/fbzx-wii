/*********************************************************************
 * Copyright (C) 2012,  Fabio Olimpieri
 * Copyright (C) 2009,  Simon Kagstrom
 *
 * Filename:      menu_sdl.h
 * 
 * Description:   Code for menus (originally for Mophun)
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
 
#ifndef __MENU_H__
#define __MENU_H__

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define KEY_UP         1
#define KEY_DOWN       2
#define KEY_LEFT       4
#define KEY_RIGHT      8
#define KEY_SELECT    16
#define KEY_ESCAPE    32
#define KEY_PAGEDOWN  64
#define KEY_PAGEUP   128
#define KEY_HELP     256

int FULL_DISPLAY_X; //640
int FULL_DISPLAY_Y; //480
int RATIO;


void menu_print_font(SDL_Surface *screen, int r, int g, int b, int x, int y, const char *msg, int font_size);

void print_font(SDL_Surface *screen, int r, int g, int b, int x, int y, const char *msg, int font_size);

/* Various option selects */
int menu_select_title(const char *title, const char **pp_msgs, int *p_submenus);
int menu_select(const char **pp_msgs, int *p_submenus);
const char *menu_select_file(const char *dir_path,const char *selected_file, int draw_scr);


uint32_t menu_wait_key_press(int vk);

void msgKill(SDL_Rect *rc);
int msgInfo(char *text, int duration, SDL_Rect *rc);

int msgYesNo(char *text, int def,int x, int y);

void font_init();

void menu_init(SDL_Surface *screen);

int menu_is_inited(void);

int ext_matches(const char *name, const char *ext);

int ask_value_sdl(int *final_value,int y_coord,int max_value);

int ask_filename_sdl(char *nombre_final,int y_coord,char *extension, char *path, char *name);

#endif /* !__MENU_H__ */

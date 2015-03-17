/* 
 * Copyright (C) 2015 Fabio Olimpieri
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
 
#define MAX_RZX_BROWSER_ITEM 63


typedef struct
{
   unsigned int position;
   unsigned int frames_count;
} RZX_browser;


extern RZX_browser rzx_browser_list[MAX_RZX_BROWSER_ITEM+1];
extern char extracted_rzx_file[MAX_PATH_LENGTH];
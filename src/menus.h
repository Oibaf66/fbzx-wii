/*
 * Copyright 2003-2011 (C) Raster Software Vigo (Sergio Costas)
 * This file is part of FBZX
 *
 * FBZX is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * FBZX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

struct fichero {
	char nombre[256]; // filename (for files and directories)
	char nombrepath[2049]; // filename with path
	int tipo; // file type (0=file, 1=directory, 2=parent directory)
	struct fichero *next;
};

enum LOAD_FILE_TYPES {FILETYPE_Z80, FILETYPE_TAP_TZX, FILETYPE_MDR, FILETYPE_SCR};

void clean_screen();
void help_menu();
void load_z80file();
char *select_file(char *,enum LOAD_FILE_TYPES);
struct fichero *read_directory(char *,enum LOAD_FILE_TYPES);
unsigned int wait_key();
void print_files(struct fichero *,int,int);
void delete_filelist(struct fichero *);
void select_tapfile();
void save_z80file();
void settings_menu();
void snapshots_menu();
void taps_menu();
void create_tapfile();
void select_mdrfile();
void create_mdrfile();
void microdrive_menu();
void keyboard_menu();
void load_scrfile();
int ask_filename(char *nombre,int y_coord,char *extension);
void create_scrfile();
void do_poke();
int ask_value(int *final_value,int y_coord,int max_value);
void tools_menu();
int launch_menu(unsigned int key_pressed);

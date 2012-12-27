/*********************************************************************
 *
 * Copyright (C) 2012,  Fabio Olimpieri
 *
 * Filename:      menu_sdl.c
 * Author:        Fabio Olimpieri <fabio.olimpieri@tin.it>
 * Description:   a SDL Gui
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
 *
 ********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "menu_sdl.h"
#include "emulator.h"
#include "VirtualKeyboard.h"
#include "tape.h"
#include "menus.h"
#include "emulator.h"
#include "cargador.h"
#include "characters.h"

#define ID_BUTTON_OFFSET 0
#define ID_AXIS_OFFSET 32

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

#define MAX_POKE 20
#define MAX_TRAINER 50

extern int countdown;
void clean_screen();


static const char *main_menu_messages[] = {
		/*00*/		"Tape",
		/*01*/		"^|Insert|Load|Play|Stop|Rewind|Create|Delete",
		/*02*/		"Snapshot",
		/*03*/		"^|Load|Save|Delete",
		/*04*/		"#1---------------------------------------------",
		/*05*/		"Wiimote configuration",
		/*06*/		"^|Wiimote1|Wiimote2",
		/*07*/		"Emulation settings",
		/*08*/		"Screen settings",
		/*09*/		"Audio settings",
		/*10*/		"Config files",
		/*11*/		"Microdrive",
		/*12*/		"Tools",
		/*13*/		"Reset",
		/*14*/		"Quit",
		NULL
};

static const char *emulation_messages[] = {
		/*00*/		"Emulated machine",
		/*01*/		"^|48k_2|48K_3|128k|+2|+2A/+3|128K_Sp|NTSC",
		/*02*/		"Frame rate",
		/*03*/		"^|100%|50%|33%|25%|20%",
		/*04*/		"Tap instant load",
		/*05*/		"^|on|off",
		/*06*/		"Turbo mode",
		/*07*/		"^|off|auto|fast|ultrafast",
		/*08*/		"Precision",
		/*09*/		"^|on|off",	
		NULL
};

static const char *audio_messages[] = {
		/*00*/		"Volume",
		/*01*/		"^|0|1|2|3|4|5|6|7|max",
		/*02*/		"  ",
		/*03*/		"AY-3-8912 Emulation",
		/*04*/		"^|on|off",	
		/*05*/		"  ",
		/*06*/		"Audio mode",
		/*07*/		"^|mono|ABC|ACB|BAC",	

		NULL
};

static const char *screen_messages[] = {
		/*00*/		"Double scan",
		/*01*/		"^|on|off",
		/*02*/		"  ",
		/*03*/		"TV mode",
		/*04*/		"^|Color|B&W",
		/*05*/		"  ",
		/*06*/		"Buffer resolution",
		/*07*/		"^|640X480|320X240",
		/*08*/		"  ",
		/*09		"576p video mode",*/
		/*10		"^|on|off",*/
		NULL
};

static const  char *input_messages[] = {
		/*00*/		"Joystick type",
		/*01*/		"^|Cursor|Kempston|Sinclair1|Sinclair2",
		/*02*/		"Bind key to Wiimote",
		/*03*/		"^|A|B|1|2|-",
		/*04*/		"Bind key to Nunchuk",
		/*05*/		"^|Z|C",
		/*06*/		"Bind key to Classic",
		/*07*/		"^|a|b|x|y|L|R|Zl|Zr|-",
		/*08*/		"Bind key to Pad",
		/*09*/		"^|Up|Down|Left|Right",
		/*10*/		"Use Joypad as Joystick",
		/*11*/		"^|On|Off",
		/*12*/		"Rumble",
		/*13*/		"^|On|Off",
		NULL,
};

static const char *microdrive_messages[] = {
		/*00*/		"Microdrive",
		/*01*/		"^|Select|Create|Delete",
		/*02*/		"  ",
		/*03*/		"Interface I",
		/*04*/		"^|on|off",
		/*05*/		"  ",
		/*06*/		"Write protection",
		/*07*/		"^|on|off",
		NULL
};

static const char *tools_messages[] = {
		/*00*/		"Show keyboard",
		/*01*/		"  ",
		/*02*/		"Save SCR",
		/*03*/		"  ",
		/*04*/		"Load SCR",
		/*05*/		"  ",
		/*06*/		"Insert poke",
		/*07*/		"  ",
		/*08*/		"Load poke file",
		/*09*/		"  ",
		/*10*/		"Port",
		/*11*/		"^|sd|usb|smb|ftp",
		/*12*/		"  ",
		/*13*/		"Help",
		NULL
};

static const char *help_messages[] = {
		/*00*/		"#2HOME enters the menu system, where arrow keys",
		/*01*/		"#2and nunchuck are used to navigate up and down.",
		/*02*/		"#2You can bind keyboard keys to the wiimote",
		/*03*/		"#2buttons in the 'Wiimote' menu and",
		/*04*/		"#2change emulation options in the Settings menu.",
		/*05*/		"#2 ",
		/*06*/		"#2The easiest way to play a game is to load",
		/*07*/		"#2a snapshot (.z80 and .sna files).",
		/*08*/		"#2You can also insert a tape file (.tap and .tzx)",
		/*09*/		"#2and then load the file in the tape menu.",
		/*10*/		"#2 ",
		/*11*/		"#2More information is available on the wiki:",
		/*12*/		"#2   http://wiibrew.org/wiki/FBZX_Wii",
		/*13*/		"#2 ",
		/*14*/		"OK",
		NULL,
};

static const char *confs_messages[] = {
		/*00*/		"General configurations",
		/*01*/		"^|Load|Save|Delete",
		/*02*/		"  ",
		/*03*/		"Game configurations",
		/*04*/		"^|Load|Save|Delete",
		/*05*/		"  ",
		/*06*/		"Load confs automatically",
		/*07*/		"^|on|off",
		NULL
};

void maybe_load_conf(const char *filename)
{
	const char *dir = path_confs;
	char *ptr;
	char db[256];
	char fb[81];
	
	if (filename==NULL) return;	

	if (strrchr(filename, '/'))
		strncpy(fb, strrchr(filename, '/') + 1, 80);
	else
		strncpy(fb, filename, 80);
		
	//remove the extension
	ptr = strrchr (fb, '.');
		if (ptr) *ptr = 0;	

	snprintf(db, 255, "%s/%s.conf", dir, fb);
	if (!load_config(&ordenador,db)) msgInfo("Configurations loaded",2000,NULL)	;
	
}

static void insert_tape()
{
	unsigned char char_id[11];
	int retorno, retval;

	// Maybe should go after the first control??
	if(ordenador.tap_file!=NULL)
		rewind_tape(ordenador.tap_file,1);

	ordenador.tape_current_bit=0;
	ordenador.tape_current_mode=TAP_TRASH;
	
	const char *filename = menu_select_file(path_taps, ordenador.current_tap, 0);
	
	if (filename==NULL) // Aborted
		return; 
	
	if (strstr(filename, "None") != NULL)
	{
			ordenador.current_tap[0] = '\0';
			free((void *)filename);
			return;
	}
	
	if (!(ext_matches(filename, ".tap")|ext_matches(filename, ".TAP")|ext_matches(filename, ".tzx")|
	ext_matches(filename, ".TZX"))) {free((void *)filename); return;}
	
	
	if(ordenador.tap_file!=NULL) {
		fclose(ordenador.tap_file);
	}

	if (!strncmp(filename,"smb:",4)) ordenador.tap_file=fopen(filename,"r"); //tinysmb does not work with r+
	else ordenador.tap_file=fopen(filename,"r+"); // read and write
	
	ordenador.tape_write = 1; // by default, can record
	
	if(ordenador.tap_file==NULL)
		retorno=-1;
	else
		retorno=0;
	
	switch(retorno) {
	case 0: // all right
	strcpy(ordenador.current_tap,filename);
	strcpy(ordenador.last_selected_file,filename);
	if (ordenador.autoconf) maybe_load_conf(filename);
	break;
	case -1:
		msgInfo("Error: Can't load that file",3000,NULL);
		ordenador.current_tap[0]=0;
		free((void *)filename);
		return;
	break;
	}
	
	free((void *)filename);
	
	retval=fread(char_id,10,1,ordenador.tap_file); // read the (maybe) TZX header
	if((!strncmp(char_id,"ZXTape!",7)) && (char_id[7]==0x1A)&&(char_id[8]==1)) {
		ordenador.tape_file_type = TAP_TZX;
		rewind_tape(ordenador.tap_file,1);
	} else {
		ordenador.tape_file_type = TAP_TAP;
		rewind_tape(ordenador.tap_file,1);
	}
}

static void delete_tape()
{
	const char *filename = menu_select_file(path_taps, NULL, -1);
	
	if (filename==NULL) // Aborted
		return; 
	
	if ((ext_matches(filename, ".tap")|ext_matches(filename, ".TAP")|ext_matches(filename, ".tzx")|
	ext_matches(filename, ".TZX"))
	&& (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138/RATIO, FULL_DISPLAY_Y /2-48/RATIO))) unlink(filename);
	
	free((void *)filename);
}

void create_tapfile_sdl() {

	unsigned char *videomem;
	int ancho,retorno;
	unsigned char nombre2[1024];

	videomem=screen->pixels;
	ancho=screen->w;

	clean_screen();

	print_string(videomem,"Choose a name for the TAP file",-1,32,14,0,ancho);
	print_string(videomem,"(up to 30 characters)",-1,52,14,0,ancho);

	print_string(videomem,"TAP file will be saved in:",-1,112,12,0,ancho);
	print_string(videomem,path_taps,0,132,12,0,ancho);


	retorno=ask_filename_sdl(nombre2,82,"tap",path_taps,NULL);

	if(retorno==2) // abort
		return;

	if(ordenador.tap_file!=NULL)
		fclose(ordenador.tap_file);
	
	ordenador.tap_file=fopen(nombre2,"r"); // test if it exists
	if(ordenador.tap_file==NULL)
		retorno=0;
	else
		retorno=-1;
	
	if(!retorno) {
		ordenador.tap_file=fopen(nombre2,"a+"); // create for read and write
		if(ordenador.tap_file==NULL)
			retorno=-2;
		else
			retorno=0;
	}
	ordenador.tape_write=1; // allow to write
	strcpy(ordenador.current_tap,nombre2);
	ordenador.tape_file_type = TAP_TAP;
	switch(retorno) {
	case 0:
	break;
	case -1:
		msgInfo("File already exists",3000,NULL);
		ordenador.current_tap[0]=0;
	break;
	case -2:
		msgInfo("Can't create file",3000,NULL);
		ordenador.current_tap[0]=0;
	break;
	}
	clean_screen();
}	

static void manage_tape(int which)
{
	switch (which)
	{
	case 0: //Insert
		insert_tape();
		break;
	case 1: //Emulate load ""
		countdown=8;
		if (ordenador.mode128k==4) //Spanish 128k
			{
			ordenador.keyboard_buffer[0][8]= SDLK_l;		
			ordenador.keyboard_buffer[1][8]= 0;
			ordenador.keyboard_buffer[0][7]= SDLK_o;		
			ordenador.keyboard_buffer[1][7]= 0;
			ordenador.keyboard_buffer[0][6]= SDLK_a;		
			ordenador.keyboard_buffer[1][6]= 0;
			ordenador.keyboard_buffer[0][5]= SDLK_d;		
			ordenador.keyboard_buffer[1][5]= 0;
			ordenador.keyboard_buffer[0][4]= SDLK_p;		//"	
			ordenador.keyboard_buffer[1][4]= SDLK_LCTRL;
			ordenador.keyboard_buffer[0][3]= SDLK_p;		//"	
			ordenador.keyboard_buffer[1][3]= SDLK_LCTRL;
			ordenador.keyboard_buffer[0][2]= SDLK_RETURN;	// Return
			ordenador.keyboard_buffer[1][2]= 0;
			ordenador.keyboard_buffer[0][1]= SDLK_F6;		//F6 - play
			ordenador.keyboard_buffer[1][1]= 0;
			ordenador.kbd_buffer_pointer=8;
			}
			else
			{
			ordenador.keyboard_buffer[0][5]= SDLK_j;		//Load
			ordenador.keyboard_buffer[1][5]= 0;
			ordenador.keyboard_buffer[0][4]= SDLK_p;		//"
			ordenador.keyboard_buffer[1][4]= SDLK_LCTRL;
			ordenador.keyboard_buffer[0][3]= SDLK_p;		//"
			ordenador.keyboard_buffer[1][3]= SDLK_LCTRL;
			ordenador.keyboard_buffer[0][2]= SDLK_RETURN;	// Return
			ordenador.keyboard_buffer[1][2]= 0;
			ordenador.keyboard_buffer[0][1]= SDLK_F6;		//F6
			ordenador.keyboard_buffer[1][1]= 0;
			ordenador.kbd_buffer_pointer=5;
			}
		
		break;
	case 2: //Play
		if ((ordenador.tape_fast_load == 0) || (ordenador.tape_file_type==TAP_TZX))
				ordenador.pause = 0;
		break;
	case 3: //Stop
		if ((ordenador.tape_fast_load == 0) || (ordenador.tape_file_type==TAP_TZX))
				ordenador.pause = 1;
		break;
	case 4: //Rewind
			ordenador.pause=1;
			if(ordenador.tap_file!=NULL) {
				ordenador.tape_current_mode=TAP_TRASH;
				rewind_tape(ordenador.tap_file,1);		
			}
			msgInfo("Tape rewinded",3000,NULL);
		break;
	case 5: //Create
		// Create tape 
		create_tapfile_sdl();
		break;	
	case 6: //Delete
		delete_tape();
		break;	
	default:
		break;
	}
}

static unsigned int get_machine_model(void)
{
	if (ordenador.videosystem == 0)
	return  (ordenador.mode128k + (ordenador.issue==3));
	else return (6); 
}

static void set_machine_model(int which)
{
	switch (which)
	{
	case 0: //48k issue2
			ordenador.issue=2;
			ordenador.mode128k=0;
			ordenador.ay_emul=0;
			ordenador.videosystem =0;
		break;
	case 1: //48k issue3
			ordenador.issue=3;
			ordenador.mode128k=0;
			ordenador.ay_emul=0;
			ordenador.videosystem =0;
		break;
	case 2: //128k
			ordenador.issue=3;
			ordenador.mode128k=1;
			ordenador.ay_emul=1;
			ordenador.videosystem =0;
		break;
	case 3: //Amstrad +2
			ordenador.issue=3;
			ordenador.mode128k=2;
			ordenador.ay_emul=1;
			ordenador.videosystem =0;
		break;
	case 4: //Amstrad +2A/+3
			ordenador.issue=3;
			ordenador.mode128k=3;
			ordenador.ay_emul=1;
			ordenador.mdr_active=0;
			ordenador.videosystem =0;
		break;
	case 5: //128K Spanish
			ordenador.issue=3;
			ordenador.mode128k=4;
			ordenador.ay_emul=1;
			ordenador.videosystem =0;
		break;
	case 6: //48k ntsc
			ordenador.issue=3;
			ordenador.mode128k=0;
			ordenador.ay_emul=0;
			ordenador.videosystem =1;
		break;
	}
}

static void emulation_settings(void)
{
	unsigned int submenus[5],submenus_old[5];
	int opt, i;
	unsigned char old_mode, old_videosystem;
	
	memset(submenus, 0, sizeof(submenus));
	
	submenus[0] = get_machine_model();
	submenus[1] = jump_frames;
	submenus[2] = !ordenador.tape_fast_load;
	submenus[3] = ordenador.turbo;
	submenus[4] = !ordenador.precision;

	
	for (i=0; i<5; i++) submenus_old[i] = submenus[i];
	old_mode=ordenador.mode128k;
	old_videosystem = ordenador.videosystem;
	
	opt = menu_select_title("Emulation settings menu",
			emulation_messages, submenus);
	if (opt < 0)
		return;
	
	set_machine_model(submenus[0]);
	if ((old_mode!=ordenador.mode128k)||(old_videosystem!=ordenador.videosystem)) ResetComputer(); 
	
	jump_frames = submenus[1];
	ordenador.tape_fast_load = !submenus[2];
	ordenador.turbo = submenus[3];
	
	curr_frames=0;
	if (submenus[3] != submenus_old[3])
	{
	switch(ordenador.turbo)
	{
	case 1: //auto
		ordenador.precision =0;
	case 0: //off
		update_frequency(0); //set machine frequency
		jump_frames=0;
		ordenador.turbo_state=0;
		break;
	case 2:	//fast	
		update_frequency(10000000);
		jump_frames=4;
		ordenador.precision =0;
		ordenador.turbo_state=2;
		break;
	case 3:	//ultra fast
		update_frequency(15000000);
		jump_frames=24;
		ordenador.precision =0;
		ordenador.turbo_state=3;
		break;
	default:
		break;	
	}
	}
	
	if (submenus[4] != submenus_old[4])
	{
	ordenador.precision = !submenus[4];
		if (ordenador.precision)
		{
		update_frequency(0);
		jump_frames=0;
		ordenador.turbo =0;
		ordenador.turbo_state=0;
		}
	}
}

static void audio_settings(void)
{
	unsigned int submenus[3];
	int opt;

	
	memset(submenus, 0, sizeof(submenus));
	
	
	submenus[0] = ordenador.volume/2;
	submenus[1] = !ordenador.ay_emul;
	submenus[2] = ordenador.audio_mode;
	
	
	opt = menu_select_title("Audio settings menu",
			audio_messages, submenus);
	if (opt < 0)
		return;

	
	ordenador.volume = submenus[0]*2; 
	ordenador.ay_emul = !submenus[1];
	ordenador.audio_mode = submenus[2];
	
	
}

static void save_load_general_configurations(int);

static void screen_settings(void)
{
	unsigned int submenus[4],submenus_old[4];
	int opt, i;
	
	memset(submenus, 0, sizeof(submenus));
	
	submenus[0] = !ordenador.dblscan;
	submenus[1] = ordenador.bw;
	submenus[2] = ordenador.zaurus_mini?1:0;
	submenus[3] = !ordenador.progressive;
	
	for (i=0; i<4; i++) submenus_old[i] = submenus[i];
	
	
	opt = menu_select_title("Screen settings menu",
			screen_messages, submenus);
	if (opt < 0)
		return;
	
	ordenador.dblscan = !submenus[0];
	ordenador.bw = submenus[1]; 
	ordenador.progressive = !submenus[3];
	
	if (submenus[0] != submenus_old[0]) update_npixels();
	
	if (submenus[1]!=submenus_old[1]) computer_set_palete();
	
	if (submenus[2] != submenus_old[2])
	{
		if (submenus[2]==0) {ordenador.zaurus_mini = 0; ordenador.text_mini=0;}
		else {ordenador.zaurus_mini = 3; ordenador.text_mini=1;}
		update_npixels();
	    restart_video();
	}
	if (submenus[3] != submenus_old[3])
	{
		switch (set_video_mode()) 
		{
		case 1:
		msgInfo("Necessary component cable",3000,NULL);
		ordenador.progressive = 0;
		break;
		case 2:
		msgInfo("Only avalaible from 576i PAL",3000,NULL);
		ordenador.progressive = 0;
		break;
		}
	}
}

static void setup_joystick(int joy, unsigned int sdl_key, int joy_key)
{
	int loop;
	
	//Cancel the previous assignement - it is not possible to assign a same sdl_key to 2 joybuttons
	for (loop=0; loop<22; loop++)
	 if (ordenador.joybuttonkey[joy][loop] == sdl_key) ordenador.joybuttonkey[joy][loop] =0;
	
	ordenador.joybuttonkey[joy][joy_key] = sdl_key;
	
}

static void input_options(int joy)
{
	const unsigned int wiimote_to_sdl[] = {0, 1, 2, 3, 4};
	const unsigned int nunchuk_to_sdl[] = {7, 8};
	const unsigned int classic_to_sdl[] = {9, 10, 11, 12, 13, 14, 15, 16, 17};
	const unsigned int pad_to_sdl[] = {18, 19, 20, 21};
	int joy_key = 1;
	unsigned int sdl_key;
	unsigned int submenus[7];
	int opt;
	
	struct virtkey *virtualkey;

	memset(submenus, 0, sizeof(submenus));
	
	submenus[0] = ordenador.joystick[joy];
	submenus[5] = !ordenador.joypad_as_joystick[joy];
	submenus[6] = !ordenador.rumble[joy];
	
	opt = menu_select_title("Input menu",
			input_messages, submenus);
	if (opt < 0)
		return;
	
	ordenador.joystick[joy] = submenus[0];
	ordenador.joypad_as_joystick[joy] = !submenus[5];
	ordenador.rumble[joy] = !submenus[6];
	
	if (opt == 0 || opt == 10|| opt == 12)
		return;
	
	virtualkey = get_key();
	if (virtualkey == NULL)
		return;
	sdl_key = virtualkey->sdl_code;
	
	if (virtualkey->sdl_code==1) //"Done" selected
		{if (virtualkey->caps_on)  sdl_key = 304; //Caps Shit
			else if (virtualkey->sym_on)  sdl_key = 306; //Sym Shit
			else return; } 
	
	switch(opt)
		{
		case 2: // wiimote 
			joy_key = wiimote_to_sdl[submenus[1]]; break;
		case 4: // nunchuk
			joy_key = nunchuk_to_sdl[submenus[2]]; break;
		case 6: // classic
			joy_key = classic_to_sdl[submenus[3]]; break;
		case 8: // pad
			joy_key = pad_to_sdl[submenus[4]]; break;
		default:
			break;
		}
		
	setup_joystick(joy, sdl_key, joy_key);
	
}

static void select_mdr()
{
	int retorno, retval;

	const char *filename = menu_select_file(path_mdrs, ordenador.mdr_current_mdr, 0);
	
	if (filename==NULL) // Aborted
		return; 
		
	if (strstr(filename, "None") != NULL)
	{
			ordenador.mdr_current_mdr[0] = '\0';
			free((void *)filename);
			return;
	}	
	
	if (!(ext_matches(filename, ".mdr")|ext_matches(filename, ".MDR"))) {free((void *)filename); return;}
	
	ordenador.mdr_file=fopen(filename,"rb"); // read
	if(ordenador.mdr_file==NULL)
		retorno=-1;
	else {
		retorno=0;
		retval=fread(ordenador.mdr_cartridge,137923,1,ordenador.mdr_file); // read the cartridge in memory
		ordenador.mdr_modified=0; // not modified
		fclose(ordenador.mdr_file);
		ordenador.mdr_tapehead=0;
	}

	strcpy(ordenador.mdr_current_mdr,filename);

	free((void *)filename);

	switch(retorno) {
	case 0: // all right
		break;
	default:
		ordenador.mdr_current_mdr[0]=0;
		msgInfo("Error: Can't load that file",3000,NULL);
	break;
	}
}

static void delete_mdr()
{
	const char *filename = menu_select_file(path_mdrs, NULL, -1);
	
	if (filename==NULL) // Aborted
		return; 
	
	if ((ext_matches(filename, ".mdr")|ext_matches(filename, ".MDR"))
	&& (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138/RATIO, FULL_DISPLAY_Y /2-48/RATIO))) unlink(filename);
	
	free((void *)filename);
}

void create_mdrfile_sdl() {

	unsigned char *videomem;
	int ancho,retorno,bucle,retval;
	unsigned char nombre2[1024];

	videomem=screen->pixels;
	ancho=screen->w;

	clean_screen();

	print_string(videomem,"Choose a name for the MDR file",-1,32,14,0,ancho);
	print_string(videomem,"(up to 30 characters)",-1,52,14,0,ancho);

	print_string(videomem,"MDR file will be saved in:",-1,112,12,0,ancho);
	print_string(videomem,path_mdrs,0,132,12,0,ancho);

	retorno=ask_filename_sdl(nombre2,82,"mdr",path_mdrs, NULL);

	if(retorno==2) // abort
		return;

	ordenador.mdr_file=fopen(nombre2,"r"); // test if it exists
	if(ordenador.mdr_file==NULL)
		retorno=0;
	else
		retorno=-1;
	
	if(!retorno) {
		ordenador.mdr_file=fopen(nombre2,"wb"); // create for write
		if(ordenador.mdr_file==NULL)
			retorno=-2;
		else {
			for(bucle=0;bucle<137921;bucle++)
				ordenador.mdr_cartridge[bucle]=0xFF; // erase cartridge
			ordenador.mdr_cartridge[137922]=0;
			retval=fwrite(ordenador.mdr_cartridge,137923,1,ordenador.mdr_file); // save cartridge
			fclose(ordenador.mdr_file);
			ordenador.mdr_file=NULL;
			ordenador.mdr_modified=0;
			retorno=0;
		}
	}	
	strcpy(ordenador.mdr_current_mdr,nombre2);	
	switch(retorno) {
	case 0:
	break;
	case -1:
		msgInfo("File already exists",3000,NULL);
		ordenador.mdr_current_mdr[0]=0;
	break;
	case -2:
		msgInfo("Can't create file",3000,NULL);
		ordenador.mdr_current_mdr[0]=0;
	break;
	}
	clean_screen();
}

static void microdrive()
{
	
	unsigned int submenus[3], submenus_old[3];
	int opt,retval ;

	memset(submenus, 0, sizeof(submenus));
	
	submenus[1] = !ordenador.mdr_active;
	submenus[2] = !ordenador.mdr_cartridge[137922];
	
	submenus_old[1] = submenus[1];
	submenus_old[2] = submenus[2];
	
	opt = menu_select_title("Microdrive menu",
			microdrive_messages, submenus);
	if (opt < 0)
		return;
	
	ordenador.mdr_active = !submenus[1];
	
	
	if (submenus[1]!=submenus_old[1]) ResetComputer();
	if (submenus[2]!=submenus_old[2]) 
		{if(ordenador.mdr_cartridge[137922])
				ordenador.mdr_cartridge[137922]=0;
			else
				ordenador.mdr_cartridge[137922]=1;
			ordenador.mdr_file=fopen(ordenador.mdr_current_mdr,"wb"); // create for write
			if(ordenador.mdr_file!=NULL) {				
				retval=fwrite(ordenador.mdr_cartridge,137923,1,ordenador.mdr_file); // save cartridge
				fclose(ordenador.mdr_file);
				ordenador.mdr_file=NULL;
				ordenador.mdr_modified=0;
			}			
		}
	
	if (opt==0)
		switch (submenus[0]) 
		{
		case 0: // Select microdrive 
			select_mdr();
			break;
		case 1: // Create microdrive file
			// Create microdrive file ;
			create_mdrfile_sdl();
			//msgInfo("Not yet implemented",3000,NULL);
			break;
		case 2: // Delete microdrive file
			delete_mdr();
			break;
		default:
			break;
		}		
}
void show_keyboard_layout() {
	
	FILE *fichero;
	int bucle1,bucle2,retval;
	unsigned char *buffer,valor;

	buffer=screen->pixels;
	
	fichero=myfopen("fbzx/keymap.bmp","r");
	if (fichero==NULL) {
		msgInfo("Keymap picture not found",3000,NULL);
		return;
	}
	
	for (bucle1=0;bucle1<344;bucle1++)
		for(bucle2=0;bucle2<640;bucle2++) {
			retval=fscanf(fichero,"%c",&valor);
			paint_one_pixel((unsigned char *)(colors+valor),buffer);
			buffer+=ordenador.bpp;
			}
	SDL_Flip(ordenador.screen);
	menu_wait_key_press();
}
	
static void load_scr()
{
	int retorno,loop;
	unsigned char value;
	FILE *fichero;
	unsigned char paleta_tmp[64];


	const char *filename = menu_select_file(path_scr, NULL, -1);
	
	if (filename==NULL) // Aborted
		return; 
	
	if (!(ext_matches(filename, ".scr")|ext_matches(filename, ".SCR"))) {free((void *)filename); return;}
	
	ordenador.osd_text[0]=0;
	fichero=fopen(filename,"rb");
	retorno=0;
	if (!fichero) {
		retorno=-1;
	} else {
		for(loop=0;loop<6912;loop++) {
			if (1==fread(&value,1,1,fichero)) {
				*(ordenador.block1 + 0x04000 + loop) = value;
			} else {
				retorno=-1;
				break;
			}
		}
		if (1==fread(paleta_tmp,64,1,fichero)) {
			memcpy(ordenador.ulaplus_palete,paleta_tmp,64);
			ordenador.ulaplus=1;
		} else {
			ordenador.ulaplus=0;
		}
		fclose(fichero);
	}

	switch(retorno) {
	case 0: // all right
		strcpy(ordenador.last_selected_file,filename);
		break;
	case -1:
		msgInfo("Error: Can't load that file",3000,NULL);
		break;
	default:
		break;	
	}
	
	free((void *)filename);
	
}

static void save_scr()
{
	const char *dir = path_scr;
	const char *tape = ordenador.last_selected_file;
	char *ptr;
	FILE *fichero;
	char db[256];
	char fb[81];
	int retorno,retval;

	// Name (for saves) - TO CHECK
	if (tape && strrchr(tape, '/'))
		strncpy(fb, strrchr(tape, '/') + 1, 80);
	else
		strcpy(fb, "unknown");
		
	//remove the extension
	ptr = strrchr (fb, '.');
		if (ptr) *ptr = 0;
					
	//If file is taken from FTP, saves file on SD card
	if (ordenador.port==3)  
		{
			int length;
			strcpy(path_scr,getenv("HOME"));
			length=strlen(path_scr);
			if ((length>0)&&(path_scr[length-1]!='/')) strcat(path_scr,"/");
			strcat(path_scr,"scr");
			dir=path_scr;
		}
	
	// Save SCR file		
	snprintf(db, 255, "%s/%s.scr", dir, fb);
	
	if (ordenador.port==3) strcpy(path_scr,"ftp:");
		
	fichero=fopen(db,"r");
	
	if(fichero!=NULL)
	{	
		fclose(fichero);
		if (!msgYesNo("Overwrite the exiting file?", 0, FULL_DISPLAY_X /2-160/RATIO, FULL_DISPLAY_Y /2-48/RATIO))
			return; // file already exists
	}
	
	fichero=fopen(db,"wb"); // create for write
		
	if(fichero==NULL)
		retorno=-1;
	else {
		retval=fwrite(ordenador.block1+0x04000,6912,1,fichero); // save screen
		if (ordenador.ulaplus!=0) {
			retval=fwrite(ordenador.ulaplus_palete,64,1,fichero); // save ULAPlus palete
			}
		fclose(fichero);
		retorno=0;
		}

	switch(retorno) {
	case 0:
		msgInfo("SCR saved",3000,NULL);
		break;
	case -1:
		msgInfo("Can't create file",3000,NULL);
	break;
	default:
	break;
	}
}

static void set_port(int which)
{
	int length;
	
	switch (which)
	{
	case 0: //PORT_SD
		strcpy(path_snaps,getenv("HOME"));
		length=strlen(path_snaps);
		if ((length>0)&&(path_snaps[length-1]!='/')) strcat(path_snaps,"/");
		strcpy(path_taps,path_snaps);
		strcpy(path_poke,path_snaps);
		strcpy(path_scr,path_snaps);
		strcat(path_snaps,"snapshots");
		strcat(path_taps,"tapes");
		strcat(path_poke,"poke");
		strcat(path_scr,"scr");
		ordenador.port = which;
		break;
	case 1: //PORT_USB
		if (usbismount) {
			strcpy(path_snaps,"usb:/");
			strcpy(path_taps,"usb:/");
			strcpy(path_poke,"usb:/");
			strcpy(path_scr,"usb:/");
			ordenador.port = which;}
		else
			msgInfo("USB is not mounted",3000,NULL);
		break;
	case 2: //PORT_SMB
		if (smbismount) {
			strcpy(path_snaps,"smb:/");
			strcpy(path_taps,"smb:/");
			strcpy(path_poke,"smb:/");
			strcpy(path_scr,"smb:/");
			ordenador.port = which;}
		else
			msgInfo("SMB is not mounted",3000,NULL);
		break;
	case 3: //PORT_FTP
		if (ftpismount) {
			strcpy(path_snaps,"ftp:");
			strcpy(path_taps,"ftp:");
			strcpy(path_poke,"ftp:");
			strcpy(path_scr,"ftp:");
			ordenador.port = which;}
		else
			msgInfo("FTP is not connected",3000,NULL);
		break;	
	default:
		break;		
	}	
}

// shows the POKE menu

void do_poke_sdl() {

	unsigned char *videomem,string[80];
	int ancho,retorno,address,old_value,new_value;

	videomem=screen->pixels;
	ancho=screen->w;

	clean_screen();

	while(1) {
		print_string(videomem,"Type address to POKE",-1,32,15,0,ancho);

		retorno=ask_value_sdl(&address,84,65535);

		clean_screen();

		if (retorno==2) {
			return;
		}

		if ((address<16384) && ((ordenador.mode128k != 3) || (1 != (ordenador.mport2 & 0x01)))) {
			print_string(videomem,"That address is ROM memory.",-1,13,15,0,ancho);
			continue;
		}

		switch (address & 0x0C000) {
		case 0x0000:
			old_value= (*(ordenador.block0 + address));
		break;

		case 0x4000:
			old_value= (*(ordenador.block1 + address));
		break;

		case 0x8000:
			old_value= (*(ordenador.block2 + address));
		break;

		case 0xC000:
			old_value= (*(ordenador.block3 + address));
		break;
		default:
			old_value=0;
		break;
		}

		print_string(videomem,"Type new value to POKE",-1,32,15,0,ancho);
		sprintf(string,"Address: %d; old value: %d\n",address,old_value);
		print_string(videomem,string,-1,130,14,0,ancho);

		retorno=ask_value_sdl(&new_value,84,255);

		clean_screen();

		if (retorno==2) {
			continue;
		}

		switch (address & 0x0C000) {
		case 0x0000:
			(*(ordenador.block0 + address))=new_value;
		break;

		case 0x4000:
			(*(ordenador.block1 + address))=new_value;
		break;

		case 0x8000:
			(*(ordenador.block2 + address))=new_value;
		break;

		case 0xC000:
			(*(ordenador.block3 + address))=new_value;
		break;
		default:
		break;
		}

		sprintf(string,"Set address %d from %d to %d\n",address,old_value,new_value);
		print_string(videomem,string,-1,130,14,0,ancho);

	}
}


int parse_poke (const char *filename)
{
	static unsigned char old_poke[MAX_TRAINER][MAX_POKE]; //Max 19 Pokes per trainer and max 50 trainer
	FILE* fpoke;
	unsigned char title[128], flag, newfile, restore, old_mport1;
	int bank, address, value, original_value, ritorno,y,k, trainer, poke;
	SDL_Rect src, banner;

	src.x=0;
	src.y=30/RATIO;
	src.w=FULL_DISPLAY_X;
	src.h=FULL_DISPLAY_Y-60/RATIO;

	banner.x=0;
	banner.y=30/RATIO;
	banner.w=FULL_DISPLAY_X;
	banner.h=20/RATIO;

	y=60/RATIO;

	if (strcmp(ordenador.last_selected_poke_file,filename)) newfile=1; else newfile=0;

	trainer=0;

	fpoke = fopen(filename,"r");

	if (fpoke==NULL) 
	{
		msgInfo("Can not access the file",3000,NULL);	
		return (0);
	}

	clean_screen();

	SDL_FillRect(screen, &src, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));

	print_font(screen, 0x0, 0x0, 0x0,0, 30/RATIO, "Press 1 to deselect, 2 to select", 16);

	ritorno=0;
	do
	{
		if (trainer==MAX_TRAINER) {ritorno=2;break;}
	
		poke=1;
		restore=0;
		if (!fgets(title,128,fpoke)) {ritorno=1;break;}
		if (title[0]=='Y') break;
		if (title[0]!='N') {ritorno=1;break;}

		if (strlen(title)>1) title[strlen(title)-2]='\0'; //cancel new line and line feed

		if (y>420/RATIO) {SDL_FillRect(screen, &src, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));y=40/RATIO;}
	
		if (newfile) print_font(screen, 0x80, 0x80, 0x80,0, y, title+1, 16);
		else {if (old_poke[trainer][0]==0) print_font(screen, 0xd0, 0, 0,0, y, title+1, 16); //In row 0 information on trainer selection 
				else print_font(screen, 0, 0xd0, 0,0, y, title+1, 16);}

		SDL_Flip(screen);
		k=0;

		while (!((k & KEY_ESCAPE)||(k & KEY_SELECT)))
		{k = menu_wait_key_press();}
	
		banner.y=y;
	
		SDL_FillRect(screen, &banner, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
	
		if (k & KEY_SELECT) 
		{
			print_font(screen, 0, 0x40, 0,0, y, title+1, 16);
			old_poke[trainer][0]=1;
		}
		else 
		{
			if ((!newfile)&&(old_poke[trainer][0]==1)) restore=1;
			print_font(screen, 0x80, 0, 0,0, y, title+1, 16);
			old_poke[trainer][0]=0;
		}	
	
		SDL_Flip(screen);

		y+=20/RATIO;
					
		do 
		{
			if (poke==MAX_POKE) old_poke[trainer][0]=0; //in order not to restore the old_value
		
			fscanf(fpoke, "%1s %d %d %d %d", &flag, &bank, &address, &value, &original_value);
			if (((flag!='M')&&(flag!='Z'))||(bank>8)||(bank<0)||(address>0xFFFF)||(address<0x4000)||(value>256)||(value<0)||(original_value>255)||(original_value<0)) {ritorno=1;break;}
			if (feof(fpoke)) {ritorno=1;break;}
			if ((!(bank&0x8))&&(ordenador.mode128k)) //128k,+2,+3,SP
			{
				old_mport1 = ordenador.mport1;
				ordenador.mport1 = (unsigned char) (bank&0x7);
				set_memory_pointers ();	// set the pointers
			
				if (poke<MAX_POKE)
				{
					if(newfile)
						{if (original_value) old_poke[trainer][poke]=(unsigned char) original_value; else old_poke[trainer][poke]= Z80free_Rd_fake ((word) address);}
					if (restore) value = (int) old_poke[trainer][poke]; 
				}
				//if ((value == 256) && (k & KEY_SELECT)) {value = choice_value(); Z80free_Wr_fake ((word)address, (unsigned char) value);} TODO
				if (((value < 256) && (k & KEY_SELECT))||(restore)) Z80free_Wr_fake ((word)address, (unsigned char) value);
				ordenador.mport1 = old_mport1;
				set_memory_pointers ();	// set the pointers
			}
			else 
			{
				if (poke<MAX_POKE)
				{
					if(newfile)
						{if (original_value) old_poke[trainer][poke]=(unsigned char) original_value; else old_poke[trainer][poke]= Z80free_Rd_fake ((word) address);}
					if (restore) value = (int) old_poke[trainer][poke];
				}
				//if ((value == 256) && (k & KEY_SELECT)) {value = choice_value(); Z80free_Wr_fake ((word)address, (unsigned char) value);} TODO
				if (((value < 256) && (k & KEY_SELECT))||(restore)) Z80free_Wr_fake ((word)address, (unsigned char) value);
			}
			poke++;
		}
		while (flag!='Z');
	
		trainer++;
	
		if (!fgets(title,128,fpoke)) {ritorno=1;break;} //line feed reading
	
	} 
	while (ritorno==0);

	k=0;

	while (!(k & KEY_ESCAPE)&&(ritorno==0))
	{k = menu_wait_key_press();}

	fclose(fpoke);
	if (ritorno==0) strcpy(ordenador.last_selected_poke_file,filename);		
	return (ritorno);
}

void load_poke_file()
{
	const char *dir = path_poke;
	int ritorno;
	ritorno=0;
	
	const char *filename = menu_select_file(dir, NULL,-1);
		
	if (!filename) return;

	if (ext_matches(filename, ".pok")|ext_matches(filename, ".POK"))
	ritorno = parse_poke(filename);
	
	switch(ritorno)
	{
	case 1:
	msgInfo("Not compatible file",3000,NULL);
	break;
	case 2:
	msgInfo("Too many trainers",3000,NULL);
	break;
	}
				
	free((void*)filename);
	
}

static void help(void)
{
	menu_select_title("FBZX-WII help",
			help_messages, NULL);
}

static void tools()
{
	int opt ;
	int submenus[1];

	memset(submenus, 0, sizeof(submenus));

	submenus[0] = ordenador.port;
	
	opt = menu_select_title("Tools menu",
			tools_messages, submenus);
	if (opt < 0)
		return;
		
	set_port(submenus[0]);
	
	switch(opt)
		{
		case 0: // Show keyboard 
			if (ordenador.zaurus_mini == 0) show_keyboard_layout(); else msgInfo("No picture available in 320X240 resolution",3000,NULL);
			break;
		case 2: // Save SCR
			save_scr();
			break;
		case 4: // Load SCR 
			load_scr();
			break;
		case 6: // Insert poke
			do_poke_sdl();
			break;
		case 8: // Load poke file
			load_poke_file();
			break;
		case 13:
			help();
			break;
		default:
			break;
		}	
}


void virtual_keyboard(void)
{
	int key_code;
	
	virtkey_t *key =get_key();  
	if (key) {key_code = key->sdl_code;} else return;
	
	ordenador.kbd_buffer_pointer=1;
	countdown=8;
	ordenador.keyboard_buffer[0][1]= key_code;
	if 	(key->caps_on) ordenador.keyboard_buffer[1][1]= SDLK_LSHIFT; 
	else if (key->sym_on) ordenador.keyboard_buffer[1][1]= SDLK_LCTRL; 
	else ordenador.keyboard_buffer[1][1]= 0;
	
	printf ("Push Event: keycode %d\n", key_code);

}	

static void save_load_snapshot(int which)
{
	const char *dir = path_snaps;
	const char *tape = ordenador.last_selected_file;
	char *ptr;
	char db[256];
	char fb[81];
	int retorno;

	// Name (for saves) - TO CHECK
	if (tape && strrchr(tape, '/'))
		strncpy(fb, strrchr(tape, '/') + 1, 80);
	else
		strcpy(fb, "unknown");
		
	//remove the extension
	ptr = strrchr (fb, '.');
		if (ptr) *ptr = 0;	

	switch(which)
	{
	case 2:
	case 0: // Load or delete file
	{
		const char *filename = menu_select_file(dir, NULL,-1);

		if (!filename)
			return;

		if (ext_matches(filename, ".z80")|ext_matches(filename, ".Z80")|
		ext_matches(filename, ".sna")|ext_matches(filename, ".SNA"))
		{
			if (which == 0) // Load snapshot file
			{
				retorno=load_z80((char *)filename);

				switch(retorno) {
				case 0: // all right
				strcpy(ordenador.last_selected_file,filename);
				if (ordenador.autoconf) maybe_load_conf(filename);
				break;
				case -1:
				msgInfo("Error: Can't load that file",3000,NULL);
				break;
				case -2:
				case -3:
				msgInfo("Error: unsuported snap file",3000,NULL);
				break;
				}
			}
			else // Delete snashot file
				if (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138/RATIO, FULL_DISPLAY_Y /2-48/RATIO)) unlink(filename);
		}	
		free((void*)filename);
	} break;
	case 1: // Save snapshot file
		if (ordenador.port==3)  //If file is taken from FTP, saves file on SD card
		{
			int length;
			strcpy(path_snaps,getenv("HOME"));
			length=strlen(path_snaps);
			if ((length>0)&&(path_snaps[length-1]!='/')) strcat(path_snaps,"/");
			strcat(path_snaps,"snapshots");
			dir=path_snaps;
		}
		snprintf(db, 255, "%s/%s.z80", dir, fb);
		if (ordenador.port==3) strcpy(path_snaps,"ftp:"); 
		retorno=save_z80(db,0);
		switch(retorno) 
			{
			case 0: //OK
				msgInfo("Snapshot saved",3000,NULL);
				break;
			case -1:
				if (msgYesNo("Overwrite the exiting file?", 0, FULL_DISPLAY_X /2-160/RATIO, FULL_DISPLAY_Y /2-48/RATIO))
				{
					save_z80(db,1); //force overwrite
					msgInfo("Snapshot saved",3000,NULL);
				}
				break;
			case -2:
				msgInfo("Can't create file",3000,NULL);
				break;
			}
		break;
	default:
		break;
	}
}

static void save_load_game_configurations(int which)
{
	const char *dir = path_confs;
	const char *tape = ordenador.last_selected_file;
	char *ptr;
	char db[256];
	char fb[81];
	int retorno;
	
	switch(which)
	{
	case 2:
	case 0: // Load or delete file
	{
		const char *filename = menu_select_file(dir, NULL,-1);
		
		if (!filename)
			return;

		if (ext_matches(filename, ".conf")|ext_matches(filename, ".CONF"))
		{
			if (which == 0) // Load config file
			{
				if (!load_config(&ordenador,db)) msgInfo("Game confs loaded",3000,NULL);
				break;
			}
			else // Delete config file
				if (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138/RATIO, FULL_DISPLAY_Y /2-48/RATIO)) unlink(filename);
		}	
		free((void*)filename);
	} break;
	case 1: // Save configuration file
		
		// Name (for game config saves) - TO CHECK
		if (tape && strrchr(tape, '/'))
			strncpy(fb, strrchr(tape, '/') + 1, 80);
		else
			{
			msgInfo("No file selected",3000,NULL);
			return;
			}
		
		//remove the extension
		ptr = strrchr (fb, '.');
		
		if (ptr) *ptr = 0;	
	
		snprintf(db, 255, "%s/%s.conf", dir, fb);
	
		retorno=save_config_game(&ordenador,db,0);
		
		switch(retorno) 
			{
			case 0: //OK
				msgInfo("Game confs saved",3000,NULL);
				break;
			case -1:
				if (msgYesNo("Overwrite the exiting file?", 0, FULL_DISPLAY_X /2-160/RATIO, FULL_DISPLAY_Y /2-48/RATIO))
				{
					save_config_game(&ordenador,db,1); //force overwrite
					msgInfo("Game confs saved",3000,NULL);
				}
				break;
			case -2:
				msgInfo("Can't create file",3000,NULL);
				break;
			}
		break;
	default:
		break;
	}
}
static void save_load_general_configurations(int which)
{

	int retorno;
	unsigned char old_bw,old_mode;
	char config_path[1024];
	int length;
	FILE *fconfig; 
	
	strcpy(config_path,getenv("HOME"));
	length=strlen(config_path);
	if ((length>0)&&(config_path[length-1]!='/'))
		strcat(config_path,"/");
	strcat(config_path,"fbzx.conf");
	
	switch(which)
	{
	case 2:
	case 0: // Load or delete file
	{
		fconfig = fopen(config_path,"r");
		if (fconfig==NULL) 
			{
			msgInfo("Can not access the file",3000,NULL);
			return;
			}
		else fclose(fconfig);
		
			if (which == 0) // Load config file
			{
				old_bw = ordenador.bw;
				old_mode= ordenador.mode128k;
				if (!load_config(&ordenador,config_path)) msgInfo("General confs loaded",3000,NULL);
				if (old_bw!=ordenador.bw) computer_set_palete();
				if (old_mode != ordenador.mode128k) ResetComputer();
				break;
			}
			else // Delete config file
				if (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138/RATIO, FULL_DISPLAY_Y /2-48/RATIO)) unlink(config_path);
		
	} break;
	case 1: // Save configuration file
		retorno=save_config(&ordenador,config_path);
		
		switch(retorno) 
			{
			case 0: //OK
				msgInfo("General confs saved",3000,NULL);
				break;
			case -2:
				msgInfo("Can't create file",3000,NULL);
				break;
			}
		break;
	default:
		break;
	}
}
static void manage_configurations()
{
	int opt ;
	int submenus[3];

	memset(submenus, 0, sizeof(submenus));
	
	submenus[2]=!ordenador.autoconf;

	opt = menu_select_title("Configurations file menu",
			confs_messages, submenus);
	if (opt < 0)
		return;
		
	ordenador.autoconf=!submenus[2];	
	
	switch(opt)
		{
		case 0: // Save, load and delete general configurations 
			save_load_general_configurations(submenus[0]);
			break;
		case 3: // Save, load and delete game configurations
			save_load_game_configurations(submenus[1]);
			break;
		default:
			break;
		}		
}


void main_menu()
{
	int submenus[3];
	int opt;
	
	memset(submenus, 0, sizeof(submenus));

	do
	{
		opt = menu_select_title("Main menu", main_menu_messages, submenus);
		if (opt < 0)
			break;

		switch(opt)
		{
		case 0:
			manage_tape(submenus[0]);
			break;
		case 2:
			save_load_snapshot(submenus[1]);
			break;
		case 5:
			input_options(submenus[2]);
			break;
		case 7:
			emulation_settings();
			break;
		case 8:
			screen_settings();
			break;
		case 9:
			audio_settings();
			break;	
		case 10:
			manage_configurations();
			break;
		case 11:
			microdrive();
			break;	
		case 12:
			tools();
			break;
		case 13:
			ResetComputer ();
			ordenador.pause = 1;
			if (ordenador.tap_file != NULL) {
				ordenador.tape_current_mode = TAP_TRASH;
				rewind_tape (ordenador.tap_file,1);				
			}
			break;	
		case 14:
			if (msgYesNo("Are you sure to quit?", 0, FULL_DISPLAY_X /2-138/RATIO, FULL_DISPLAY_Y /2-48/RATIO)) 
				{salir = 0;}	
			break;
		default:
			break;
		}
	} while (opt == 4 || opt == 7 || opt == 11);
	
	clean_screen();
	
}

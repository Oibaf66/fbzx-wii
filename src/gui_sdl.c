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
//#include "menus.h"
#include "emulator.h"
#include "cargador.h"

#define ID_BUTTON_OFFSET 0
#define ID_AXIS_OFFSET 32

extern int usbismount, smbismount;

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

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
		/*08*/		"Microdrive",
		/*09*/		"Tools",
		/*10*/		"Save confs",
		/*11*/		"Reset",
		/*12*/		"Help",
		/*13*/		"Quit",
		NULL
};

static const char *emulation_messages[] = {
		/*00*/		"Emulated machine",
		/*01*/		"^|48k_2|48K_3|128k|+2|+2A/+3|128K_Sp",
		/*02*/		"Volume",
		/*03*/		"^|0|1|2|3|4|5|6|7|max",
		/*04*/		"Tap fast speed",
		/*05*/		"^|on|off",
		/*06*/		"Turbo mode",
		/*07*/		"^|on|off",
		/*08*/		"Double scan",
		/*09*/		"^|on|off",
		/*10*/		"TV mode",
		/*11*/		"^|Color|B&W",
		/*12*/		"AY-3-8912 Emulation",
		/*13*/		"^|on|off",		
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
		/*08*/		"Port",
		/*09*/		"^|sd|usb|smb",
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
	
	ordenador.tape_write = 0; // by default, can't record
	
	if(ordenador.tap_file==NULL)
		retorno=-1;
	else
		retorno=0;
	
	switch(retorno) {
	case 0: // all right
	strcpy(ordenador.current_tap,filename);
	strcpy(ordenador.last_selected_file,filename);
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
	&& (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138, FULL_DISPLAY_Y /2-48))) unlink(filename);
	
	free((void *)filename);
}
	

static void manage_tape(int which)
{
	switch (which)
	{
	case 0: //Insert
		insert_tape();
		break;
	case 1: //Emulate load ""
		ordenador.kbd_buffer_pointer=6;
		countdown=8;
		ordenador.keyboard_buffer[0][6]= SDLK_1;		//Edit
		ordenador.keyboard_buffer[1][6]= SDLK_LSHIFT;
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
		msgInfo("Not yet implemented",3000,NULL);
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
	return  ordenador.mode128k + (ordenador.issue==3);
}

static void set_machine_model(int which)
{
	switch (which)
	{
	case 0: //48k issue2
			ordenador.issue=2;
			ordenador.mode128k=0;
			ordenador.ay_emul=0;
		break;
	case 1: //48k issue3
			ordenador.issue=3;
			ordenador.mode128k=0;
			ordenador.ay_emul=0;
		break;
	case 2: //128k
			ordenador.issue=3;
			ordenador.mode128k=1;
			ordenador.ay_emul=1;
		break;
	case 3: //Amstrad +2
			ordenador.issue=3;
			ordenador.mode128k=2;
			ordenador.ay_emul=1;
		break;
	case 4: //Amstrad +2A/+3
			ordenador.issue=3;
			ordenador.mode128k=3;
			ordenador.ay_emul=1;
			ordenador.mdr_active=0;
		break;
	case 5: //128K Spanish
			ordenador.issue=3;
			ordenador.mode128k=4;
			ordenador.ay_emul=1;
		break;	
	
	}
}

static void emulation_settings(void)
{
	unsigned int submenus[7],submenus_old[7];
	int opt, i;
	
	memset(submenus, 0, sizeof(submenus));
	
	submenus[0] = get_machine_model();
	submenus[1] = ordenador.volume/2;
	submenus[2] = !ordenador.tape_fast_load;
	submenus[3] = !ordenador.turbo;
	submenus[4] = !ordenador.dblscan;
	submenus[5] = ordenador.bw;
	submenus[6] = !ordenador.ay_emul;
	
	for (i=0; i<7; i++) submenus_old[i] = submenus[i];
	
	opt = menu_select_title("Emulation settings menu",
			emulation_messages, submenus);
	if (opt < 0)
		return;
	
	set_machine_model(submenus[0]);
	if (submenus[0] != submenus_old[0]) ResetComputer(); else 
	ordenador.ay_emul = !submenus[6];
	
	ordenador.volume = submenus[1]*2; //I should use set_volume() ?
	ordenador.tape_fast_load = !submenus[2];
	ordenador.turbo = !submenus[3];
	
	if(ordenador.turbo){
				ordenador.tst_sample=12000000/ordenador.freq; //5,0 MHz max emulation speed for wii at full frames
				jump_frames=3;
			} else {
				ordenador.tst_sample=3500000/ordenador.freq;
				jump_frames=0;
				curr_frames=0;
			}
	
	ordenador.dblscan = !submenus[4];
	ordenador.bw = submenus[5]; 
	if (submenus[5]!=submenus_old[5]) computer_set_palete();
	
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
	&& (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138, FULL_DISPLAY_Y /2-48))) unlink(filename);
	
	free((void *)filename);
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
			msgInfo("Not yet implemented",3000,NULL);
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
					
	// Save SCR file		
	snprintf(db, 255, "%s/%s.scr", dir, fb);	
		
	fichero=fopen(db,"r");
	
	if(fichero!=NULL)
	{	
		fclose(fichero);
		if (!msgYesNo("Overwrite the exiting file?", 0, FULL_DISPLAY_X /2-160, FULL_DISPLAY_Y /2-48))
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
		if ((length>0)&&(path_snaps[length-1]!='/'))
		strcat(path_snaps,"/");
		strcpy(path_taps,path_snaps);
		strcat(path_snaps,"snapshots");
		strcat(path_taps,"tapes");
		ordenador.port = which;
		break;
	case 1: //PORT_USB
		if (usbismount) {
			strcpy(path_snaps,"usb:/");
			strcpy(path_taps,"usb:/");
			ordenador.port = which;}
		else
			msgInfo("USB is not mounted",3000,NULL);
		break;
	case 2: //PORT_SMB
		if (smbismount) {
			strcpy(path_snaps,"smb:/");
			strcpy(path_taps,"smb:/");
			ordenador.port = which;}
		else
			msgInfo("SMB is not mounted",3000,NULL);
		break;
	default:
		break;		
	}	
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
			show_keyboard_layout();
			break;
		case 2: // Save SCR
			save_scr();
			break;
		case 4: // Load SCR 
			load_scr();
			break;
		case 6: // Insert poke
			// Insert poke ;
			msgInfo("Not yet implemented",3000,NULL);
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
				if (msgYesNo("Delete the file?", 0, FULL_DISPLAY_X /2-138, FULL_DISPLAY_Y /2-48)) unlink(filename);
		}	
		free((void*)filename);
	} break;
	case 1: // Save snapshot file
		snprintf(db, 255, "%s/%s.z80", dir, fb);
		retorno=save_z80(db,0);
		switch(retorno) 
			{
			case 0: //OK
				msgInfo("Snapshot saved",3000,NULL);
				break;
			case -1:
				if (msgYesNo("Overwrite the exiting file?", 0, FULL_DISPLAY_X /2-160, FULL_DISPLAY_Y /2-48))
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
	
static void help(void)
{
	menu_select_title("FBZX-WII help",
			help_messages, NULL);
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
			microdrive();
			break;
		case 9:
			tools();
			break;	
		case 10:
			save_config(&ordenador);
			msgInfo("Configurations saved",3000,NULL);
			break;
		case 11:
			ResetComputer ();
			ordenador.pause = 1;
			if (ordenador.tap_file != NULL) {
				ordenador.tape_current_mode = TAP_TRASH;
				rewind_tape (ordenador.tap_file,1);				
			}
			break;	
		case 12:
			help();
			break;
		case 13:
			if (msgYesNo("Are you sure to quit?", 0, FULL_DISPLAY_X /2-138, FULL_DISPLAY_Y /2-48)) 
				{salir = 0;}	
			break;
		default:
			break;
		}
	} while (opt == 5 || opt == 7 || opt == 8 || opt == 12);
	
	clean_screen();
	
}

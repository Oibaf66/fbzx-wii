 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#include <sys/types.h>
#include <sys/stat.h>

#include "menu_sdl.h"
#include "emulator.h"
//#include "VirtualKeyboard.h" per ora

#define ID_BUTTON_OFFSET 0
#define ID_AXIS_OFFSET 32

//extern int usbismount, smbismount; per ora

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

extern int countdown;


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
		/*03*/		"^|0%|25%|50%|75%|100%",
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
		/*02*/		"  ",
		/*03*/		"Bind key to Wiimote",
		/*04*/		"^|1|2|-",
		/*05*/		"  ",
		/*06*/		"Bind key to Nunchuk",
		/*07*/		"^|Z|C",
		/*08*/		"  ",
		/*09*/		"Bind key to Classic",
		/*10*/		"^|a|b|x|y|L|R|Zl|Zr|-",
		/*11*/		"  ",
		/*12*/		"Rumble",
		/*13*/		"^|On|Off",
		NULL,
};

static const char *microdrive_messages[] = {
		/*00*/		"Select microdrive",
		/*01*/		"  ",
		/*02*/		"Create microdrive file",
		/*03*/		"  ",
		/*04*/		"Interface I",
		/*05*/		"^|on|off",
		/*06*/		"  ",
		/*07*/		"Write protection",
		/*08*/		"^|on|off",
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
		NULL
};

static const char *help_messages[] = {
		/*00*/		"#2HOME enters the menu system, where arrow keys",
		/*01*/		"#2and nunchuck are used to navigate up and down.",
		/*02*/		"#2You can bind keyboard keys to the wiimote",
		/*03*/		"#2buttons in the 'Wiimote' menu and",
		/*04*/		"#2change emulation options in the Settings menu.",
		/*05*/		"#2 ",
		/*06*/		"#2 ",
		/*07*/		"#2 ",
		/*08*/		"#2 ",
		/*09*/		"#2 ",
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
	
	if (strcmp(filename, "None") == 0) //TO FIX IT
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

	ordenador.tap_file=fopen(filename,"r+"); // read and write
	
	ordenador.tape_write = 0; // by default, can't record
	
	if(ordenador.tap_file==NULL)
		retorno=-1;
	else
		retorno=0;

	strcpy(ordenador.current_tap,filename);

	free((void *)filename);

	switch(retorno) {
	case 0: // all right
	break;
	case -1:
		msgInfo("Error: Can't load that file",3000,NULL);
		ordenador.current_tap[0]=0;
		return;
	break;
	}

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
	
	if (strcmp(filename, "None") == 0)
	{
			free((void *)filename);
			return;
	}
	
	if (ext_matches(filename, ".tap")|ext_matches(filename, ".TAP")|ext_matches(filename, ".tzx")|
	ext_matches(filename, ".TZX")) unlink(filename);
	
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
		countdown=5;
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
	submenus[1] = (unsigned int) (ordenador.volume/16);
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
	
	ordenador.volume = submenus[1]*16;
	ordenador.tape_fast_load = !submenus[2];
	ordenador.turbo = !submenus[3];
	
	if(ordenador.turbo){
				ordenador.tst_sample=12000000/ordenador.freq; //5,0 MHz max emulation speed for wii at full frames
				jump_frames=3;
			} else {
				ordenador.tst_sample=3500000/ordenador.freq;
				jump_frames=0;
			}
	
	ordenador.dblscan = !submenus[4];
	ordenador.bw = submenus[5]; 
	if (submenus[5]!=submenus_old[5]) computer_set_palete();
	
}

static void setup_joystick(int joy, const char *key, int sdl_key)
{
	/*
	if (!strcmp(key, "None")) 
	{
	changed_prefs.joystick_settings[1][joy].eventid[ID_BUTTON_OFFSET + sdl_key][0] = 0;
	}
	else
	insert_keyboard_map(key, "input.1.joystick.%d.button.%d", joy, sdl_key);
	*/
}

static void input_options(int joy)
{
	const int wiimote_to_sdl[] = {2, 3, 4, 5};
	const int nunchuk_to_sdl[] = {7, 8};
	const int classic_to_sdl[] = {9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
	int sdl_key = 1;
	const char *key;
	unsigned int submenus[5];
	int opt;
	
	//struct virtkey *virtualkey;

	memset(submenus, 0, sizeof(submenus));
	
	submenus[0] = ordenador.joystick[joy];
	submenus[4] = !ordenador.rumble[joy];
	
	opt = menu_select_title("Input menu",
			input_messages, submenus);
	if (opt < 0)
		return;
	
	ordenador.joystick[joy] = submenus[0];
	ordenador.rumble[joy] = !submenus[4];
	
	/*
	virtualkey = virtkbd_get_key();
	if (virtualkey == NULL)
		return;
	key = virtualkey->ev_name;
	*/
	
	switch(opt)
		{
		case 3: // wiimote 
			sdl_key = wiimote_to_sdl[submenus[1]]; break;
		case 6: // nunchuk
			sdl_key = nunchuk_to_sdl[submenus[2]]; break;
		case 9: // classic
			sdl_key = classic_to_sdl[submenus[3]]; break;
		default:
			break;
		}
		
	setup_joystick(joy, key, sdl_key);
	
}

static void microdrive()
{
	
	unsigned int submenus[2], submenus_old[2];
	int opt,retval ;

	memset(submenus, 0, sizeof(submenus));
	
	submenus[0] = !ordenador.mdr_active;
	submenus[1] = !ordenador.mdr_cartridge[137922];
	
	submenus_old[0] = submenus[0];
	submenus_old[1] = submenus[1];
	
	opt = menu_select_title("Microdrive menu",
			microdrive_messages, submenus);
	if (opt < 0)
		return;
	
	ordenador.mdr_active = !submenus[0];
	
	
	if (submenus[0]!=submenus_old[0]) ResetComputer();
	if (submenus[1]!=submenus_old[1]) 
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
	
	switch(opt)
		{
		case 0: // Select microdrive 
			//Select microdrive ;
			break;
		case 2: // Create microdrive file
			// Create microdrive file ;
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
	
static void tools()
{
	int opt ;
	
	opt = menu_select_title("Tools menu",
			tools_messages, NULL);
	if (opt < 0)
		return;
	
	switch(opt)
		{
		case 0: // Show keyboard 
			show_keyboard_layout();
			break;
		case 2: // Save SCR
			// Save SCR ;
			break;
		case 4: // Load SCR 
			//Load SCR ;
			break;
		case 6: // Insert poke
			// Insert poke ;
			break;
		default:
			break;
		}		
}


/*

static void set_Port(int which)
{
	switch (which)
	{
	case PORT_SD:
		prefs_set_attr ("floppy_path",    strdup_path_expand (TARGET_FLOPPY_PATH));
		changed_prefs.Port = which;
		currprefs.Port = changed_prefs.Port;
		break;
	case PORT_USB:
		if (usbismount) {
			prefs_set_attr ("floppy_path",    strdup_path_expand (TARGET_USB_PATH));
			changed_prefs.Port = which;
			currprefs.Port = changed_prefs.Port;}
		else
			msgInfo("USB is not mounted",3000,NULL);
		break;
	case PORT_SMB:
		if (smbismount) {
			prefs_set_attr ("floppy_path",    strdup_path_expand (TARGET_SMB_PATH));
			changed_prefs.Port = which;
			currprefs.Port = changed_prefs.Port;}
		else
			msgInfo("SMB is not mounted",3000,NULL);
		break;
	default:
		break;		
	}	
}

static void virtual_keyboard(void)
{
	int key_code;
	
	virtkey_t *key =virtkbd_get_key();  
	if (key) {key_code = key->sdl_code;} else return;
	
	SDL_Event event_key;
	
	event_key.type=SDL_KEYDOWN;
	event_key.key.keysym.sym=key_code;
	SDL_PushEvent(&event_key);
	DEBUG_LOG ("Push Event: keycode %d %s\n", key_code, "SDL_KEYDOWN");
	
	event_key.type=SDL_KEYUP;
	SDL_PushEvent(&event_key);
	DEBUG_LOG ("Push Event: keycode %d %s\n", key_code, "SDL_KEYUP");
	
}	

*/

static void save_load_snapshot(int which)
{
	const char *dir = path_snaps;
	const char *tape = ordenador.current_tap;
	char db[256];
	char fb[81];
	int retorno;

	// Name (for saves) - TO CHECK
	if (tape && strrchr(tape, '/'))
		strncpy(fb, strrchr(tape, '/') + 1, 80);
	else
		strcpy(fb, "unknown");

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
				retorno=load_z80(filename);

				switch(retorno) {
				case 0: // all right
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
				unlink(filename);
		}	
		free((void*)filename);
	} break;
	case 1: // Save snapshot file
		snprintf(db, 255, "%s/%s.z80", dir, fb);
		retorno=save_z80(db);
		msgInfo("State saved",3000,NULL);
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
	} while (opt == 5 || opt == 7 || opt == 8 || opt == 9 || opt == 12);
	
}

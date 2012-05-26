/* 
 * Copyright (C) 2012 Fabio Olimpieri
 * Copyright 2003-2009 (C) Raster Software Vigo (Sergio Costas)
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

#include "z80free/Z80free.h"
#include "computer.h"
#include "emulator.h"
#include "cargador.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include "characters.h"
#include "menus.h"
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include<SDL/SDL_image.h>
#include "sound.h"
#include "tape.h"
#include "microdrive.h"
#include "menu_sdl.h"

#ifdef GEKKO
#include <gccore.h>
#include <fat.h>
#include <ogc/usbstorage.h>
#include <network.h>
#include <smb.h>
#endif

#ifdef DEBUG
FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

char debug_var=1;

Z80FREE procesador;
struct computer ordenador;
SDL_Surface *screen;
char salir,sound_aborted;
unsigned char *sound[NUM_SNDBUF];
char path_snaps[2049];
char path_taps[2049];
char path_mdrs[2049];
char path_scr[2049];
char path_confs[2049];
unsigned int colors[80];
unsigned int jump_frames,curr_frames;
char *filenames[5];
static SDL_Surface *image;

bool usbismount = false;
bool networkisinit = false;
bool smbismount = false; 

#if defined(GEKKO)

/****************************************************************************
 * Mount SMB Share
 ****************************************************************************/

bool ConnectShare ()
{
	
	if(smbismount)
		return true;
		printf("user:  %s\n", ordenador.SmbUser);
		printf("pass:  %s\n", ordenador.SmbPwd);
		printf("share: %s\n", ordenador.SmbShare);
		printf("ip:    %s\n", ordenador.SmbIp);
		
		int a;
		for (a=0;a<3;a++)
		if(smbInit(ordenador.SmbUser, ordenador.SmbPwd,ordenador.SmbShare, ordenador.SmbIp))
			{smbismount = true;	break;}
			
		
		if(!smbismount) printf("Failed to connect to SMB share\n");
		else {
		printf("Established connection to SMB share\n");
		}

	return smbismount;
}

void CloseShare()
{

	if(smbismount) {
	printf("Disconnected from SMB share\n");
	smbClose("smb");
	}
	smbismount = false;
}

/****************************************************************************
 * init and deinit USB device functions
 ****************************************************************************/
 
bool InitUSB()
{ 
	printf("Initializing USB FAT subsytem ...\n");
	fatUnmount("usb:");
	
	// This should wake up the drive
	bool isMounted = fatMountSimple("usb", &__io_usbstorage);
	
	bool isInserted = __io_usbstorage.isInserted();
	if (!isInserted) 
	{
	printf("USB device not found\n");
	return false;
	}
 
	// USB Drive may be "sleeeeping" 
	// We need to try Mounting a few times to wake it up
	int retry = 10;
	while (retry && !isMounted)
	{
		sleep(1);
		isMounted = fatMountSimple("usb", &__io_usbstorage);
		retry--; 
	}
	if (isMounted) 
		printf("USB FAT subsytem initialized\n");
	else
		printf("Impossible to initialize USB FAT subsytem\n");
	return isMounted;
 }
 
 void DeInitUSB()
{
	fatUnmount("usb:");
	__io_usbstorage.shutdown(); 
}

bool InitNetwork()
{
        char myIP[16];

        memset(myIP, 0, sizeof(myIP));
	printf("Getting IP address via DHCP...\n");

	if (if_config(myIP, NULL, NULL, true) < 0) {
	        	printf("No DHCP reply\n");
	        	return false;
        }
	printf("Got an address: %s\n",myIP);
	return true;
}

#endif

int load_zxspectrum_picture()
{

image=IMG_Load("/fbzx-wii/fbzx/ZXSpectrum48k.png");

if (image == NULL) {printf("Impossible to load image\n"); return 0;}

SDL_BlitSurface(image, NULL, ordenador.screen, NULL);

if (ordenador.mustlock) {
				SDL_UnlockSurface (ordenador.screen);
				SDL_Flip (ordenador.screen);
				SDL_LockSurface (ordenador.screen);
			} else {
				SDL_Flip (ordenador.screen);
			}

return 1;
} 

void SDL_Fullscreen_Switch()
{
	Uint32 flags = screen->flags;
	if ( flags & SDL_FULLSCREEN )
		flags &= ~SDL_FULLSCREEN;
	else
		flags |= SDL_FULLSCREEN;

	screen = SDL_SetVideoMode(screen->w, screen->h, screen->format->BitsPerPixel,flags);
}

FILE *myfopen(char *filename,char *mode) {
	
	char tmp[4096];
	FILE *fichero;
	
	fichero=fopen(filename,mode);
	if (fichero!=NULL) {
		return (fichero);
	}
	sprintf(tmp,"/usr/share/%s",filename);
	fichero=fopen(tmp,mode);
	if (fichero!=NULL) {
		return (fichero);
	}
	sprintf(tmp,"/usr/local/share/%s",filename);
	fichero=fopen(tmp,mode);
	if (fichero!=NULL) {
		return (fichero);
	}
	#ifdef GEKKO
	sprintf(tmp,"/fbzx-wii/%s",filename);
	fichero=fopen(tmp,mode);
	if (fichero!=NULL) {
		return (fichero);
	}
	#endif
	
	return (NULL);
}

char *load_a_rom(char **filenames) {
	
	char **pointer;
	int offset=0;
	FILE *fichero;
	int size;
	
	for(pointer=filenames;*pointer!=NULL;pointer++) {
		fichero=myfopen(*pointer,"r");
		if(fichero==NULL) {
			return (*pointer);
		}
		size=fread(ordenador.memoria+offset,16384,1,fichero);
		offset+=16384;
		fclose(fichero);
	}
	return (NULL);
}

void load_rom(char type) {

	char *retval;
	FILE *fichero;
	int size;

	switch(type) {
	case 0:
		filenames[0]="spectrum-roms/48.rom";
		filenames[1]=NULL;
		retval=load_a_rom(filenames);
		if (retval) {
			printf("Can't load file %s\n",retval);
			exit(1);
		}
	break;
	case 1:
		filenames[0]="spectrum-roms/128-0.rom";
		filenames[1]="spectrum-roms/128-1.rom";
		filenames[2]=NULL;
		retval=load_a_rom(filenames);
		if (retval) {
			printf("Can't load file %s\n",retval);
			exit(1);
		}
	break;
	case 2:
		filenames[0]="spectrum-roms/plus2-0.rom";
		filenames[1]="spectrum-roms/plus2-1.rom";
		filenames[2]=NULL;
		retval=load_a_rom(filenames);
		if (retval) {
			printf("Can't load file %s\n",retval);
			exit(1);
		}
	break;
	case 3:
		// first, try last version of PLUS3 roms
		
		filenames[0]="spectrum-roms/plus3-41-0.rom";
		filenames[1]="spectrum-roms/plus3-41-1.rom";
		filenames[2]="spectrum-roms/plus3-41-2.rom";
		filenames[3]="spectrum-roms/plus3-41-3.rom";
		filenames[4]=NULL;
		retval=load_a_rom(filenames);
		if (retval) {
			printf("Can't load the Spectrum +3 ROM version 4.1. Trying with version 4.0\n");
			filenames[0]="spectrum-roms/plus3-40-0.rom";
			filenames[1]="spectrum-roms/plus3-40-1.rom";
			filenames[2]="spectrum-roms/plus3-40-2.rom";
			filenames[3]="spectrum-roms/plus3-40-3.rom";
			filenames[4]=NULL;
			retval=load_a_rom(filenames);
			if (retval) {
				printf("Can't load the Spectrum +3 ROM version 4.0. Trying with legacy filenames\n");
				filenames[0]="spectrum-roms/plus3-0.rom";
				filenames[1]="spectrum-roms/plus3-1.rom";
				filenames[2]="spectrum-roms/plus3-2.rom";
				filenames[3]="spectrum-roms/plus3-3.rom";
				filenames[4]=NULL;
				retval=load_a_rom(filenames);
				if (retval) {
					printf("Can't load file %s\n",retval);
					exit(1);
				}
			}
		}
	break;
	case 4:
		filenames[0]="spectrum-roms/128-spanish-0.rom";
		filenames[1]="spectrum-roms/128-spanish-1.rom";
		filenames[2]=NULL;
		retval=load_a_rom(filenames);
		if (retval) {
			printf("Can't load file %s\n",retval);
			exit(1);
		}
	break;
	}
  
	fichero=myfopen("spectrum-roms/if1-2.rom","r"); // load Interface1 ROM
	if(fichero==NULL) {
		// try legacy name
		fichero=myfopen("spectrum-roms/if1-v2.rom","r");
		if(fichero==NULL) {
			printf("Can't open Interface1 ROM file\n");
			exit(1);
		}
	}
	size=fread(ordenador.shadowrom,8192,1,fichero);
  	fclose(fichero);
}

void init_screen(int resx,int resy,int depth,int fullscreen,int dblbuffer,int hwsurface) {

	int retorno,bucle,bucle2,valores,ret2;

	//if (sound_type!=3)
	retorno=SDL_Init(SDL_INIT_VIDEO);
	/*else
		retorno=SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);*/
	if(retorno!=0) {
		printf("Can't initialize SDL library. Exiting\n");
		exit(1);
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
		ordenador.use_js=0;
		printf("Can't initialize JoyStick subsystem\n");
	} else {
		printf("JoyStick subsystem initialized\n");
		ordenador.use_js=1;
		if(SDL_NumJoysticks()>0){
			// Open joystick
			ordenador.joystick_number = SDL_NumJoysticks();
			if (ordenador.joystick_number>2) ordenador.joystick_number = 2; //Open max 2 joysticks
			printf("Try to open %d joysticks \n", ordenador.joystick_number);
			for (bucle=0;bucle<ordenador.joystick_number;bucle++) {
			ordenador.joystick_sdl [bucle] = SDL_JoystickOpen(bucle);
	  			if (NULL==ordenador.joystick_sdl [bucle]) {
	  				printf("Can't open joystick %d\n",bucle);
	  			}
  			}
  		}
	}

	// screen initialization
	valores=SDL_HWPALETTE|SDL_ANYFORMAT;
	if (fullscreen==1)
		valores|=SDL_FULLSCREEN;
  
	if (dblbuffer==1)
		valores|=SDL_DOUBLEBUF;
	if (hwsurface==1)
		valores|=SDL_HWSURFACE;
	else
		valores|=SDL_SWSURFACE;
  
	screen=SDL_SetVideoMode(resx,resy,depth,valores);
	if(screen==NULL) {
		printf("Can't assign SDL Surface. Exiting\n");
		exit(1);
	}

	ordenador.bpp=screen->format->BytesPerPixel;
	printf("Bytes per pixel: %d\n",ordenador.bpp);
  
	if(SDL_MUSTLOCK(screen)) {
		ordenador.mustlock=1;
		SDL_LockSurface(screen);
	} else
		ordenador.mustlock=0;

	printf("Locking screen: %d\n", ordenador.mustlock);

	// sound initialization

	if (sound_type==SOUND_AUTOMATIC) {
		ret2=sound_init(1); // check all sound systems
	} else {
		ret2=sound_init(0); // try with the one specified in command line
	}
	if(ret2==0) {
		sound_aborted=0;
	} else { // if fails, run without sound
		sound_type=SOUND_NO;
		sound_init(0);
		sound_aborted=1;
	}
	printf("Init sound\n");
	if(ordenador.format)
		ordenador.increment=2*ordenador.channels;
	else
		ordenador.increment=ordenador.channels;
	
	for(bucle2=0;bucle2<NUM_SNDBUF;bucle2++) {
		//sound[bucle2]=(unsigned char *)malloc(ordenador.buffer_len*ordenador.increment+8);
		//ASND Required alligned memory with padding
		sound[bucle2]=(unsigned char *)memalign(32,ordenador.buffer_len*ordenador.increment+32);
		for(bucle=0;bucle<ordenador.buffer_len*ordenador.increment+4;bucle++)
			sound[bucle2][bucle]=0; 
	}

	printf("Init sound 2\n");
	ordenador.tst_sample=3500000/ordenador.freq;
	//printf("Set volume\n");
	//set_volume(70);
	printf("Return init\n");
}

void end_system() {
	
	sound_close();
	
	if(ordenador.mustlock)
		SDL_UnlockSurface(screen);

	if(ordenador.tap_file!=NULL)
		fclose(ordenador.tap_file);

	SDL_Quit();
}

void load_main_game(char *nombre) {

	int longitud,retval;
	char *puntero;
	
	longitud=strlen(nombre);
	if (longitud<5) {
		return;
	}
	puntero=nombre+(longitud-4);
	if ((0==strcasecmp(".z80",puntero))||(0==strcasecmp(".sna",puntero))) {
		load_z80(nombre);
		return;
	}
	
	if ((0==strcasecmp(".tap",puntero))||(0==strcasecmp(".tzx",puntero))) {
		char char_id[10];
		ordenador.tape_write = 0; // by default, can't record
		ordenador.tap_file=fopen(nombre,"r+"); // read and write
		if(ordenador.tap_file==NULL)
			return;

		strcpy(ordenador.current_tap,nombre);
   
		retval=fread(char_id,10,1,ordenador.tap_file); // read the (maybe) TZX header
		if((!strncmp(char_id,"ZXTape!",7)) && (char_id[7]==0x1A)&&(char_id[8]==1)) {
			ordenador.tape_file_type = TAP_TZX;
			rewind_tape(ordenador.tap_file,1);	  
		} else {
			ordenador.tape_file_type = TAP_TAP;
			rewind_tape(ordenador.tap_file,1);
		}
		return;
	}
}

int save_config(struct computer *object, char *filename) {
	
	unsigned char key, joy_n;
	FILE *fconfig;
	
	fconfig = fopen(filename,"wb");
	if (fconfig==NULL) {
		return -2; // can't create file
	}
	
	fprintf(fconfig,"mode=%c%c",48+object->mode128k,10);
	fprintf(fconfig,"issue=%c%c",48+object->issue,10);
	fprintf(fconfig,"joystick1=%c%c",48+object->joystick[0],10);
	fprintf(fconfig,"joystick2=%c%c",48+object->joystick[1],10);
	fprintf(fconfig,"ay_sound=%c%c",48+object->ay_emul,10);
	fprintf(fconfig,"interface1=%c%c",48+object->mdr_active,10);
	fprintf(fconfig,"doublescan=%c%c",48+object->dblscan,10);
	fprintf(fconfig,"volume=%c%c",65+(object->volume),10);
	fprintf(fconfig,"bw=%c%c",48+object->bw,10);
	fprintf(fconfig,"tap_fast=%c%c",48+object->tape_fast_load,10);
	fprintf(fconfig,"joypad1=%c%c",48+object->joypad_as_joystick[0],10);
	fprintf(fconfig,"joypad2=%c%c",48+object->joypad_as_joystick[1],10);
	fprintf(fconfig,"rumble1=%c%c",48+object->rumble[0],10);
	fprintf(fconfig,"rumble2=%c%c",48+object->rumble[1],10);
	fprintf(fconfig,"port=%c%c",48+object->port,10);
	fprintf(fconfig,"autoconf=%c%c",48+object->autoconf,10);
	
	
	for (joy_n=0; joy_n<2; joy_n++)
		for (key=0; key<22; key++)
		fprintf(fconfig,"joybutton_%c_%c=%.3d%c",joy_n+48,key+97, object->joybuttonkey[joy_n][key],10);
	
	fclose(fconfig);
	return 0;
	
}

int save_config_game(struct computer *object, char *filename, int overwrite) {
	
	unsigned char key, joy_n;
	FILE *fconfig;
	
	fconfig=fopen(filename,"r");
	if((fconfig!=NULL)&&(!overwrite)) {
    fclose(fconfig);
    return -1; // file already exists
	}
	
	fconfig = fopen(filename,"wb");
	if (fconfig==NULL) {
		return -2; // can't create file
	}
	
	fprintf(fconfig,"joystick1=%c%c",48+object->joystick[0],10);
	fprintf(fconfig,"joystick2=%c%c",48+object->joystick[1],10);
	fprintf(fconfig,"ay_sound=%c%c",48+object->ay_emul,10);
	fprintf(fconfig,"joypad1=%c%c",48+object->joypad_as_joystick[0],10);
	fprintf(fconfig,"joypad2=%c%c",48+object->joypad_as_joystick[1],10);
	fprintf(fconfig,"rumble1=%c%c",48+object->rumble[0],10);
	fprintf(fconfig,"rumble2=%c%c",48+object->rumble[1],10);
	
	for (joy_n=0; joy_n<2; joy_n++)
		for (key=0; key<22; key++)
		fprintf(fconfig,"joybutton_%c_%c=%.3d%c",joy_n+48,key+97, object->joybuttonkey[joy_n][key],10);
	
	fclose(fconfig);
	return 0;
	
}

void load_config_smb(struct computer *object) {
	
	char line[1024],carac,done;
	int pos;
	FILE *fconfig;
	unsigned char smb_enable=0;
	
	fconfig = fopen("/fbzx-wii/fbzx.smb","r");
	if (fconfig==NULL) {
		return;
	}
	
	done=1;
	pos=0;
	line[0]=0;
	while(!feof(fconfig)) {
		if (done) {
			line[0]=0;
			pos=0;
			done=0;
		}
		if (0!=fread(&carac,1,1,fconfig)) {
			if ((carac!=13)&&(carac!=10)) {
				line[pos]=carac;
				if (pos<1023) {
					pos++;
				}
				continue;
			}
		}
		done=1;
		line[pos]=0;
		if (line[0]=='#') { // comment
			continue;
		}
		if (!strncmp(line,"smb_enable=",11)) {
			smb_enable=line[11]-'0';
			continue;
		}
		if (!strncmp(line,"user=",5)) {
			if (line[5])
			strcpy (object->SmbUser,line+5);
			continue;
		}
		if (!strncmp(line,"password=",9)) {
			if (line[9])
			strcpy (object->SmbPwd,line+9);
			continue;
		}
		if (!strncmp(line,"share_name=",11)) {
			if (line[11])
			strcpy (object->SmbShare,line+11);
			continue;
		}
		
		if (!strncmp(line,"smb_ip=",7)) {
			if (line[7])
			strcpy (object->SmbIp,line+7);
			continue;
		}
		
		if (smb_enable<2) {
		object->smb_enable=smb_enable;}
	}
		
		
fclose(fconfig);
}

int load_config(struct computer *object, char *filename) {
	
	char config_path[1024];
	char line[1024],carac,done;
	int pos, key_sdl=0;
	FILE *fconfig;
	unsigned char volume=255,mode128k=255,issue=255,joystick1=255,joystick2=255,ay_emul=255,mdr_active=255,
	dblscan=255,bw=255, tap_fast=255, joypad1=255, joypad2=255, rumble1=255, rumble2=255, joy_n=255, key_n=255, port=255, autoconf=255;
	
	if (filename) strcpy(config_path,filename); 
	else return -2;
	
	fconfig = fopen(config_path,"rb");
	if (fconfig==NULL) {
		return -1;
	}
	
	done=1;
	pos=0;
	line[0]=0;
	while(!feof(fconfig)) {
		if (done) {
			line[0]=0;
			pos=0;
			done=0;
		}
		if (0!=fread(&carac,1,1,fconfig)) {
			if ((carac!=13)&&(carac!=10)) {
				line[pos]=carac;
				if (pos<1023) {
					pos++;
				}
				continue;
			}
		}
		done=1;
		line[pos]=0;
		if (line[0]=='#') { // comment
			continue;
		}
		if (!strncmp(line,"mode=",5)) {
			printf("Cambio a modo %c\n",line[5]);
			mode128k=line[5]-'0';
			continue;
		}
		if (!strncmp(line,"issue=",6)) {
			issue=line[6]-'0';
			continue;
		}
		if (!strncmp(line,"joystick1=",10)) {
			joystick1=line[10]-'0';
			continue;
		}
		if (!strncmp(line,"joystick2=",10)) {
			joystick2=line[10]-'0';
			continue;
		}
		if (!strncmp(line,"ay_sound=",9)) {
			ay_emul=line[9]-'0';
			continue;
		}
		if (!strncmp(line,"interface1=",11)) {
			mdr_active=line[11]-'0';
			continue;
		}
		if (!strncmp(line,"doublescan=",11)) {
			dblscan=line[11]-'0';
			continue;
		}
		if (!strncmp(line,"volume=",7)) {
			volume=(line[7]-'A');
			continue;
		}
		if (!strncmp(line,"bw=",3)) {
			bw=(line[3]-'0');
			continue;
		}
		if (!strncmp(line,"tap_fast=",9)) {
			tap_fast=(line[9]-'0');
			continue;
		}
		if (!strncmp(line,"joypad1=",8)) {
			joypad1=line[8]-'0';
			continue;
		}
		if (!strncmp(line,"joypad2=",8)) {
			joypad2=line[8]-'0';
			continue;
		}
		if (!strncmp(line,"rumble1=",8)) {
			rumble1=line[8]-'0';
			continue;
		}
		if (!strncmp(line,"rumble2=",8)) {
			rumble2=line[8]-'0';
			continue;
		}
		if (!strncmp(line,"port=",5)) {
			port=line[5]-'0';
			continue;
		}
		if (!strncmp(line,"autoconf=",9)) {
			autoconf=line[9]-'0';
			continue;
		}
		if (!strncmp(line,"joybutton_",10)) {
			sscanf(line, "joybutton_%c_%c=%3d",&joy_n ,&key_n, &key_sdl);
			if ((joy_n<50) && (joy_n>47) && (key_n<119) && (key_n>96))
			object->joybuttonkey[joy_n-48][key_n-97]=key_sdl;
			continue;
		}
	}
	
	if (mode128k<5) {
		object->mode128k=mode128k;
	}
	if (issue<4) {
		object->issue=issue;
	}
	if (joystick1<4) {
		object->joystick[0]=joystick1;
	}
	if (joystick2<4) {
		object->joystick[1]=joystick2;
	}
	if (ay_emul<2) {
		object->ay_emul=ay_emul;
	}
	if (mdr_active<2) {
		object->mdr_active=mdr_active;
	}
	if (dblscan<2) {
		object->dblscan=dblscan;
	}
	if (bw<2) {
		object->bw=bw;
	}
	if (volume<17) {
		object->volume=volume;
		set_volume(volume);
	}
	if (tap_fast<2) {
		object->tape_fast_load=tap_fast;
	}
	if (joypad1<2) {
		object->joypad_as_joystick[0]=joypad1;
	}
	if (joypad2<2) {
		object->joypad_as_joystick[1]=joypad2;
	}
	if (rumble1<2) {
		object->rumble[0]=rumble1;
	}
	if (rumble2<2) {
		object->rumble[1]=rumble2;
	}
	if (port<3) {
		object->port=port;
	}
	if (autoconf<2) {
		object->autoconf=autoconf;
	}
		
	fclose(fconfig);
	return 0;
}

int main(int argc,char *argv[]) {

	int bucle,tstados,argumento,fullscreen,dblbuffer,hwsurface,length;
	char gamefile[4096],config_path[1024] ;
	
	word PC=0;
	

	// by default, try all sound modes
	sound_type=SOUND_AUTOMATIC;
	gamefile[0]=0;
	ordenador.zaurus_mini=0;
	ordenador.text_mini=0;
	ordenador.ulaplus=0;
	ordenador.ulaplus_reg=0;
	fullscreen=0;
	dblbuffer=0;
	hwsurface=0;
	
	argumento=0;
	jump_frames=0;
	curr_frames=0;
	ordenador.dblscan=1;
	ordenador.bw=0;
	
	#ifdef DEBUG
	fatInitDefault();
	fdebug = fopen("/fbzx-wii/logfile.txt","w");
	#endif
	
	#ifdef GEKKO
	dblbuffer=1;
	hwsurface=1;
	setenv("HOME", "/fbzx-wii", 1);

	//initialize libfat library
	if (!fatInitDefault())
	{ 
		printf("Couldn't initialize SD fat subsytem\n");
		
		exit(0);
	}
	else
		printf("SD FAT subsytem initialized\n");
		
	#endif


	computer_init();
	printf("Computer init\n");

	printf("Modo: %d\n",ordenador.mode128k);
	
	printf("Set volume\n");
	set_volume(16);
	
	// load current config
	strcpy(config_path,getenv("HOME"));
	length=strlen(config_path);
	if ((length>0)&&(config_path[length-1]!='/'))
		strcat(config_path,"/");
	strcat(config_path,"fbzx.conf");	
	
	load_config(&ordenador, config_path);
	
	printf("Modo: %d\n",ordenador.mode128k);
	while(argumento<argc) {
		if ((0==strcmp(argv[argumento],"-h"))||(0==strcmp(argv[argumento],"--help"))) {
			printf("\nUsage: fbzx [-nosound] ");
#ifdef D_SOUND_ALSA
			printf("[-alsa] ");
#endif
#ifdef D_SOUND_OSS
			printf("[-oss] ");
#endif
#ifdef D_SOUND_PULSE
			printf("[-pulse] ");
#endif

			printf("[-mini] [-rotate] [-fs] [-hw] [-db] [-ds] [-ss] [-jump N] [gamefile]\n");
			printf("\n  -nosound: don't emulate sound\n");

#ifdef D_SOUND_ALSA
			printf("  -alsa: use ALSA for sound output\n");
#endif
#ifdef D_SOUND_OSS
			printf("  -oss: use OSS for sound output\n");
#endif
#ifdef D_SOUND_PULSE
			printf("  -pulse: use PulseAudio for sound output (default)\n");
#endif
			printf("  -mini: show screen at 320x240 in a rotated 640x480 screen\n");
			printf("  -rotate: rotate screen clockwise\n");
			printf("  -micro: show screen at 320x240\n");
			printf("  -fs: start FBZX at fullscreen\n");
			printf("  -hw: use hardware buffer (best for console framebuffer)\n");
			printf("  -db: use double buffer\n");
			printf("  -ds: use doublescan (don't emulate TV black stripes)\n");
			printf("  -ss: force singlescan (emulate TV black stripes)\n");
			printf("  -bw: emulate black&white TV set\n");
			printf("  -color: emulate a color TV set\n");
			printf("  -jump N: show one TV refresh and jump over N refreshes (for slow systems)\n");
			printf("   gamefile: an optional .Z80 snapshot or .TAP/.TZX tape file\n\n");
			exit(0);
		} else if(0==strcmp(argv[argumento],"-nosound")) {
			sound_type=SOUND_NO;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-mini")) {
			ordenador.zaurus_mini=1;
			argumento++;
#ifdef D_SOUND_PULSE
		} else if(0==strcmp(argv[argumento],"-pulse")) {
			sound_type=SOUND_PULSEAUDIO;
			argumento++;
#endif
#ifdef D_SOUND_ALSA
		} else if(0==strcmp(argv[argumento],"-alsa")) {
			sound_type=SOUND_ALSA;
			argumento++;
#endif
#ifdef D_SOUND_OSS
		} else if(0==strcmp(argv[argumento],"-oss")) {
			sound_type=SOUND_OSS;
			argumento++;
#endif
		} else if(0==strcmp(argv[argumento],"-rotate")) {
			ordenador.zaurus_mini=2;
			argumento++;
		} else if (0==strcmp(argv[argumento],"-micro")) {
			ordenador.zaurus_mini=3;
			ordenador.text_mini=1;
			argumento++;
		}else if(0==strcmp(argv[argumento],"-fs")) {
			fullscreen=1;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-hw")) {
			hwsurface=1;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-db")) {
			dblbuffer=1;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-ds")) {
			ordenador.dblscan=1;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-bw")) {
			ordenador.bw=1;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-color")) {
			ordenador.bw=0;
			argumento++;
		} else if(0==strcmp(argv[argumento],"-ss")) {
			ordenador.dblscan=0;
			argumento++;
		} else if(0==strncmp(argv[argumento],"-jump",5)) {
			jump_frames=(int)(argv[argumento][5]);
			jump_frames-=48;//???
			argumento++;
			printf ("Jump %d\n",jump_frames);
		} else {
			strcpy(gamefile,argv[argumento]);
			argumento++;
		}
	}
	
	atexit(end_system);
	switch(ordenador.zaurus_mini) {
	case 0:
		init_screen(640,480,0,0,dblbuffer,hwsurface);
	break;
	case 1:
	case 2:
		init_screen(480,640,0,0,dblbuffer,hwsurface);
	break;
	case 3:
		init_screen(320,240,0,0,dblbuffer,hwsurface);
	break;
	}
	printf("Modo: %d\n",ordenador.mode128k);
	register_screen(screen);
	printf("Screen registered\n");
	printf("Modo: %d\n",ordenador.mode128k);
	#ifndef GEKKO
	if(fullscreen) {
		SDL_Fullscreen_Switch();
	}
	SDL_WM_SetCaption("FBZX","");
	#endif
	ordenador.interr=0;

	ordenador.screenbuffer=ordenador.screen->pixels;
	ordenador.screen_width=ordenador.screen->w;
	
	//Init SDL Menu
	
	menu_init(ordenador.screen);
	
	//Load the splash screen
	if (load_zxspectrum_picture()) SDL_FreeSurface (image);
	
	#ifdef GEKKO
	usbismount = InitUSB();
	
	load_config_smb(&ordenador);
	
	if (ordenador.smb_enable) networkisinit = InitNetwork();
	
	if (networkisinit && ordenador.smb_enable) ConnectShare(); 
	
	#endif

	// assign initial values for PATH variables

	strcpy(path_snaps,getenv("HOME"));
	length=strlen(path_snaps);
	if ((length>0)&&(path_snaps[length-1]!='/'))
		strcat(path_snaps,"/");
	strcpy(path_taps,path_snaps);
	strcpy(path_mdrs,path_snaps);
	strcpy(path_scr,path_snaps);
	strcpy(path_confs,path_snaps);
	strcat(path_snaps,"snapshots");
	strcat(path_taps,"tapes");
	strcat(path_mdrs,"microdrives");
	strcat(path_scr,"scr");
	strcat(path_confs,"configurations");
	
	#ifdef GEKKO
	if ((ordenador.port==1)&&usbismount) {
	strcpy(path_snaps,"usb:/");
	strcpy(path_taps,"usb:/");
	}
	if ((ordenador.port==2)&&smbismount) {
	strcpy(path_snaps,"smb:/");
	strcpy(path_taps,"smb:/");
	}	
	#endif
	
	ordenador.current_tap[0]=0;

	// assign random values to the memory before start execution

	printf("Reset memory\n");
	printf("Modo: %d\n",ordenador.mode128k);
	for(bucle=0;bucle<196608;bucle++)
		ordenador.memoria[bucle]=(unsigned char) rand();

	printf("Memory resetted\n");
	ordenador.tap_file=NULL;
	printf("Modo: %d\n",ordenador.mode128k);
	
	// we filter all the events, except keyboard events

	SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);
	SDL_EventState(SDL_JOYAXISMOTION,SDL_IGNORE);
	SDL_EventState(SDL_JOYBALLMOTION,SDL_IGNORE);
	SDL_EventState(SDL_JOYHATMOTION,SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_JOYBUTTONUP,SDL_IGNORE);
	SDL_EventState(SDL_QUIT,SDL_ENABLE);
	SDL_EventState(SDL_SYSWMEVENT,SDL_IGNORE);
	SDL_EventState(SDL_VIDEORESIZE,SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT,SDL_IGNORE);

	SDL_ShowCursor(SDL_DISABLE);
	salir=1;
  
	printf("Init microdrive\n");
	microdrive_init();

	printf("Reset computer\n");
	ResetComputer();

	sleep(1);

	printf("Reset screen\n");
	clean_screen();

	if (sound_aborted==1) {
		strcpy(ordenador.osd_text,"Running without sound (read the FAQ)");
		ordenador.osd_time=100;
	}

	printf("Modo: %d\n",ordenador.mode128k);
	printf("load main game\n");
	load_main_game(gamefile);
	printf("Modo: %d\n",ordenador.mode128k);

	sprintf(ordenador.osd_text,"Press Home for menu");
	ordenador.osd_time=200;

	printf("BPP: %d\n",ordenador.bpp);
	while(salir) {

		do {
			tstados=Z80free_ustep(&procesador);
			if(tstados<0) {
				printf("Error %X\n",procesador.PC);
				exit(1);
			}
			emulate(tstados); // execute the whole hardware emulation for that number of TSTATES
		} while(procesador.Status!=Z80XX);
			
		PC=procesador.PC;
				
		/* if PC is 0x0556, a call to LD_BYTES has been made, so if
		FAST_LOAD is 1, we must load the block in memory and return */

		if((!ordenador.mdr_paged)&&(PC==0x0556) && (ordenador.tape_fast_load==1)&&(ordenador.tape_file_type==TAP_TAP)) {
			if(ordenador.tap_file!=NULL)
				fastload_block(ordenador.tap_file);
			else {
				sprintf(ordenador.osd_text,"No TAP file selected");
				ordenador.osd_time=50;
			}
		}
		
		/* if PC is 0x04C2, a call to SA_BYTES has been made, so if
		we want to save to the TAP file, we do it */
		
		if((!ordenador.mdr_paged)&&(PC==0x04C2)&&(ordenador.tape_write==1)&&(ordenador.tape_file_type==TAP_TAP)) {
			if(ordenador.tap_file!=NULL)
				save_file(ordenador.tap_file);
			else {
				sprintf(ordenador.osd_text,"No TAP file selected");
				ordenador.osd_time=50;
			}
		}
		
		/* if ordenador.mdr_paged is 2, we have executed the RET at 0x0700, so
		we have to return to the classic ROM */
		
		if(ordenador.mdr_paged==2)
			ordenador.mdr_paged=0;
		
		/* if PC is 0x0008 or 0x1708, and we have a microdrive, we have to page
		the Interface 1 ROM */
		
		if(((PC==0x0008)||(PC==0x1708))&&(ordenador.mdr_active))
			ordenador.mdr_paged = 1;
		
		/* if PC is 0x0700 and we have a microdrive, we have to unpage
		the Interface 1 ROM after the last instruction */
		
		if((PC==0x0700)&&(ordenador.mdr_active))
			ordenador.mdr_paged = 2;

		if(ordenador.interr==1) {
			read_keyboard ();	// read the physical keyboard
			Z80free_INT(&procesador,bus_empty());
			ordenador.interr=0;
		}
	}
	
	#ifdef GEKKO
	if (smbismount) CloseShare ();
	DeInitUSB();
	fatUnmount(0);
	#endif
	
	return 0;
}

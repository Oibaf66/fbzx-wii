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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rzx_lib/rzx.h"
#include "cargador.h"
#include "menu_sdl.h"
#include "computer.h"
#include "rzx_init.h"

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

#define FBZXVMAJ 14
#define FBZXVMIN 0

RZX_EMULINFO emul_info;
rzx_u32 tstates;
int rzx_snapshot_counter;
RZX_browser rzx_browser_list[MAX_RZX_BROWSER_ITEM+1];
char extracted_rzx_file[MAX_PATH_LENGTH];

void find_name (char *name, char *filename)
{
 const char *ptr;
 const char *dir = path_snaps;
 char fb[81];

 ptr = strrchr(filename, '\\');
 
 if (ptr==NULL) ptr = strrchr(filename, '/');
 
 if (ptr==NULL) strncpy(fb, filename, 80); else
	strncpy(fb, ptr+1, 80);
	
 snprintf(name, MAX_PATH_LENGTH-1, "%s/%s", dir, fb);
}

rzx_u32 rzx_callback(int msg, void *par)
{
 int retorno;
 
 switch(msg)
 {
  case RZXMSG_LOADSNAP:
		if(rzx.mode==RZX_SCAN)
		{
		  if (rzx_snapshot_counter>=MAX_RZX_BROWSER_ITEM) break;
          if (!(((RZX_SNAPINFO*)par)->options&RZX_EXTERNAL)) rzx_browser_list[rzx_snapshot_counter++].position=((RZX_SNAPINFO*)par)->position; //Embedded snapshot - Position at beginning
		  rzx_browser_list[rzx_snapshot_counter].position=0;
		}
		else
		{
		retorno=0;
		printf("> LOADSNAP: '%s' (%i bytes), %s, %s\n",
              ((RZX_SNAPINFO*)par)->filename,
              (int)((RZX_SNAPINFO*)par)->length,
              (((RZX_SNAPINFO*)par)->options&RZX_EXTERNAL)?"external":"embedded",
              (((RZX_SNAPINFO*)par)->options&RZX_COMPRESSED)?"compressed":"uncompressed");
			  if (((RZX_SNAPINFO*)par)->options&RZX_EXTERNAL) find_name(extracted_rzx_file,((RZX_SNAPINFO*)par)->filename); 
			  else strncpy (extracted_rzx_file, ((RZX_SNAPINFO*)par)->filename,80);
			  
			  if (ordenador.extract_screen_rzx)
			  {
				 ((RZX_SNAPINFO*)par)->options&=~RZX_REMOVE; //We do not want the file removed
			  }
			  else
			  {
				if (ext_matches(extracted_rzx_file, ".z80")|ext_matches(extracted_rzx_file, ".Z80"))	  
					retorno = load_z80(extracted_rzx_file);
				else if (ext_matches(extracted_rzx_file, ".sna")|ext_matches(extracted_rzx_file, ".SNA")) 
					retorno = load_sna(extracted_rzx_file);
				else
					{printf("> Not supported snap format\n");retorno=-1;}
			   }	
		if (retorno)
			{
				printf("> Load snapshot error %d\n", retorno);
				return RZX_INVALID;
			}
		}	
       break;
  case RZXMSG_SAVESNAP:
		printf("> Save snaphsot\n");
		if (rzx_snapshot_counter>=MAX_RZX_BROWSER_ITEM) break;
		rzx_browser_list[rzx_snapshot_counter].position=((RZX_SNAPINFO*)par)->position;
		rzx_browser_list[rzx_snapshot_counter].frames_count = ordenador.total_frames_rzx;
		rzx_browser_list[++rzx_snapshot_counter].position=0;
		break;		   
  case RZXMSG_CREATOR:
		printf("> CREATOR: '%s %d.%d'\n",
              ((RZX_EMULINFO*)par)->name,
			  (int)((RZX_EMULINFO*)par)->ver_major,
              (int)((RZX_EMULINFO*)par)->ver_minor);
		if ((int)((RZX_EMULINFO*)par)->length>0) 
			printf("%s \n", (const char *)((RZX_EMULINFO*)par)->data);
		rzx_snapshot_counter=0;
		rzx_browser_list[0].frames_count=0;
		rzx_browser_list[0].position=0;      
       break;
  case RZXMSG_IRBNOTIFY:
       if(rzx.mode==RZX_PLAYBACK)
       {
         /* fetch the IRB info if needed */
         tstates=((RZX_IRBINFO*)par)->tstates;
		 ordenador.cicles_counter=tstates;
         printf("> IRB notify: tstates=%i, %s, %s\n",(int)tstates,
                   ((RZX_IRBINFO*)par)->options&RZX_COMPRESSED?"compressed":"uncompressed",
				   ((RZX_IRBINFO*)par)->options&RZX_PROTECTED?"protected":"not protected");
       }
       else if(rzx.mode==RZX_RECORD)
       {
         /* fill in the relevant info, i.e. tstates, options */
         ((RZX_IRBINFO*)par)->tstates=ordenador.cicles_counter;
         ((RZX_IRBINFO*)par)->options=0;
         #ifdef RZX_USE_COMPRESSION
         ((RZX_IRBINFO*)par)->options|=RZX_COMPRESSED;
         #endif
         printf("> IRB notify: tstates=%i, %s\n",(int)ordenador.cicles_counter,
                   ((RZX_IRBINFO*)par)->options&RZX_COMPRESSED?"compressed":"uncompressed");
       }
	   else if(rzx.mode==RZX_SCAN)
	   {
	     if ((rzx_snapshot_counter>0)&&(rzx_snapshot_counter<MAX_RZX_BROWSER_ITEM+1)) rzx_browser_list[rzx_snapshot_counter-1].frames_count = ordenador.total_frames_rzx;
		 ordenador.total_frames_rzx += ((RZX_IRBINFO*)par)->framecount;
		 printf("> IRB notify: Total frames to play %d\n", ordenador.total_frames_rzx);
	   }
       break;
  case RZXMSG_IRBCLOSE:
		printf("> IRB close\n");
		if ((rzx_snapshot_counter>0)&&(rzx_snapshot_counter<MAX_RZX_BROWSER_ITEM+1)) 
		ordenador.total_frames_rzx += ((RZX_IRBINFO*)par)->framecount;
		break;	   
  case RZXMSG_SECURITY:
		printf("> Security Information Block\n");
		break;
  case RZXMSG_SEC_SIG:
		printf("> Security Signature Block\n");
		break;		
  case RZXMSG_UNKNOWN:
		printf("> Unknown block\n");
		break;		
  default:
       printf("> MSG #%02X\n",msg);
       return RZX_INVALID;
       break;
 }
 return RZX_OK;
}


///////////////////////////////////////////////////////////////////////////


void init_rzx()
{

 printf("Init RZX library\n");
 printf("Using RZX Library v%i.%02i build %i\n",(RZX_LIBRARY_VERSION&0xFF00)>>8,RZX_LIBRARY_VERSION&0xFF,RZX_LIBRARY_BUILD);
 strcpy(emul_info.name,"RZX FBZX Wii ");
 emul_info.ver_major=FBZXVMAJ;
 emul_info.ver_minor=FBZXVMIN;
 emul_info.data=0; emul_info.length=0;
 emul_info.options=0;
 rzx_init(&emul_info,rzx_callback);

}



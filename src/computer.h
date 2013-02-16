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

#ifndef computer_h
#define computer_h

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

// #define MUT

#define KB_BUFFER_LENGHT 10
#define MAX_PATH_LENGTH 256

extern char salir;

enum tapmodes {TAP_GUIDE, TAP_DATA, TAP_PAUSE, TAP_TRASH, TAP_STOP, TAP_PAUSE2, TZX_PURE_TONE,
	TZX_SEQ_PULSES, TAP_FINAL_BIT, TAP_PAUSE3};
enum taptypes {TAP_TAP, TAP_TZX};

int countdown_buffer;

struct computer {

	unsigned char precision; //If set 1 emulate with more precision
	unsigned char precision_old;
	unsigned char npixels; //1, 2 or 4 depending on dblscan and zaurus_mini
	unsigned char progressive; //interlace or progressive 576
	unsigned int temporal_io;

	// screen private global variables
	SDL_Surface *screen;
	unsigned char *screenbuffer;
	unsigned int screen_width;
	unsigned int translate[6144],translate2[6144];
	unsigned char zaurus_mini;
	unsigned char text_mini;
	unsigned char dblscan;
	unsigned char bw;
	

	int contador_flash;

	unsigned int *p_translt,*p_translt2;
	unsigned char *pixel; // current address
	char border,flash, border_sampled;
	int currline,currpix;

	int tstados_counter; // counts tstates leaved to the next call
	int resx,resy,bpp; // screen resolutions
	int init_line; // cuantity to add to the base address to start to paint
	int next_line; // cuantity to add when we reach the end of line to go to next line
	int next_scanline; // cuantity to add to pass to the next scanline
	int first_line; // first line to start to paint
	int last_line; // last line to paint
	int first_line_kb; // first line to start to paint the keyboard
	int last_line_kb; // last line to paint the keyboard
	int first_pixel; // first pixel of a line to paint
	int last_pixel; // last pixel of a line to paint
	int next_pixel; // next pixel
	int pixancho,pixalto; // maximum pixel value for width and height
	int jump_pixel;
	int upper_border_line; //63 or 62 for 48k or 128k
	int lower_border_line; //upper_border_line + 192
	int start_screen; //Pixel at which the interrupt is generated
	int cpufreq; //frequency CPU
	int tstatodos_frame; //number of tstados per frame
	int pixels_octect; //2 bits in the octect
	int pixels_word; //2 bits in the word
	int start_contention; //start tstados for contention
	//int end_contention; //end tstados for contention
	
	unsigned char screen_snow; // 0-> no emulate snow; 1-> emulate snow
	unsigned char fetch_state;
	//unsigned char contended_zone; // 0-> no contention; 1-> contention possible
	int cicles_counter; // counts how many pixel clock cicles passed since las interrupt

	char ulaplus; // 0 = inactive; 1 = active
	unsigned char ulaplus_reg; // contains the last selected register in the ULAPlus
	unsigned char ulaplus_palete[64]; // contains the current palete

	// keyboard private global variables

	unsigned char s8,s9,s10,s11,s12,s13,s14,s15;
	unsigned char k8,k9,k10,k11,k12,k13,k14,k15;
	unsigned char readed;
	//unsigned char tab_extended;
	unsigned char esc_again;

	// kempston joystick private global variables

	unsigned char js,jk;

	// Linux joystick private global variables

	//unsigned char use_js;
	//unsigned char updown,leftright;

	// sound global variables

	int tst_sample; // number of tstates per sample
	int freq; // frequency for reproduction
	int format; // 0: 8 bits, 1: 16 bits LSB, 2: 16 bits MSB
	signed char sign; // 0: unsigned; -128: signed
	int channels; // number of channels
	int buffer_len; // sound buffer length (in samples)
	int increment; // quantity to add to jump to the next sample
	unsigned char volume; // volume
	unsigned char sample1[4]; // buffer with precalculated sample 1 (for buzzer) -currently not used
	unsigned char sample1b[4]; // buffer with prec. sample 1 (for AY-3-8912) -currently not used
	//unsigned char sample0[4]; // buffer with precalculated sample 0
	unsigned char sound_bit;
	unsigned char sound_bit_mic;
	unsigned int tstados_counter_sound;
	unsigned int low_filter;
	unsigned int *current_buffer;
	unsigned char num_buff;
	unsigned int sound_cuantity; // counter for the buffer
	unsigned char ay_registers[16]; // registers for the AY emulation
	unsigned int aych_a,aych_b,aych_c,aych_n,aych_envel; // counters for AY emulation
	unsigned char ayval_a,ayval_b,ayval_c,ayval_n;
	unsigned char ay_emul; // 0: no AY emulation; 1: AY emulation
	unsigned char audio_mode; //mono, ABC, ACB, BAC
	unsigned int vol_a,vol_b,vol_c;
	unsigned int tst_ay;
	unsigned int ay_latch;
	signed char ay_envel_value;
	unsigned char ay_envel_way;
	//unsigned char sound_current_value;
	unsigned int wr;
	unsigned int r_fetch;
	unsigned int io;
	unsigned int contention;

	// bus global variables

	unsigned char bus_counter;
	unsigned char bus_value;
	unsigned char issue; // 2= 48K issue 2, 3= 48K issue 3
	unsigned char mode128k; // 0=48K, 1=128K, 2=+2, 3=+3 4=sp
	unsigned char videosystem; //0=PAL, 1=NTSC
	unsigned char joystick[2]; // 0=cursor, 1=kempston, 2=sinclair1, 3=sinclair2
	unsigned char port254;


	// tape global variables

	enum tapmodes tape_current_mode;
	unsigned char pause; // 1=tape stop
	enum taptypes tape_file_type;
	unsigned int tape_counter0;
	unsigned int tape_counter1;
	unsigned int tape_counter_rep;
	unsigned char tape_byte;
	unsigned char tape_bit;
	unsigned char tape_readed;
	unsigned int tape_byte_counter;
	unsigned int tape_pause_at_end;
	FILE *tap_file;
	unsigned char tape_fast_load; // 0 normal load; 1 fast load
	unsigned char rewind_on_reset;
	unsigned char pause_instant_load;
	unsigned char current_tap[MAX_PATH_LENGTH];
	unsigned char last_selected_file[MAX_PATH_LENGTH];
	unsigned char last_selected_poke_file[MAX_PATH_LENGTH];

	unsigned char tape_current_bit;
	unsigned int tape_block_level;
	unsigned int tape_sync_level0;
	unsigned int tape_sync_level1;
	unsigned int tape_bit0_level;
	unsigned int tape_bit1_level;
	unsigned char tape_bits_at_end;
	unsigned int tape_loop_counter;
	unsigned int tape_start_countdwn;
	unsigned int pause_fastload_countdwn;
	long tape_loop_pos;

	unsigned char tape_write; // 0 can't write; 1 can write

	// Microdrive global variables
	FILE *mdr_file;                  // Current microdrive file
	unsigned char mdr_current_mdr[MAX_PATH_LENGTH]; // current path and name for microdrive file
	unsigned char mdr_active;	// 0: not installed; 1: installed
	unsigned char mdr_paged;	// 0: not pagined; 1: pagined
	unsigned int mdr_tapehead; // current position in the tape
	unsigned int mdr_bytes;      // number of bytes read or written in this transfer
	unsigned int mdr_maxbytes; // maximum number of bytes to read or write in this transfer
	unsigned int mdr_gap;         // TSTATEs remaining for GAP end
	unsigned int mdr_nogap;      // TSTATEs remaining for next GAP
	unsigned char mdr_cartridge[137923]; // current cartridge
	unsigned char mdr_drive; // current drive
	byte mdr_old_STATUS; // to detect an edge in COM CLK
	unsigned char mdr_modified; // if a sector is stored, this change to know that it must be stored in the file

	// OSD global variables

	unsigned char osd_text[200];
	unsigned int osd_time;

	// pagination global variables

	unsigned char mport1,mport2; // ports for memory management (128K and +3)
	unsigned int video_offset; // 0 for page 5, and 32768 for page 7
	unsigned char *block0,*block1,*block2,*block3; // pointers for memory access (one for each 16K block).

	// public

	unsigned char memoria[196608]; // memory (12 pages of 16K each one). 4 for ROM, and 8 for RAM
	unsigned char shadowrom[8192]; // space for Interface I's ROMs
	unsigned char interr;
	unsigned char readkeyboard;
	unsigned char mustlock;
	unsigned char other_ret; // 0=no change; 1=memory returns RET (201)

	unsigned char turbo;
	unsigned char turbo_state;
	unsigned int keyboard_buffer[2][KB_BUFFER_LENGHT];
	unsigned int kbd_buffer_pointer;
	unsigned char *key;
	unsigned char joystick_number;
	SDL_Joystick *joystick_sdl[2];
	unsigned char joy_axis_x_state[2];
	unsigned char joy_axis_y_state[2];
	unsigned int joybuttonkey[2][23];
	unsigned char joypad_as_joystick[2];
	unsigned char rumble[2];
	unsigned char vk_auto;
	unsigned char vk_rumble;
	unsigned char vk_is_active;
	unsigned char port; //SD, USB, SMB or FTP
	unsigned char smb_enable;
	unsigned char SmbUser[32]; 
	unsigned char SmbPwd[32];
	unsigned char SmbShare[32]; 
	unsigned char SmbIp[32];
	unsigned char ftp_enable;
	unsigned char FTPUser[32]; 
	unsigned char FTPPwd[32];
	unsigned char FTPPath[MAX_PATH_LENGTH]; 
	unsigned char FTPIp[64];
	unsigned char FTPPassive;
	unsigned short FTPPort;
	unsigned char autoconf;
};

void computer_init();
void register_screen(SDL_Surface *);
inline void show_screen(int);
inline void show_screen_precision(int);
inline void paint_pixels(unsigned char, unsigned char, unsigned char, unsigned char draw);
inline void paint_pixels_precision(unsigned char, unsigned char, unsigned char,unsigned char draw);
inline void read_keyboard();
void fill_audio(void *udata,Uint8 *,int);
void set_volume(unsigned char);
inline void play_sound(unsigned int);
inline void emulate_screen(int);
inline void emulate(int);
void ResetComputer();
inline byte bus_empty();
void set_memory_pointers();
inline void play_ay();
inline void paint_one_pixel(unsigned char *colour,unsigned char *address);
void computer_set_palete();
void set_palete_entry(unsigned char entry, byte Value);
void restart_video();
void update_npixels();

#endif

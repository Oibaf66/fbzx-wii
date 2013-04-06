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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL/SDL_audio.h>


//char tabla[1024];

#include <fcntl.h>
#ifndef GEKKO 
//#include <sys/ioctl.h>
#endif

#ifdef DEBUG
extern FILE *fdebug;
#define printf(...) fprintf(fdebug,__VA_ARGS__)
#else
 #ifdef GEKKO
 #define printf(...)
 #endif
#endif

/*#include <SDL/SDL.h>
#include "SDL/SDL_audio.h"
#include "SDL_thread.h"*/

#include "sound.h"

#ifdef D_SOUND_OSS
#include <sys/soundcard.h>
int audio_fd;
#endif

#ifdef D_SOUND_ALSA
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>
int started_sound;
snd_pcm_t * _soundDevice;
#endif

#ifdef D_SOUND_PULSE
#include <pulse/simple.h>
pa_simple *pulse_s;
#endif

#ifdef GEKKO
#include <asndlib.h>
int started_sound_asnd;
#endif

unsigned char started_sound_sdl;
unsigned char buffer0_occupied;
unsigned char buffer1_occupied;
unsigned char buffer_reading;

enum e_soundtype sound_type;

int sound_init() {

	if (sound_type!=SOUND_AUTOMATIC) {
		switch(sound_type) {
		case SOUND_NO: // No sound; simulate 8bits mono
			printf("No Sound\n");
			ordenador.sign=0;
			ordenador.format=1;
			ordenador.channels=2;
			ordenador.freq=48000;
			ordenador.buffer_len=4800; // will wait 1/10 second
			return (0);
		break;
		case SOUND_SDL:
			printf("Trying SDL sound\n");
			if(0==sound_init_sdl()) {
				sound_type=SOUND_SDL;
				return 0;
			} else {
				printf("Failed\n");
				return -1;
			}
		break;
#ifdef D_SOUND_PULSE
		case SOUND_PULSEAUDIO:
			printf("Trying PulseAudio\n");
			if(0==sound_init_pulse()) {
				sound_type=SOUND_PULSEAUDIO;
				return 0;
			} else {
				printf("Failed\n");
				return -1;
			}
		break;
#endif
#ifdef D_SOUND_ALSA
		case SOUND_ALSA:
			printf("Trying ALSA sound\n");
			if(0==sound_init_alsa()) {
				sound_type=SOUND_ALSA;
				return 0;
			} else {
				printf("Failed\n");
				return -1;
			}
		break;
#endif
#ifdef D_SOUND_OSS
		case SOUND_OSS:
			printf("Trying OSS sound\n");
			if(0==sound_init_oss()) {
				sound_type=SOUND_OSS;
				return 0;
			} else {
				printf("Failed\n");
				return -1;
			}
		break;
#endif
#ifdef GEKKO
		case SOUND_ASND:
			printf("Trying ASND sound\n");
			if(0==sound_init_asnd()) {
				sound_type=SOUND_ASND;
				return 0;
			} else {
				printf("Failed\n");
				return -1;
			}
		break;
#endif

		default:
		break;
		}
	}
	
printf("Trying SDL sound\n");
	if(0==sound_init_sdl()) {
		sound_type=SOUND_SDL;
		return 0;
	}	
	
#ifdef D_SOUND_PULSE
	printf("Trying PulseAudio\n");
	if(0==sound_init_pulse()) {
		sound_type=SOUND_PULSEAUDIO;
		return 0;
	}
#endif

#ifdef D_SOUND_ALSA
	printf("Trying ALSA sound\n");
	if(0==sound_init_alsa()) {
		sound_type=SOUND_ALSA;
		return 0;
	}
#endif

#ifdef D_SOUND_OSS
	printf("Trying OSS sound\n");
	if(0==sound_init_oss()) {
		sound_type=SOUND_OSS;
		return 0;
	}
#endif

#ifdef GEKKO
	printf("Trying ASND sound\n");
	if(0==sound_init_asnd()) {
		sound_type=SOUND_ASND;
		return 0;
	}
#endif
	return -1;
}

void sdlcallback(void *unused, Uint8 *stream, int len)
{
	int i;
	len=len/4;

	for(i=0;i<len;i++) //len = ordenador.buffer_len
	{
		if (buffer_reading==0)
			{
			*((unsigned int *)stream) =sound[0][i];
			sound[0][i]=0;
			}
		else
			{
			*((unsigned int *)stream) =sound[1][i];
			sound[1][i]=0;
			}
		stream=stream+4;
	}
		
	//change reading buffer
	if (buffer_reading==0) {buffer0_occupied=0;if (buffer1_occupied) buffer_reading=1;} 
	else {buffer1_occupied=0;if (buffer0_occupied) buffer_reading=0;}

}

int sound_init_sdl() {
	
	SDL_AudioSpec fmt;

	ordenador.sign=0;
	ordenador.format=1; //16 bit
	ordenador.channels=2; //stereo
	ordenador.freq=48000;
	ordenador.buffer_len=4096;
	
	 /* Set 16-bit stereo audio at 48Khz */
    fmt.freq = ordenador.freq;
    fmt.format = AUDIO_U16LSB; //unsigned Little endian
    fmt.channels = ordenador.channels;
    fmt.samples = ordenador.buffer_len; //number of samples
    fmt.callback = sdlcallback;
    fmt.userdata = NULL;
	
	started_sound_sdl=0;

    /* Open the audio device and start playing sound! */
    if (SDL_OpenAudio(&fmt, NULL) < 0 ) return -1;

	return 0;
}

#ifdef GEKKO
int voice;
void callback(voice)
{}

int sound_init_asnd() {
	
	ASND_Init();
	ASND_Pause(0);
	ordenador.sign=0;
	ordenador.format=1; //16 bit
	ordenador.channels=2; //stereo
	ordenador.freq=48000;
	ordenador.buffer_len=4096;
	started_sound_asnd = 0;
	return 0;

}
#endif

#ifdef D_SOUND_PULSE
int sound_init_pulse() {

	pa_sample_spec ss;
	pa_buffer_attr buf;

	ss.format = PA_SAMPLE_U8;
	ss.channels = 1;
	ss.rate = 48000;

	buf.maxlength=8192;
	buf.tlength=4096;
	buf.prebuf=4096;
	buf.minreq=4096;
	buf.fragsize=4096;

	pulse_s = pa_simple_new(NULL,"fbzx",PA_STREAM_PLAYBACK,NULL,"Spectrum",&ss,NULL,&buf,NULL);
	if (pulse_s==NULL) {
		return -1;
	}
	ordenador.sign=0;
	ordenador.format=0;
	ordenador.channels=1;
	ordenador.freq=48000;
	ordenador.buffer_len=4096;

	return 0;

}
#endif

#ifdef D_SOUND_ALSA
int sound_init_alsa() {
	
	int err;
	snd_pcm_hw_params_t *hw_params;
		
	unsigned int resample,samplerate;
	snd_pcm_uframes_t bufferSize;
	
	err = snd_pcm_open( &_soundDevice, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0 );
	if (err<0) {
		return -1;
	}
	
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		snd_pcm_close (_soundDevice);
		return -2;
	}

	if ((err = snd_pcm_hw_params_any (_soundDevice, hw_params)) < 0) {
		snd_pcm_close (_soundDevice);
		return -2;
	}

		
	if ((err = snd_pcm_hw_params_set_access (_soundDevice, hw_params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		snd_pcm_close (_soundDevice);
		return -3;
	}
	
	if (snd_pcm_hw_params_set_format (_soundDevice, hw_params,SND_PCM_FORMAT_U8) >= 0) {
		ordenador.sign=0;
	    ordenador.format=0;
	} else if (snd_pcm_hw_params_set_format (_soundDevice, hw_params,SND_PCM_FORMAT_S8) >= 0) {
		ordenador.sign=-128;
	    ordenador.format=0;
	} else if (snd_pcm_hw_params_set_format (_soundDevice, hw_params,SND_PCM_FORMAT_U16_LE) >= 0) {
		ordenador.sign=0;
	    ordenador.format=1;
	} else if (snd_pcm_hw_params_set_format (_soundDevice, hw_params,SND_PCM_FORMAT_S16_LE) >= 0) {
		ordenador.sign=-128;
	    ordenador.format=1;
	} else if (snd_pcm_hw_params_set_format (_soundDevice, hw_params,SND_PCM_FORMAT_U16_BE) >= 0) {
		ordenador.sign=0;
	    ordenador.format=2;
	} else if (snd_pcm_hw_params_set_format (_soundDevice, hw_params,SND_PCM_FORMAT_S16_BE) >= 0) {
		ordenador.sign=-128;
	    ordenador.format=2;
	} else {
		snd_pcm_close (_soundDevice);
		return -3;
	}
	
	// Disable resampling.
	resample = 0;
	err = snd_pcm_hw_params_set_rate_resample(_soundDevice, hw_params, resample);
	if (err < 0) {
		snd_pcm_close (_soundDevice);
		return -3;
	}
	
	if ((err = snd_pcm_hw_params_set_channels (_soundDevice, hw_params, 1)) >= 0) {
		ordenador.channels=1;
	} else if ((err = snd_pcm_hw_params_set_channels (_soundDevice, hw_params, 2)) >= 0) {
		ordenador.channels=2;
	} else {
		snd_pcm_close (_soundDevice);
		return -3;
	}
	
	samplerate=48000;
	if ((err = snd_pcm_hw_params_set_rate_near (_soundDevice, hw_params, &samplerate, 0)) < 0) {
		snd_pcm_close (_soundDevice);
		return -3;
	}

	bufferSize=4096;
	if (snd_pcm_hw_params_set_buffer_size_near(_soundDevice, hw_params, &bufferSize) < 0) {
		fprintf(stderr, "Error setting buffersize.\n");
		return -3;
	}

	ordenador.freq=samplerate;
	
	err = snd_pcm_hw_params (_soundDevice, hw_params);
	if (err<0) {
		return -3;
	}
	//snd_pcm_hw_params_get_buffer_size( hw_params, &bufferSize );

	ordenador.buffer_len=bufferSize;
	
	started_sound=0;
	return 0;

	err = snd_pcm_prepare (_soundDevice);
	if (err<0) {
		return -5;
	}
	return 0;
}
#endif

#ifdef D_SOUND_OSS
int sound_init_oss() {
	
	int parameter;
	int parameter2;
	int bytes,stereo;
	int retval;
	audio_buf_info bi;

	audio_fd=open("/dev/dsp",O_WRONLY); // open DSP
	if(audio_fd==-1) {
		return (-1);
	}

	ordenador.sign=0;
	ordenador.format=0;
	bytes=1;
	ordenador.channels=1;
	ordenador.freq=22050;
	ordenador.buffer_len=2048/(ordenador.channels*bytes);

	parameter=0x0002000C ; // two buffers with 4096 bytes each one
	
	if(ioctl(audio_fd,SNDCTL_DSP_SETFRAGMENT, &parameter)==-1)
		return (-6);

	// set format
	
	if(ioctl(audio_fd,SNDCTL_DSP_GETFMTS, &parameter2)==-1)
		return (-2);
	parameter = 2; // we want mono audio
	if(ioctl(audio_fd,SNDCTL_DSP_CHANNELS, &parameter)==-1)
		return (-4);

	ordenador.channels = parameter;
	
	// Priority: U8, S8, U16LE, S16LE, U16BE, U16LE
	
	if(parameter2 & AFMT_S16_BE) {
		parameter = AFMT_S16_BE;
	}
	if(parameter2 & AFMT_U16_BE){
		parameter = AFMT_U16_BE;
	}
	if(parameter2 & AFMT_S16_LE) {
		parameter = AFMT_S16_LE;
	}
	if(parameter2 & AFMT_U16_LE) {
		parameter = AFMT_U16_LE;
	}
	if(parameter2 & AFMT_S8) {
		parameter = AFMT_S8;
	}
	if(parameter2 & AFMT_U8) {
		parameter = AFMT_U8;
	}
	
	bytes=0; //8 bits
	stereo=0; // no stereo

	retval=ioctl(audio_fd,SNDCTL_DSP_SETFMT,&parameter);
	if(retval != 0) {
		return (-3);
	}
	
	switch(parameter) {
		case AFMT_U8:
	    	ordenador.sign=0;
	    	ordenador.format=0;
	    	bytes=1;
    	break;
		case AFMT_S8:
	    	ordenador.sign=-128;
    		ordenador.format=0;
    		bytes=1;
    	break;
		case AFMT_U16_LE:
		    ordenador.sign=0;
	    	ordenador.format=1;
			bytes=2;			
    	break;
		case AFMT_S16_LE:
	    	ordenador.sign=-128;
    		ordenador.format=1;
    		bytes=2;
    	break;
		case AFMT_U16_BE:
	    	ordenador.sign=0;
    		ordenador.format=2;
    		bytes=2;
    	break;
		case AFMT_S16_BE:
		    ordenador.sign=-128;
	    	ordenador.format=2;
	    	bytes=2;
    	break;
	}
	
	parameter=48000; // we want, by default, 48000 samples per second
	if(ioctl(audio_fd,SNDCTL_DSP_SPEED, &parameter)==-1)
		return (-5);
	ordenador.freq=parameter;

	if(ioctl(audio_fd,SNDCTL_DSP_GETOSPACE, &bi)==-1)
		return (-6);

	parameter=bi.bytes/4;
	ordenador.buffer_len=parameter/(ordenador.channels*bytes);
	return(0);	

}
#endif


void sound_remove_dc(unsigned char *sound_buffer,int size){
	static float oldsample=0.0;
	static float origsample;
	static int i;

	for (i=0;i<size;i++) {
		origsample=((float)(sound_buffer[i]))-128;
		oldsample=(origsample+oldsample*999.0)*0.001+1e-6;
		sound_buffer[i]=(int)(origsample-oldsample)*0.98+128;
	}
}

void sound_play() {


	static int retval,retval2;

	ordenador.current_buffer=sound[0];
	//remove_dc(ordenador.current_buffer,ordenador.buffer_len);

	switch(sound_type) {
	case SOUND_NO: // no sound
		usleep(75000); // wait 1/20 second
		return;
	break;
	case SOUND_SDL: // SDL
		if (!started_sound_sdl) {
		SDL_PauseAudio(0);
		ordenador.current_buffer = sound[0];
		buffer0_occupied=1;
		started_sound_sdl = 1;
		buffer_reading=0;
		}
		//Double buffer
		while ((buffer0_occupied)&&(buffer1_occupied)){usleep(1000);}; //Wait for one buffer to be free
		if (!buffer0_occupied) //Buffer 0 is now free
			{buffer0_occupied=1;
			ordenador.current_buffer = sound[0]; }
		else //Buffer 1 is now free
			{buffer1_occupied=1;
			ordenador.current_buffer = sound[1]; }
		return;
	break;
#ifdef D_SOUND_OSS
	case SOUND_OSS: // OSS
		retval=write(audio_fd,ordenador.current_buffer,ordenador.buffer_len*ordenador.increment);
		return;
	break;
#endif
#ifdef D_SOUND_ALSA
	case SOUND_ALSA: // ALSA
		if (started_sound==0) {
			snd_pcm_prepare (_soundDevice);
			started_sound=1;
		}
		retval=snd_pcm_writei(_soundDevice,sound[0],ordenador.buffer_len);
		if (retval<0) {
			retval=snd_pcm_prepare(_soundDevice);
		}
		return;
	break;
#endif
#ifdef D_SOUND_PULSE
	case SOUND_PULSEAUDIO: // PulseAudio
		// Remove the DC component to avoid losing the sound when multiplexing with other sources
		sound_remove_dc(ordenador.current_buffer,ordenador.buffer_len);
		retval=pa_simple_write(pulse_s,sound[0],ordenador.buffer_len,&retval2);
		return;
	break;
#endif
#ifdef GEKKO
	case SOUND_ASND: // ASND
		if (!started_sound_asnd) {
		ASND_SetVoice(1,VOICE_STEREO_16BIT_BE,ordenador.freq,0,sound[0],ordenador.buffer_len*ordenador.increment,
		255, 255, callback);
		started_sound_asnd = 1;
		}
		//Double buffer
		while (!ASND_TestVoiceBufferReady(1)){}; //Wait for one buffer to be free
		if (!ASND_TestPointer (1, sound[0])) 
			{ASND_AddVoice(1,sound[0],ordenador.buffer_len*ordenador.increment);
			ordenador.current_buffer = sound[0]; }
		else 
			{ASND_AddVoice(1,sound[1],ordenador.buffer_len*ordenador.increment);
			ordenador.current_buffer = sound[1]; }
	
		return;
	break;
#endif

	default:
	break;
	}
}


void sound_close() {

	switch(sound_type) {
	case SOUND_NO:
	break;
	case SOUND_SDL:
	SDL_CloseAudio();
	break;
#ifdef D_SOUND_OSS
	case SOUND_OSS:
		close(audio_fd);
	break;
#endif
#ifdef D_SOUND_ALSA
	case SOUND_ALSA:
		snd_pcm_drain (_soundDevice);
		snd_pcm_close (_soundDevice);
	break;
#endif
#ifdef D_SOUND_PULSE
	case SOUND_PULSEAUDIO:
		pa_simple_free(pulse_s);
	break;
#endif
#ifdef GEKKO
	case SOUND_ASND:
		ASND_End();
	break;
#endif

	default:
	break;
	}
}

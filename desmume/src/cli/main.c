/* main.c - this file is part of DeSmuME
 *
 * Copyright (C) 2006,2007 DeSmuME Team
 * Copyright (C) 2007 Pascal Giard (evilynux)
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/time.h>
#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifndef VERSION
#define VERSION "Unknown version"
#endif

#ifndef CLI_UI
#define CLI_UI
#endif

#include "../MMU.h"
#include "../NDSSystem.h"
#include "../cflash.h"
#include "../sndsdl.h"
#include "../ctrlssdl.h"
#include "../render3D.h"
#ifdef GKD350H
#include "scalers.h"
#endif
//#include "../opengl_collector_3Demu.h"

uint_fast8_t sdl_quit = 0;
volatile BOOL execute = FALSE;
static float nds_screen_size_ratio = 1.0f;
SDL_Surface * sdl_screen, *rl_screen;

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
  &SNDFile,
  &SNDSDL,
  NULL
};

GPU3DInterface *core3DList[] = {
//&gpu3D_opengl_collector
&gpu3DNull
};


/* Our keyboard config is different because of the directional keys */
const u16 cli_kb_cfg[NB_KEYS] =
  { SDLK_LCTRL,         // A
    SDLK_LALT,         // B
    SDLK_BACKSPACE, // select
    SDLK_RETURN,    // start
    SDLK_RIGHT,     // Right
    SDLK_LEFT,      // Left
    SDLK_UP,        // Up
    SDLK_DOWN,      // Down
    SDLK_BACKSPACE,         // R
    SDLK_TAB,         // L
    SDLK_LSHIFT,         // X
    SDLK_SPACE,         // Y
    SDLK_p,         // DEBUG
    SDLK_o          // BOOST
  };


static void Draw( void)
{
#ifdef GKD350H
	scale_256x384_to_160x240((uint32_t* restrict)rl_screen->pixels, (uint32_t* restrict)sdl_screen->pixels);
	SDL_Flip(rl_screen);
#else
#ifdef SDL_SWIZZLEBGR
	SDL_Flip(sdl_screen);
#else
	SDL_BlitSurface(sdl_screen, NULL, rl_screen, NULL);
	SDL_Flip(rl_screen);
#endif
#endif
	return;
}

#ifdef FRAMESKIP
static uint32_t Timer_Read(void) 
{
	/* Timing. */
	struct timeval tval;
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec)));
}
static long lastTick = 0, newTick;
static uint32_t SkipCnt = 0, FPS = 60, FrameSkip, video_frames = 0;
static const uint32_t TblSkip[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 1, 1},
    {0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1},
};
#endif



int main(int argc, char ** argv) {
	static unsigned short keypad = 0;
	u32 last_cycle = 0;

#ifdef DISPLAY_FPS
	u32 fps_timing = 0;
	u32 fps_frame_counter = 0;
	u32 fps_previous_time = 0;
	u32 fps_temp_time;
	#define NUM_FRAMES_TO_TIME 60
#endif

	/* the firmware settings */
	struct NDS_fw_config_data fw_config;

	/* default the firmware settings, they may get changed later */
	NDS_FillDefaultFirmwareConfigData( &fw_config);

	fw_config.language = 1;

	NDS_Init();

	/* Create the dummy firmware */
	NDS_CreateDummyFirmware( &fw_config);

	SPU_ChangeSoundCore(SNDCORE_SDL, 735 * 4);

	if (NDS_LoadROM(argv[1], MC_TYPE_AUTODETECT, 1, 0) < 0) {
		fprintf(stderr, "error while loading %s\n", argv[1]);
		exit(-1);
	}
	
	execute = TRUE;

	if(SDL_Init(SDL_INIT_VIDEO) == -1)
    {
      fprintf(stderr, "Error trying to initialize SDL: %s\n",
              SDL_GetError());
      return 1;
    }
#if defined(GKD350H)
	rl_screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
	sdl_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 256, 384, 16, 0x001F, 0x03E0, 0x7C00, 0);
#else
#ifdef SDL_SWIZZLEBGR
	sdl_screen = SDL_SetVideoMode(256, 384, 16, SDL_HWSURFACE | SDL_TRIPLEBUF | SDL_SWIZZLEBGR);
#else
	rl_screen = SDL_SetVideoMode(256, 384, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
	sdl_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 256, 384, 16, 0x001F, 0x03E0, 0x7C00, 0);
#endif
#endif

    if ( !sdl_screen ) {
      fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError( ) );
      exit( -1);
    }
    
	SDL_WM_SetCaption("Desmume SDL", NULL);

	/* Initialize joysticks */
	if(!init_joy()) return 1;
	/* Load our own keyboard configuration */
	set_kb_keys(cli_kb_cfg);

	while(!sdl_quit)
	{
		/* Look for queued events and update keypad status */
		sdl_quit = process_ctrls_events( &keypad, NULL, nds_screen_size_ratio);
		/* Update mouse position and click */
		if(mouse.down) NDS_setTouchPos(mouse.x, mouse.y);
		if(mouse.click)
		{ 
			NDS_releasTouch();
			mouse.click = FALSE;
		}

		update_keypad(keypad);     /* Update keypad */
		
#ifdef FRAMESKIP
		SkipCnt++;
		if (SkipCnt > 4) SkipCnt = 0;
		last_cycle = NDS_exec((560190 << 1) - last_cycle, FALSE, TblSkip[FrameSkip][SkipCnt]);
#else
		last_cycle = NDS_exec((560190 << 1) - last_cycle, FALSE, 0);
#endif
		SPU_Emulate();
		Draw();
		
#ifdef FRAMESKIP
	newTick = Timer_Read();
	if ( (newTick) - (lastTick) > 1000000) 
	{
		FPS = video_frames;
		video_frames = 0;
		lastTick = newTick;
		
		FrameSkip = 4;
		if (FPS >= 60)
		{
			FrameSkip = 0;
		}
		else if (FPS >= 45)
		{
			FrameSkip = 3;
		}
	}
#endif

#ifdef DISPLAY_FPS
		fps_frame_counter += 1;
		fps_temp_time = SDL_GetTicks();
		fps_timing += fps_temp_time - fps_previous_time;
		fps_previous_time = fps_temp_time;

		if ( fps_frame_counter == NUM_FRAMES_TO_TIME)
		{
			char win_title[100];
			float fps = (float)fps_timing;
			fps /= NUM_FRAMES_TO_TIME * 1000.f;
			fps = 1.0f / fps;
			fps_frame_counter = 0;
			fps_timing = 0;
			sprintf( win_title, "Desmume %f", fps);
			SDL_WM_SetCaption( win_title, NULL);
		}
#endif
	}

	/* Unload joystick */
	uninit_joy();
	SDL_Quit();
	NDS_DeInit();
	return 0;
}

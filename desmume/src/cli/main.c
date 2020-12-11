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

#include "../MMU.h"
#include "../NDSSystem.h"
#include "../cflash.h"
#include "../sndsdl.h"
#include "../ctrlssdl.h"
#include "../render3D.h"
#if defined(GKD350H) || defined(FUNKEY)
#include "scalers.h"
#endif
//#include "../opengl_collector_3Demu.h"

uint_fast8_t sdl_quit = 0;
volatile BOOL execute = FALSE;
static float nds_screen_size_ratio = 1.0f;
SDL_Surface * sdl_screen;
SDL_Surface *rl_screen;
SDL_Surface *cursor_sdl;

SoundInterface_struct *SNDCoreList[] = {
  &SNDSDL
};

#ifdef _3DRENDERING
GPU3DInterface *core3DList[] = {
//&gpu3D_opengl_collector
&gpu3DNull
};
#endif

#ifdef FRAMESKIP
static uint32_t Timer_Read(void) 
{
	/* Timing. */
	struct timeval tval;
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec)));
}
static long lastTick = 0, newTick;
static uint32_t SkipCnt = 0, FPS = 60, FrameSkip = 4, video_frames = 0;
static const uint32_t TblSkip[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 1, 1},
    {0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1},
};
#endif


#ifdef GKD350H
uint_fast8_t fullscreen_option = 0;
void Set_Offset(void)
{
	uint_fast8_t i;
	switch(fullscreen_option)
	{
		case 0:
			MOUSE_X_OFFSET = 80;
			MOUSE_Y_OFFSET = 120;
			RESOLUTION_WIDTH = 160.0f;
			RESOLUTION_HEIGHT = 120.0f;
		break;
		case 1:
			MOUSE_X_OFFSET = 32;
			MOUSE_Y_OFFSET = 48;
			RESOLUTION_WIDTH = 256.0f;
			RESOLUTION_HEIGHT = 192.0f;
		break;
		case 2:
			MOUSE_X_OFFSET = 32;
			MOUSE_Y_OFFSET = 48;
			RESOLUTION_WIDTH = 256.0f;
			RESOLUTION_HEIGHT = 192.0f;
		break;
	}
	for(i=0;i<3;i++)
	{
		SDL_FillRect(rl_screen, NULL, 0);
		SDL_Flip(rl_screen);
	}
}
#endif

static void Draw( void)
{
#ifdef GKD350H
	SDL_Rect rct;
	SDL_Rect rct2, pos;
	rct2.x = 0;
	rct2.y = 192-48;
	rct2.w = 256;
	rct2.h = 240;
	pos.x = 32;
	pos.y = 0;
	rct.x = emulated_touch_x;
	rct.y = emulated_touch_y;
#ifdef FRAMESKIP
	if (!TblSkip[FrameSkip][SkipCnt])
#endif
	{
		switch(fullscreen_option)
		{
			default:
				scale_256x384_to_160x240((uint32_t* restrict)rl_screen->pixels, (uint32_t* restrict)sdl_screen->pixels);
			break;
			#ifndef FUNKEY
			case 1:
				SDL_BlitSurface(sdl_screen, NULL, rl_screen, &pos);
			break;
			case 2:
				SDL_BlitSurface(sdl_screen, &rct2, rl_screen, &pos);
			break;
			#endif
		}
		if (mouse_mode) SDL_BlitSurface(cursor_sdl, NULL, rl_screen, &rct);
		SDL_Flip(rl_screen);
	}
#elif defined(FUNKEY)
	SDL_Rect rct;
	rct.x = emulated_touch_x;
	rct.y = emulated_touch_y;
#ifdef FRAMESKIP
	if (!TblSkip[FrameSkip][SkipCnt])
#endif
	{
		scale_256x384_to_160x240((uint32_t* restrict)rl_screen->pixels, (uint32_t* restrict)sdl_screen->pixels);
		if (mouse_mode) SDL_BlitSurface(cursor_sdl, NULL, rl_screen, &rct);
		SDL_Flip(rl_screen);
	}
#ifdef SDL_SWIZZLEBGR
	SDL_Rect rct;
	rct.x = emulated_touch_x;
	rct.y = emulated_touch_y;
#ifdef FRAMESKIP
	if (!TblSkip[FrameSkip][SkipCnt])
#endif
	{
		if (mouse_mode) SDL_BlitSurface(cursor_sdl, NULL, sdl_screen, &rct);
		SDL_Flip(sdl_screen);
	}
#else
	SDL_BlitSurface(sdl_screen, NULL, rl_screen, NULL);
	SDL_Flip(rl_screen);
#endif
#endif
	return;
}

static void Cleanup_emu(void)
{
#if defined(SDL_SWIZZLEBGR) || !defined(GKD350H) && !defined(FUNKEY)
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
#else
	if (rl_screen) SDL_FreeSurface(rl_screen);
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
#endif

#if defined(GKD350H) || defined(SDL_SWIZZLEBGR) || defined(FUNKEY)
	if (cursor_sdl) SDL_FreeSurface(cursor_sdl);
#endif
	/* Unload joystick */
	uninit_joy();
	SDL_Quit();
	NDS_DeInit();
}

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

	if(SDL_Init(SDL_INIT_VIDEO) == -1)
    {
		fprintf(stderr, "Error trying to initialize SDL: %s\n", SDL_GetError());
		return 1;
    }
#if defined(GKD350H)
	SDL_ShowCursor(0);
	rl_screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
	sdl_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 256, 384, 16, 0x001F, 0x03E0, 0x7C00, 0);
#elif defined(FUNKEY)
	SDL_ShowCursor(0);
	rl_screen = SDL_SetVideoMode(240, 240, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
	sdl_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 256, 384, 16, 0x001F, 0x03E0, 0x7C00, 0);
#else
#ifdef SDL_SWIZZLEBGR
	SDL_ShowCursor(0);
	sdl_screen = SDL_SetVideoMode(256, 384, 15, SDL_HWSURFACE | SDL_TRIPLEBUF | SDL_SWIZZLEBGR);
#else
	rl_screen = SDL_SetVideoMode(256, 384, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
	sdl_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 256, 384, 16, 0x001F, 0x03E0, 0x7C00, 0);
#endif
#endif

	if (!sdl_screen)
	{
		fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError( ) );
		return 1;
    }
    
    #if defined(GKD350H) || defined(SDL_SWIZZLEBGR) || defined(FUNKEY)
    SDL_Surface* tmp = SDL_LoadBMP("cursor.bmp");
    SDL_SetColorKey(tmp, (SDL_SRCCOLORKEY | SDL_RLEACCEL), SDL_MapRGB(tmp->format, 255, 0, 0));
	cursor_sdl = SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);
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

	if (NDS_LoadROM(argv[1], MC_TYPE_AUTODETECT, 1, 0) < 0)
	{
		Cleanup_emu();
		fprintf(stderr, "error while loading %s\n", argv[1]);
		return 1;
	}
	
	execute = TRUE;

	/* Initialize joysticks */
	if(!init_joy()) return 1;
	
	#ifdef GKD350H
	Set_Offset();
	#endif

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
		last_cycle = NDS_exec((560190 << 1) - last_cycle, FALSE);
#endif
		SPU_Emulate();
		Draw();

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
	Cleanup_emu();
	return 0;
}

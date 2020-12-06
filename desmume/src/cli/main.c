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
//#include "../opengl_collector_3Demu.h"

volatile BOOL execute = FALSE;
static float nds_screen_size_ratio = 1.0f;
static SDL_Surface * surface;

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
	SDL_Surface *rawImage;
	rawImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_screen, 256, 384, 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(rawImage == NULL) return;
	SDL_BlitSurface(rawImage, 0, surface, 0);
	SDL_UpdateRect(surface, 0, 0, 0, 0);
	SDL_FreeSurface(rawImage);
	return;
}


int main(int argc, char ** argv) {
  static unsigned short keypad = 0;
  u32 last_cycle = 0;
  int sdl_quit = 0;

  /* this holds some info about our display */
  const SDL_VideoInfo *videoInfo;

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

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
      fprintf(stderr, "Error trying to initialize SDL: %s\n",
              SDL_GetError());
      return 1;
    }

    surface = SDL_SetVideoMode(256, 384, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
  
    if ( !surface ) {
      fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError( ) );
      exit( -1);
    }
    
	SDL_WM_SetCaption("Desmume SDL", NULL);

	/* Fetch the video info */
	videoInfo = SDL_GetVideoInfo( );
	if ( !videoInfo ) {
		fprintf( stderr, "Video query failed: %s\n", SDL_GetError( ) );
		exit( -1);
	}

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
		last_cycle = NDS_exec((560190 << 1) - last_cycle, FALSE);
		SPU_Emulate();
		Draw();
	}

	/* Unload joystick */
	uninit_joy();
	SDL_Quit();
	NDS_DeInit();
	return 0;
}

/* joysdl.c - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Pascal Giard
 *
 * Author: Pascal Giard <evilynux@gmail.com>
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

#include "ctrlssdl.h"

#define DEADZONE_JOYSTICK 8192

static int_fast16_t scaled_x;
static int_fast16_t scaled_y;

#if (defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
extern SDL_Surface* sdl_screen;
int_fast16_t emulated_touch_x = 82;
int_fast16_t emulated_touch_y = 122;
uint_fast8_t mouse_mode = 0;
#ifdef GKD350H
uint_fast8_t MOUSE_X_OFFSET;
uint_fast8_t MOUSE_Y_OFFSET;
float RESOLUTION_WIDTH;
float RESOLUTION_HEIGHT;
extern void Set_Offset(void);
extern uint_fast8_t fullscreen_option;
#elif defined(FUNKEY)
#define MOUSE_X_OFFSET 40
#define MOUSE_Y_OFFSET 120
#define RESOLUTION_WIDTH 160.0f
#define RESOLUTION_HEIGHT 120.0f
#else
#define MOUSE_X_OFFSET 0
#define MOUSE_Y_OFFSET 192
#define RESOLUTION_WIDTH 256.0f
#define RESOLUTION_HEIGHT 192.0f
#endif
#endif


struct mouse_status mouse;
SDL_Joystick * sdl_joy;

/* Initialize joysticks */
BOOL init_joy( void)
{
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
    {
		fprintf(stderr, "Error trying to initialize joystick support: %s\n", SDL_GetError());
		return 0;
	}
	sdl_joy = SDL_JoystickOpen(0);
	SDL_JoystickEventState(SDL_ENABLE);
	return 1;
}

/* Unload joysticks */
void uninit_joy( void)
{
	SDL_JoystickClose(sdl_joy);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

#if !(defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
static int_fast16_t
screen_to_touch_range_x( int_fast16_t scr_x, float size_ratio) {
  int_fast16_t touch_x = (int_fast16_t)((float)scr_x * size_ratio);

  return touch_x;
}

static int_fast16_t
screen_to_touch_range_y( int_fast16_t scr_y, float size_ratio) {
  int_fast16_t touch_y = (int_fast16_t)((float)scr_y * size_ratio);

  return touch_y;
}
#endif

/* Set mouse coordinates */
void set_mouse_coord(int_fast16_t x, int_fast16_t y)
{
	if(x<0) x = 0; else if(x>255) x = 255;
	if(y<0) y = 0; else if(y>192) y = 192;
	mouse.x = x;
	mouse.y = y;
}

/* Update NDS keypad */
void update_keypad(u16 keys)
{
	((u16 *)ARM9Mem.ARM9_REG)[0x130>>1] = ~keys & 0x3FF;
	((u16 *)MMU.ARM7_REG)[0x130>>1] = ~keys & 0x3FF;
	/* Update X and Y buttons */
	MMU.ARM7_REG[0x136] = ( ~( keys >> 10) & 0x3 ) | (MMU.ARM7_REG[0x136] & ~0x3);
}

/* Retrieve current NDS keypad */
u16 get_keypad( void)
{
	u16 keypad;
	keypad = ~MMU.ARM7_REG[0x136];
	keypad = (keypad & 0x3) << 10;
	keypad |= ~((u16 *)ARM9Mem.ARM9_REG)[0x130>>1] & 0x3FF;
	return keypad;
}

#if defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY)
static int_fast16_t xaxis = 0;
static int_fast16_t yaxis = 0;
#endif

/* Manage input events */
int
process_ctrls_events( u16 *keypad,
                      void (*external_videoResizeFn)( u16 width, u16 height),
                      float nds_screen_size_ratio)
{
	int cause_quit = 0;
	SDL_Event event;
	
	#if (defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
	uint8_t* keys;
	keys = SDL_GetKeyState(NULL);
	/* Required for say, holding the save slot in Bomberman DS */
	if (mouse_mode == 1)
	{
		if (keys[SDLK_LCTRL])
		{
			mouse.click = 0;
			mouse.down = 1;
			scaled_x = (emulated_touch_x - MOUSE_X_OFFSET) * (256.0f / RESOLUTION_WIDTH);
			scaled_y = (emulated_touch_y - MOUSE_Y_OFFSET) * (192.0f / RESOLUTION_HEIGHT);
			set_mouse_coord(scaled_x, scaled_y);
		}
	}
	#endif

	/* There's an event waiting to be processed? */
	while (SDL_PollEvent(&event))
    {
        switch (event.type)
          {
          case SDL_VIDEORESIZE:
            if ( external_videoResizeFn != NULL) {
              external_videoResizeFn( event.resize.w, event.resize.h);
            }
            break;

          case SDL_KEYDOWN:
			switch(event.key.keysym.sym)
			{
				case SDLK_LCTRL:
					#if (defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
					if (mouse_mode == 0) 
					#endif
					{
						*keypad |= 1;
					}
				break;
				case SDLK_LALT:
					#ifdef GKD350H
					if (keys[SDLK_ESCAPE])
					{
						fullscreen_option++;
						if (fullscreen_option > 2) fullscreen_option = 0;
						Set_Offset();
					}
					else
					#endif
					#if (defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
					if (keys[SDLK_TAB])
					{
						mouse_mode ^= 1;
						*keypad &= ~1;
					}
					else
					#endif
					*keypad |= 2;
				break;
				case SDLK_ESCAPE:
					#ifdef GKD350H
					if (keys[SDLK_RETURN])
					{
						cause_quit = 1;
					}
					else 
					#endif
					*keypad |= 4;
				break;
				case SDLK_RETURN:
					*keypad |= 8;
				break;
				case SDLK_LEFT:
					#ifdef FUNKEY
					if (mouse_mode) xaxis = -16000;
					else
					#endif
					*keypad |= 32;
				break;
				case SDLK_RIGHT:
					#ifdef FUNKEY
					if (mouse_mode) xaxis = 16000;
					else
					#endif
					*keypad |= 16;
				break;
				case SDLK_UP:
					#ifdef FUNKEY
					if (mouse_mode) yaxis = -16000;
					else
					#endif
					*keypad |= 64;
				break;
				case SDLK_DOWN:
					#ifdef FUNKEY
					if (mouse_mode) yaxis = 16000;
					else
					#endif
					*keypad |= 128;
				break;
				case SDLK_BACKSPACE:
					*keypad |= 256;
				break;
				case SDLK_TAB:
					*keypad |= 512;
				break;
				case SDLK_LSHIFT:
					*keypad |= 1024;
				break;
				case SDLK_SPACE:
					*keypad |= 2048;
				break;
				#if defined(SDL_SWIZZLEBGR) || defined(GKD350H)
				case SDLK_PAGEUP:
					mouse_mode ^= 1;
				break;
				#endif
				default:
				break;
			}
            break;

          case SDL_KEYUP:
			switch(event.key.keysym.sym)
			{
				case SDLK_LCTRL:
				#if (defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
				if (mouse_mode)
				{
					mouse.click = 1;
					mouse.down = 0;
				}
				else
				#endif
				*keypad &= ~1;
				break;
				case SDLK_LALT:
					*keypad &= ~2;
				break;
				case SDLK_ESCAPE:
					*keypad &= ~4;
				break;
				case SDLK_RETURN:
					*keypad &= ~8;
				break;
				case SDLK_LEFT:
					#ifdef FUNKEY
					if (mouse_mode) xaxis = 0;
					else
					#endif
					*keypad &= ~32;
				break;
				case SDLK_RIGHT:
					#ifdef FUNKEY
					if (mouse_mode) xaxis = 0;
					else
					#endif
					*keypad &= ~16;
				break;
				case SDLK_UP:
					#ifdef FUNKEY
					if (mouse_mode) yaxis = 0;
					else
					#endif
					*keypad &= ~64;
				break;
				case SDLK_DOWN:
					#ifdef FUNKEY
					if (mouse_mode) yaxis = 0;
					else
					#endif
					*keypad &= ~128;
				break;
				case SDLK_BACKSPACE:
					*keypad &= ~256;
				break;
				case SDLK_TAB:
					*keypad &= ~512;
				break;
				case SDLK_LSHIFT:
					*keypad &= ~1024;
				break;
				case SDLK_SPACE:
					*keypad &= ~2048;
				break;
				case SDLK_HOME:
					cause_quit = 1;
				break;
				default:
				break;
			}
            break;
			#if defined(SDL_SWIZZLEBGR) || defined(GKD350H)
			case SDL_JOYAXISMOTION:
				switch(event.jaxis.axis)
				{
					case 0:
						xaxis = event.jaxis.value;
					break;
					case 1:
						yaxis = event.jaxis.value;
					break;
					default:
					break;
				}
			break;
			#endif
		  #if !(defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
          case SDL_MOUSEBUTTONDOWN:
            if(event.button.button==1)
              mouse.down = TRUE;
						
          case SDL_MOUSEMOTION:
            if(!mouse.down)
              break;
            else {
				scaled_x =
                screen_to_touch_range_x( event.button.x,
                                         nds_screen_size_ratio);
				scaled_y =
                screen_to_touch_range_y( event.button.y,
                                         nds_screen_size_ratio);

              if( scaled_y >= 192)
                set_mouse_coord( scaled_x, scaled_y - 192);
            }
            break;
			
          case SDL_MOUSEBUTTONUP:
            if(mouse.down) mouse.click = TRUE;
            mouse.down = FALSE;
            break;
		  #endif
          case SDL_QUIT:
            cause_quit = 1;
            break;

          default:
            break;
          }
	}

	#if (defined(SDL_SWIZZLEBGR) || defined(GKD350H) || defined(FUNKEY))
	if (xaxis < -DEADZONE_JOYSTICK) emulated_touch_x = emulated_touch_x - 2;
	else if (xaxis > DEADZONE_JOYSTICK)  emulated_touch_x = emulated_touch_x + 2;

	if (yaxis < -DEADZONE_JOYSTICK) emulated_touch_y = emulated_touch_y - 2;
	else if (yaxis > DEADZONE_JOYSTICK) emulated_touch_y = emulated_touch_y + 2;
	
	if (emulated_touch_y < MOUSE_Y_OFFSET+1) emulated_touch_y = MOUSE_Y_OFFSET+1;
	if (emulated_touch_y > sdl_screen->h) emulated_touch_y = sdl_screen->h;
	if (emulated_touch_x < MOUSE_X_OFFSET+1) emulated_touch_x = MOUSE_X_OFFSET+1;
	if (emulated_touch_x > (RESOLUTION_WIDTH+MOUSE_X_OFFSET)-10) emulated_touch_x = (RESOLUTION_WIDTH+MOUSE_X_OFFSET)-10;
	#endif

	return cause_quit;
}


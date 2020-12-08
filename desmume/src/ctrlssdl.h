/* joysdl.h - this file is part of DeSmuME
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

#ifndef CTRLSSDL_H
#define CTRLSSDL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <pthread.h>
#include <SDL/SDL.h>
#include "MMU.h"

#include "types.h"

#define KEYMASK_(k)	(1 << (k))
#define JOY_AXIS_(k)    (((k)+1) << 8)

#define NB_KEYS		14
#define KEY_NONE		0
#define KEY_A			1
#define KEY_B			2
#define KEY_SELECT		3
#define KEY_START		4
#define KEY_RIGHT		5
#define KEY_LEFT		6
#define KEY_UP			7
#define KEY_DOWN		8
#define KEY_R			9
#define KEY_L			10
#define KEY_X			11
#define KEY_Y			12
#define KEY_DEBUG		13
#define KEY_BOOST		14

#ifdef GKD350H
extern uint_fast8_t MOUSE_X_OFFSET;
extern uint_fast8_t MOUSE_Y_OFFSET;
extern float RESOLUTION_WIDTH;
extern float RESOLUTION_HEIGHT;
#endif

#if defined(SDL_SWIZZLEBGR) || defined(GKD350H)
extern int_fast16_t emulated_touch_x;
extern int_fast16_t emulated_touch_y;
extern uint_fast8_t mouse_mode;
#endif

#ifndef GTK_UI
struct mouse_status
{
  signed long x;
  signed long y;
  BOOL click;
  BOOL down;
};

extern struct mouse_status mouse;

void set_mouse_coord(int_fast16_t x, int_fast16_t y);
#endif // !GTK_UI

BOOL init_joy( void);
void uninit_joy( void);
void update_keypad(u16 keys);
u16 get_keypad( void);
int
process_ctrls_events( u16 *keypad,
                      void (*external_videoResizeFn)( u16 width, u16 height),
                      float nds_screen_size_ratio);

void
process_joystick_events( u16 *keypad);

#endif /* CTRLSSDL_H */

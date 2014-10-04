/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : console.h
 * Description : 
 * Author(s)   : Calle Laakkonen
 *
 * Luola is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Luola is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef L_CONSOLE_H
#define L_CONSOLE_H

#include "SDL.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

/* The screen */
extern SDL_Surface *screen;

/* Colours */
extern Uint32 col_black;
extern Uint32 col_gray, col_grenade;
extern Uint32 col_snow;
extern Uint32 col_clay, col_clay2;      /* The secod clay colour is underwater clay */
extern Uint32 col_default;
extern Uint32 col_yellow;
extern Uint32 col_red;
extern Uint32 col_green;
extern Uint32 col_blue;
extern Uint32 col_white;
extern Uint32 col_transculent;
extern Uint32 col_rope;
extern Uint32 col_plrs[4];
extern Uint32 col_pause_backg;

/* Initialize SDL library */
extern void init_sdl (void);

/* Initialize screen */
extern void init_video (void);

/* Wait for enter */
extern void wait_for_enter(void);

/* Toggle between fullscreen mode */
extern void toggle_fullscreen(void);

/* Graphics */
extern Uint32 map_rgba(Uint8 r,Uint8 g,Uint8 b,Uint8 a);
extern void draw_box (int x, int y, int w, int h, int width, Uint32 color);
extern void fill_box (SDL_Surface *surface,int x ,int y,int w,int h,Uint32 color);
extern void pixelcopy (Uint32 * srcpix, Uint32 * pixels, int w, int h,
                       int srcpitch, int pitch);
extern SDL_Surface *flip_surface (SDL_Surface * original);
extern SDL_Surface *copy_surface (SDL_Surface * original);
extern SDL_Rect cliprect (int x1, int y1, int w1, int h1, int x2, int y2,
                          int w2, int h2);
extern char clip_line (int *x1, int *y1, int *x2, int *y2, int left, int top,
                       int right, int bottom);
extern void recolor (SDL_Surface * surface, float red, float green,
                     float blue, float alpha);

extern Uint32 getpixel (SDL_Surface * surface, int x, int y);

extern SDL_Surface *zoom_surface (SDL_Surface *original, float aspect,float zoom);

#ifndef HAVE_LIBSDL_GFX
extern inline void putpixel (SDL_Surface * surface, int x, int y,
                             Uint32 color);
extern void draw_line (SDL_Surface * screen, int x1, int y1, int x2, int y2,
                       Uint32 pixel);
#else
#include <SDL_gfxPrimitives.h>
#define putpixel pixelColor
#define draw_line aalineColor
#endif

/* Translate joystick events to keyboard events */
extern void joystick_button (SDL_JoyButtonEvent * btn);
extern void joystick_motion (SDL_JoyAxisEvent * axis, char plrmode);

#endif

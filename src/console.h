/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "SDL.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

/* Player input device */
struct Controller {
    SDLKey keys[6];
    SDL_Joystick *device;
    int number;             /* 0=keyboard, rest are joystick number+1 */
};

/* The screen */
extern SDL_Surface *screen;

/* Colours */
extern Uint32 col_black;
extern Uint32 col_gray, col_grenade;
extern Uint32 col_snow;
extern Uint32 col_clay, col_clay_uw;
extern Uint32 col_default;
extern Uint32 col_yellow;
extern Uint32 col_red;
extern Uint32 col_green;
extern Uint32 col_blue;
extern Uint32 col_cyan;
extern Uint32 col_white;
extern Uint32 col_translucent;
extern Uint32 col_rope;
extern Uint32 col_plrs[4];
extern Uint32 col_pause_backg;

/* Initialize video and audio */
extern void init_sdl (void);

/* Initialize screen */
extern void init_video (void);

/* Wait for enter or joypad button */
extern void wait_for_enter(void);

/* Open player joypads */
extern void open_joypads();

/* Close player joypads */
extern void close_joypads();

/* Toggle between fullscreen and windowed mode */
extern void toggle_fullscreen(void);

/* Map color to RGBA value in current pixel format. Generates a */
/* SDL_gfx compatible value if SDL_gfx is used. */
extern Uint32 map_rgba(Uint8 r,Uint8 g,Uint8 b,Uint8 a);

/* Decompose color to r,g,b and a values. */
/* SDL_gfx format is used if enabled */
extern void unmap_rgba(Uint32 c, Uint8 *r,Uint8 *g,Uint8 *b,Uint8 *a);

/* Draw a box outline */
extern void draw_box (int x, int y, int w, int h, int width, Uint32 color);

/* Draw a filled box. If SDL_gfx is available, supports transparency */
extern void fill_box (SDL_Surface *surface,int x ,int y,int w,int h,Uint32 color);

/* Copy raw pixels from one surface to another */
extern void pixelcopy (Uint32 * srcpix, Uint32 * pixels, int w, int h,
                       int srcpitch, int pitch);

/* Make a deep copy of a surface */
extern SDL_Surface *copy_surface (SDL_Surface * original);

/* Make a new surface that is like the one given. */
/* If w or h is 0, the new surface will be the same size as likethis */
extern SDL_Surface *make_surface(SDL_Surface * likethis,int w,int h);

/* Return the intersection of two rectangles */
extern SDL_Rect cliprect (int x1, int y1, int w1, int h1, int x2, int y2,
                          int w2, int h2);

/* Return a line segment inside a rectangle */
extern int clip_line (int *x1, int *y1, int *x2, int *y2, int left, int top,
                       int right, int bottom);

/* Multiply surface color values */
extern void recolor (SDL_Surface * surface, float red, float green,
                     float blue, float alpha);

/* Get a single pixel from a surface */
extern Uint32 getpixel (SDL_Surface * surface, int x, int y);

/* Resize a surface. A new surface is returned */
extern SDL_Surface *zoom_surface (SDL_Surface *original, float aspect,float zoom);

/* Display an error screen */
extern void error_screen(const char *title, const char *exitsmg,
        const char *message[]);

#ifndef HAVE_LIBSDL_GFX
/* Put a single pixel on screen */
extern inline void putpixel (SDL_Surface * surface, int x, int y,
                             Uint32 color);

/* Draw a line on screen */
extern void draw_line (SDL_Surface * screen, int x1, int y1, int x2, int y2,
                       Uint32 pixel);
#else
#include <SDL_gfxPrimitives.h>
#define putpixel pixelColor
#define draw_line aalineColor
#endif

/* Translate joystick events to keyboard events */
extern void joystick_button (SDL_JoyButtonEvent * btn);
extern void joystick_motion (SDL_JoyAxisEvent * axis, int plrmode);

#endif

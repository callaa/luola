/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : font.h
 * Description : Font library abstraction
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

#ifndef FONT_H
#define FONT_H

#include "SDL_video.h"

/* Different fonts */
typedef enum { Bigfont, Smallfont } FontSize;

/* Some predefined colors for fonts */
extern SDL_Color font_color_white;
extern SDL_Color font_color_gray;
extern SDL_Color font_color_red;
extern SDL_Color font_color_green;
extern SDL_Color font_color_blue;
extern SDL_Color font_color_cyan;

/* Initialize the font library */
extern int init_font (void);

/* Write a string directly to the surface.		*/
/* This is slow if you use SDL_ttf because 		*/
/* an extra surface needs to be created and freed 	*/
extern void putstring_direct (SDL_Surface * surface, FontSize size, int x,
                              int y, const char *text, SDL_Color color);

/* A convenience wrapper for putstring_direct		*/
/* Centers the string horizontaly			*/
extern void centered_string (SDL_Surface * surface, FontSize size, int y,
                             const char *text, SDL_Color color);

/* Return a surface containing the text			*/
/* This is slow with SFont when the you use a color	*/
/* other than white. The returned surface will be	*/
/* in the same format as the display.			*/
extern SDL_Surface *renderstring (FontSize size, const char *text, SDL_Color color);

/* Get the height of the font				*/
extern int font_height (FontSize size);

/* A good spacing for menus (assuming that menus use the Bigfont fontsize) */
extern int MENU_SPACING;

#endif

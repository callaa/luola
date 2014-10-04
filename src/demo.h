/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : demo.h
 * Description : Eyecandy
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

#ifndef DEMO_H
#define DEMO_H

/* Initializes the demos */
extern void init_demos (void);

/* Draw an animated starfield animation on screen. */
extern void draw_starfield (void);

/* Fade the screen to black (Note. program is locked up while fading) */
extern void fade_to_black (void);

/* Fade from black to surface (Note. program is locked up while fading) */
void fade_from_black (SDL_Surface * surface);

#endif

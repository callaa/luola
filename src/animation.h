/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : animation.h
 * Description : This module handles all the animation and redraw timings
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

#ifndef ANIMATION_HEADER
#define ANIMATION_HEADER

/* Functions */
extern void reinit_animation (void);
extern void recalc_geometry(void);
extern SDL_Rect get_viewport_size(void);
extern void rearrange_animation (void);
extern int pause_game (void);
extern void kill_plr_screen (int plr);

extern void animate_frame (void);

/* Globals */
extern int endgame;

#endif

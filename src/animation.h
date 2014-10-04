/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
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

#ifndef ANIMATION_H
#define ANIMATION_H

/* Reinitialize animation for a new level */
extern void reinit_animation (void);

/* Rearrange screen update rectangles */
extern void rearrange_animation (void);

/* Recalculate player viewport geometry */
extern void recalc_geometry(void);

/* Get the size of a player viewport */
extern SDL_Rect get_viewport_size(void);

/* Pause/unpause the game */
extern int pause_game (void);

/* Start fading out a player viewport */
extern void kill_plr_screen (int plr);

/* Animate a single frame */
extern void animate_frame (void);

/* When >0, counts down to 0. When hits 0, the level ends */
extern int endgame;

#endif

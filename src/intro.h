/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : intro.h
 * Description : Intro and configuration screens
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

#ifndef INTRO_H
#define INTRO_H

#include "menu.h"

/* Menu IDs */
#define MENU_ID_INPUT			0x02
#define MENU_ID_HOTSEAT			0x10

/* Return values */
#define INTRO_RVAL_STARTGAME	0x01
#define INTRO_RVAL_EXIT		0x02

/* Initialize intro menus and background animations */
extern int init_intro (void);

/* Intro menus. */
extern int game_menu_screen (void);

/* This is called called by menu callbacks. */
/* It is defined here because it is used in other modules as well */
extern int draw_input_icon (int x, int y, Uint8 align, int mid,
                            MenuItem * item);
/* This is called called by menu callbacks. */
/* It is defined here because an other module needs it */
extern int draw_team_icon (int x, int y, Uint8 align, int mid,
                           MenuItem * item);

/* Globals */
SDL_Surface *keyb_icon[4], *pad_icon[4];


#endif

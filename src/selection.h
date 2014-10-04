/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2005-2006 Calle Laakkonen
 *
 * File        : selection.h
 * Description : Level/weapon selection screens
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

#ifndef SELECTION_H
#define SELECTION_H

#include "levelfile.h"

/* Load trophy images. */
extern int init_selection(LDAT *trophies);

/* Enter level selection screen */
/* The level list from game_settings is used. */
/* Returns 0 if user wishes to cancel level selection. */
extern struct LevelFile *select_level(int fade);

/* Weapon selection. Returns 1 when players have made their choices */
/* or 0 if selection was cancelled. */
extern int select_weapon(struct LevelFile *level);

#endif


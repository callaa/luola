/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : levelfile.h
 * Description : Level loading
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

#ifndef L_LEVELFILE_H
#define L_LEVELFILE_H

#include "lconf.h"

typedef enum { LEV_UNKNOWN, LEV_NORMAL, LEV_COMPACT} LevelFormat;

struct LevelFile {
    char *filename;             /* Level filename */

    LevelFormat type;           /* Level type (normal or compact) */
    int user;                   /* Is the level in user directory */

    LevelSettings *settings;    /* Level settings */
};

/* Scan the directory pointed by 'dirname' for levels */
extern int scan_levels (int user);

/* Show the "No levels found" error screen and exit */
extern void no_levels_found (void);

/* Load the artwork and collisionmap */
/* They are automatically scaled according to */
/* their scalex and scaley values */

/* Loads the level artwork file from a file */
/* The surface returned is in the same format as the screen */
extern SDL_Surface *load_level_art (struct LevelFile *level);

/* Loads the level collisionmap from a file */
/* The surface is returned as it is, that is 8bit */
/* If the image is not 8 bit, NULL is returned */
extern SDL_Surface *load_level_coll (struct LevelFile *level);

#endif


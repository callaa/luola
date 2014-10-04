/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : fs.h
 * Description : File system calls and filepath abstraction
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include "SDL.h"

#include "ldat.h"

/* TODO number of directories should be reduced to DATA_DIRECTORY,
 * USERLEVEL_DIRECTORY and HOME_DIRECTORY */
/* Directories */
typedef enum {DATA_DIRECTORY,FONT_DIRECTORY,LEVEL_DIRECTORY,
    USERLEVEL_DIRECTORY,HOME_DIRECTORY} DataDir;

/* Image transparency types */
/* T_OPAQUE   - no transparency but image is converted to screen format */
/* T_ALPHA    - use image alpha channel */
/* T_COLORKEY - use topleft pixel as colorkey */
/* T_NONE     - no transparency, image is left as it is */
typedef enum {T_OPAQUE, T_ALPHA, T_COLORKEY,T_NONE } Transparency;

/* Get the full path */
extern const char *getfullpath (DataDir dir, const char *filename);

/* Return file2 with file1's path */
extern const char *samepath(const char *file1, const char *file2);

/* Check if home directory exists and create if it doesn't */
extern void check_homedir (void);

/* Load an image */
extern SDL_Surface *load_image (const char *filename, int allownull,
                                Transparency transparency);

/* This function loads the image from SDL_RWops */
extern SDL_Surface *load_image_rw (SDL_RWops * rw, int allownull,
                                   Transparency transparency);

/* Load an array of images with sequential index numbers from an LDAT file. */
/* count is set to the number of images read. */
extern SDL_Surface **load_image_array (LDAT * datafile, int allownull,
                                       Transparency transparency,
                                       const char *id, int *count);

/* This is a convenience function to load an image from a datafile */
extern SDL_Surface *load_image_ldat (LDAT * datafile, int allownull,
                                     Transparency transparency,
                                     const char *id, int index);

/* Take a screenshot and save it in the home folder */
extern void screenshot (void);

#endif

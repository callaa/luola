/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
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

/* Directories */
typedef enum {GFX_DIRECTORY,FONT_DIRECTORY,LEVEL_DIRECTORY,
    USERLEVEL_DIRECTORY,HOME_DIRECTORY,SND_DIRECTORY} DataDir;

/* Get the full path */
extern const char *getfullpath (DataDir dir, const char *filename);

/* Check if home directory exists and create if it doesn't */
extern void check_homedir (void);

/* Read datafiles */
/* if enableAlpha is 1, the alpha channel will be loaded properly. */
/* if it is 2, colorkey will be set to the topleft pixel. */
extern SDL_Surface *load_image (char dir,const char *filename, char allownull,
                                signed char enableAlpha);

/* This function loads the image from SDL_RWops */
extern SDL_Surface *load_image_rw (SDL_RWops * rw, char allownull,
                                   signed char enableAlpha);

/* This function loads an image supported by luola directly */
extern SDL_Surface *load_luola_image_rw (SDL_RWops * rw);

/* This function loads an list of images from a Luola Datafile */
extern SDL_Surface **load_image_array (LDAT * datafile, char allownull,
                                       char enableAlpha, char *id, int first,
                                       int last);

/* This is a convenience function to load an image from a datafile */
extern SDL_Surface *load_image_ldat (LDAT * datafile, char allownull,
                                     char enableAlpha, char *id, int index);

/* Take a screenshot */
extern void screenshot (void);

#endif

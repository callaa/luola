/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : lconf.h
 * Description : Level configuration file parsing
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

#ifndef L_LCONF_H
#define L_LCONF_H

#include <SDL.h>

/* Binary luola configuration file version. */
/* Other versions are not supported */
#define LCONF_REVISION 2

typedef struct _LevelBgMusic {
    char *file;
    struct _LevelBgMusic *next;
} LevelBgMusic;

/* Main block */
typedef struct {
    char *artwork;
    char *collmap;
    char *name;
    float aspect;
    float zoom;
    LevelBgMusic *music;
} LSB_Main;

/* Override block */
typedef struct {
    int indstr_base;
    int critters;
    int stars;
    int snowfall;
    int turrets;
    int jumpgates;
    int cows;
    int fish;
    int birds;
    int bats;
    int soldiers;
    int helicopters;
} LSB_Override;

/* Objects block */
typedef struct LSB_Objects {
    Uint8 type;
    int x, y;
    Uint8 ceiling_attach;
    Uint8 value;
    Uint32 id, link;
    struct LSB_Objects *next;
} LSB_Objects;

/* Palette block */
typedef struct {
    Uint8 entries[256];
} LSB_Palette;

/* Level settings structure */
typedef struct {
    LSB_Main *mainblock;
    LSB_Override *override;
    LSB_Objects *objects;
    LSB_Palette *palette;
    SDL_Surface *icon;
} LevelSettings;

/* Load a text mode configuration file */
LevelSettings *load_level_config (const char *filename);

/* Load a binary configuration file */
LevelSettings *load_level_config_rw (SDL_RWops * rw, int len);

#endif

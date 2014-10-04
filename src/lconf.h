/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
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

#ifndef LCONF_H
#define LCONF_H

#include "SDL.h"
#include "list.h"

/* Main block */
struct LSB_Main {
    char *artwork;
    char *collmap;
    char *name;
    char *thumbnail;
    float aspect;
    float zoom;
    struct dllist *music;
};

/* Override block */
struct LSB_Override {
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
};

/* Object types */
typedef enum {OBJ_TURRET,OBJ_JUMPGATE,OBJ_COW,OBJ_FISH,OBJ_BIRD,OBJ_BAT,
    OBJ_SOLDIER,OBJ_HELICOPTER,OBJ_SHIP} ObjectType;
#define FIRST_SPECIAL OBJ_TURRET
#define LAST_SPECIAL OBJ_JUMPGATE
#define FIRST_CRITTER OBJ_COW
#define LAST_CRITTER OBJ_HELICOPTER
#define OBJECT_TYPES 9

/* Object block */
struct LSB_Object {
    ObjectType type;
    int x, y;
    int value;
    unsigned int id, link;
};

/* Palette block */
struct LSB_Palette {
    Uint8 entries[256];
};

/* Level settings structure */
struct LevelSettings {
    struct LSB_Main mainblock;      /* All levels have a mainblock */
    struct LSB_Palette palette;     /* All levels define a palette */
    struct LSB_Override *override;  /* Some levels may override settings */
    struct dllist *objects;         /* Some levels may specify objects */
    /* Thumbnail is loaded afterwards from the file named in mainblock */
    SDL_Surface *thumbnail;
};

/* Load level configuration from a file */
struct LevelSettings *load_level_config (const char *filename);

/* Load level configuration from an SDL_RWops */
/* The filename is used just for error messages */
struct LevelSettings *load_level_config_rw (SDL_RWops * rw, size_t len, const char *filename);

/* Object type string */
extern const char *obj2str(ObjectType obj);

#endif

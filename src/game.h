/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : game.h
 * Description : Game configuration and initialization
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

#ifndef GAME_H
#define GAME_H

#include "SDL.h"

#include "console.h"
#include "ldat.h"
#include "lconf.h"

#define PLAYMODE_COUNT 5

typedef enum { Normal, OutsideShip, OutsideShip1, RndCritical, RndWeapon } Playmode;

struct dllist;

typedef struct {
    int indstr_base;
    int jumpgates;
    int turrets;
    int critters;
    int cows, birds, fish, bats;
    int snowfall, stars;
} PerLevelSettings;

typedef enum {JLIFE_SHORT,JLIFE_MEDIUM,JLIFE_LONG} Jumplife;

typedef struct {
    int rounds;
    Playmode playmode;
    /* Controller options */
    struct Controller controller[4];
    /* Levels */
    struct dllist *levels;
    /* General game settings */
    int ship_collisions;
    int coll_damage;
    Jumplife jumplife;
    int onewayjp;
    int enable_smoke;
    int endmode;
    int eject;
    int recall;
    int bigscreens;
    /* Weapon settings */
    int large_bullets;
    int weapon_switch;
    int explosions;
    int criticals;
    int soldiers, helicopters;
    /* Level settings */
    PerLevelSettings ls; /* Use level_settings instead in game code */
    int base_regen;
    /* Audio settings */
    int sounds;
    int music;
    enum {PLS_ORDERED,PLS_SHUFFLED} playlist;
    int sound_vol;             /* 0-128 */
    int music_vol;             /* 0-128 */
    /* Temporary settings */
    int mbg_anim;
} GameInfo;

typedef struct {
    int wins[4];
    int lastwin;
    unsigned long lifetime[4];
    int total_rounds;
} GameStatus;

/* Load datafiles */
extern void init_game (LDAT *miscfile);

/* Reset game settings */
extern void reset_game (void);

/* Set level settings using game_settings.ls and the provided settings
 * as sources */
extern void apply_per_level_settings (struct LevelSettings * settings);

/* Draw the filler image on all screen quadrants */
extern void fill_player_screens (void);

/* Game over statistics screen */
extern void game_statistics (void);

/* Ingame eventloop */
extern void game_eventloop (void);

/* Save game settings */
extern void save_game_config (void);

/* Some globals */
extern GameInfo game_settings;
extern PerLevelSettings level_settings;
extern GameStatus game_status;
extern int game_loop;

#endif

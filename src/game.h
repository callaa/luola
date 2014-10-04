/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
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

#ifndef L_GAME_H
#define L_GAME_H

#include "SDL.h"

#include "lconf.h"

#define PLAYMODE_COUNT 5

typedef enum { Keyboard, Pad1, Pad2, Pad3, Pad4 } ControllerType;
typedef enum { Forward, Backward, Left, Right, Weap1, Weap2 } Keys;
typedef enum { Normal, OutsideShip, OutsideShip1, RndCritical,RndWeapon } Playmode;

extern unsigned int Controllers;

struct dllist;

typedef struct {
    int indstr_base;
    int jumpgates;
    int turrets;
    int critters;
    int cows, birds, fish, bats;
    int snowfall, stars;
    int soldiers, helicopters;        /* These are the maximium number of hostile critters each player can have */
} PerLevelSettings;

typedef struct {
    int rounds;
    char players_in[4];
    Playmode playmode;
    /* Controller options */
    ControllerType controller[4];
    SDLKey buttons[4][6];       /* Forward, Backward, Left,Right,Weap1,Weap2 */

    struct dllist *levels;
    struct dllist *first_level;
    struct dllist *last_level;
    int levelcount;
    /* General game settings */
    int ship_collisions;
    int coll_damage;
    int jumplife;
    int enable_smoke;
    int endmode;
    int eject;
    int recall;
    int bigscreens;
    /* Weapon settings */
    int gravity_bullets;
    int wind_bullets;
    int large_bullets;
    int weapon_switch;
    int explosions;
    int holesize;
    int criticals;
    /* Level settings */
    PerLevelSettings ls;
    int base_regen;
    /* Audio settings */
    int sounds;
    int music;
    int playlist;              /* Ordered, random */
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

/* Initialization */
extern int init_game (void);
extern void reset_game (void);
extern void prematch_game (void);
extern void apply_per_level_settings (LevelSettings * settings);

/* Drawing */
extern void fill_unused_player_screens (void);

/* Game over statistics screen */
extern void game_statistics (void);

/* Ingame eventloop */
extern void game_eventloop (void);

/* Some globals */
extern GameInfo game_settings;
extern PerLevelSettings level_settings;
extern GameStatus game_status;
extern Uint8 game_loop;

/* Saving/loading configuration and level stuff */
extern void save_game_config (void);
extern void load_game_config (void);

#endif

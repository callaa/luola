/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : ship.h
 * Description : Ship information and animation
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

#ifndef SHIP_H
#define SHIP_H

#include "SDL.h"
#include "physics.h"
#include "weapon.h"
#include "list.h"

/* Critical hits */
#define CRITICAL_COUNT          8
#define CRITICAL_ENGINE         0x01
#define CRITICAL_FUELCONTROL    0x02
#define CRITICAL_LTHRUSTER      0x04
#define CRITICAL_RTHRUSTER      0x08
#define CRITICAL_CARGO          0x10
#define CRITICAL_STDWEAPON      0x20
#define CRITICAL_SPECIAL        0x40
#define CRITICAL_POWER          0x80

typedef enum { Grey, Red, Blue, Green, Yellow, White, Frozen } PlayerColor;

struct Ship {
    struct Physics physics; /* Must be the first element for inheritance */
    PlayerColor color;  /* Player color */
    SDL_Surface **ship; /* Ship graphic */
    SDL_Surface *shield;/* Shield graphic */
    int anim;           /* Counter used in some animations */
    float angle;        /* Where the ship is facing */
    int thrust;         /* When nonzero, engine is on */
    float turn;         /* How fast the ship is turning */
    float health;       /* Ship hull integrity 0.0 - 1.0 */
    float energy;       /* Weapon energy 0.0 - 1.0 */
    int repeat_audio;   /* Sample repeat counter used by some special weapons */
    int carrying;       /* Is the ship carrying a critter */
    /* Mostly weapons related stuff */
    unsigned char criticals;    /* A bitmap of critical hits */
    int no_power;               /* Powerloss counter */
    int cooloff, special_cooloff, eject_cooloff;    /* Cooloff counters */
    int fire_weapon;            /* When nonzero, primary weapon is fired */
    int fire_special_weapon;    /* As above for special weapon */
    enum {INTACT,BROKEN,DESTROYED} state;
    int visible;        /* Is the ship visible */
    int white_ship;     /* Damage indicator counter */
    struct GravityAnomaly *shieldup; /* When not NULL, shield is activated */
    int afterburn;      /* When nonzero, afterburner is on */
    int frozen;         /* When nonzero, ship is frozen */
    enum {NODART,DARTING,SPEARED} darting;  /* Dart or spear */
    int repairing;      /* When nonzero, autorepair system is active */
    int antigrav;       /* When nonzero, gravity is being repealed */
    struct Ship *remote_control;    /* Ship that is being remote controlled */
    float sweep_angle;  /* Sweeping shot angle */
    int sweeping_down;  /* Sweeping shot turning direction */
    int standard;       /* Primary weapon */
    int special;        /* Special weapon */
};

/* Load ship graphics */
extern void init_ships (LDAT *playerfile);

/* Delete all ships */
extern void clear_ships (void);

/* Create extra ships */
struct LevelSettings;
extern void reinit_ships (struct LevelSettings * settings);

/* Create a new ship */
extern struct Ship *create_ship (PlayerColor color, int weapon, int special);

/* Animation */
extern void animate_ships (void);
extern void draw_ships (void);

/* Find the nearest enemy ship with a player in it that is visible */
extern int find_nearest_enemy (double myX, double myY, int myplr, double *dist);

/* Find the nearest ship */
struct Ship *find_nearest_ship (double myX, double myY, struct Ship * not_this, double *dist);

/* Fire ship special weapon */
extern void ship_fire_special (struct Ship *ship);

/* Stop ship special weapon */
extern void ship_stop_special(struct Ship *ship);

/* Damage a ship */
/* damage is the amount of damage done, ranging from 0.0 to 1.0 */
/* critical is the probability of the damage causing a critical hit */
extern void damage_ship(struct Ship *ship, float damage, float critical);

/* Player claims a ship. Sets color and weapons */
extern void claim_ship (struct Ship * ship, int plr);

/* Cause or repair a critical hit */
extern void ship_critical (struct Ship * ship, int repair);

/* Critical hit names */
extern const char *critical2str (int critical);

/* Bump a ship up by one pixel */
extern void bump_ship(int x,int y);

/* Make ship frozen */
extern void freeze_ship(struct Ship *ship);

/* Impale a ship */
extern void spear_ship(struct Ship *ship,struct Projectile *spear);

/* Activate/deactive cloaking device */
extern void cloaking_device (struct Ship * ship, int activate);

/* Activate/deactive ghost ship */
extern void ghostify (struct Ship * ship, int activate);

/* Activate/deactive remote control  */
extern void remote_control(struct Ship *ship, int activate);

/* Globals */
extern struct dllist *ship_list;

#endif

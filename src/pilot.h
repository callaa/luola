/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : pilot.h
 * Description : Pilot code
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

#ifndef PILOT_H
#define PILOT_H

#include "SDL.h"

#include "walker.h"
#include "game.h"
#include "spring.h"

struct Ship;

#define PARACHUTE_FRAME 2

struct Pilot {
    struct Walker walker;       /* inherits Walker */
    SDL_Surface *sprite[3];     /* Normal,Normal2, Parachute */
    Vector attack_vector;       /* Direction where gun is pointed to */
    Uint32 crosshair_color;     /* Crosshair color */
    int lock;                   /* Reserve controls for aiming */
    int parachuting;            /* When nonzero, parachute is deployed */
    int weap_cooloff;           /* Weapon cooloff counter */
    int toofast;                /* How long has the pilot been falling too fast */
    struct Spring *rope;        /* The ninjarope */
    int ropectrl;               /* -1 retracts rope, 1 extends it */
} Pilot;

/* Rope length limits. Actual rope length is nodelen*nodecount */
static const double pilot_rope_minlen = 0.1;
static const double pilot_rope_maxlen = 10.0;

/* List of active pilots */
extern struct dllist *pilot_list;

/* Load datafiles */
extern void init_pilots (LDAT *playerfile);

/* Initialize pilots */
extern void reinit_pilots (void);

/* Eject a pilot (add it to the level) */
void eject_pilot(int plr);

/* Remove a pilot from the level */
void remove_pilot(struct Pilot *pilot);

/* Kill a pilot */
void kill_pilot(struct Pilot *pilot);

/* Animate all ejected pilots */
extern void animate_pilots (void);

/* Draw all ejected pilots */
extern void draw_pilots (void);

/* Handling */
extern void control_pilot (int plr);

/* Find the nearest pilot */
extern int find_nearest_pilot(float x,float y,int myplr, double *dist);

/* Detach rope */
extern void pilot_detach_rope(struct Pilot *pilot);

#endif

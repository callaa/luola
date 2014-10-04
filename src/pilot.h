/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
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

#ifndef L_PILOT_H
#define L_PILOT_H

#include "SDL.h"

#include "game.h"

struct Ship;

typedef struct {
    SDL_Surface *sprite[3];     /* Normal,Normal2, Parachute */
    Vector vector;
    Vector attack_vector;
    Uint32 crosshair_color;
    enum {NOROPE,ROPE_EXTENDING,USEROPE} rope;
    int rope_len;
    int rope_x, rope_y;
    signed char rope_dir;
    struct Ship *rope_ship;
    double rope_th;             /* Angle of rope */
    double rope_v;              /* Angular velocity of rope */
    double x,y;
    Sint8 hx, hy;               /* Hotspots */
    float maxspeed;
    char jumping;
    signed char walking;
    signed char updown;
    unsigned char lock;
    unsigned char parachuting;
    unsigned char weap_cooloff;
    unsigned char lock_btn2;
    char onbase;
    int toofast;              /* How long has the pilot been falling too fast */
} Pilot;

/* Globals */
extern char pilot_any_ejected;

/* Initialization */
extern int init_pilots (void);
extern void init_pilot (Pilot *pilot,int playernum);

/* Animation */
extern void animate_pilots (void);
extern void draw_pilots (void);
/* Handling */
extern int hit_pilot (int x, int y,int team);
extern void control_pilot (int plr);
extern int find_nearest_pilot(int myX,int myY,int not_this, double *dist);
extern void pilot_detach_rope(Pilot *pilot);

#endif

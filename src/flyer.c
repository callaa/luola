/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : flyer.c
 * Description : Physics for things that fly (or swim)
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

#include <stdlib.h>
#include <math.h>

#include "flyer.h"

/* Initialize flyer */
void init_flyer(struct Flyer *flyer, FlyerMode mode) {
    init_physobj(&flyer->physics,0,0,makeVector(0,0));
    flyer->physics.sharpness = BOUNCY;

    flyer->physics.radius = 4;
    if(mode==AIRBORNE)
        flyer->physics.mass = 3.5;
    else
        flyer->physics.mass = 7.5;

    flyer->bat = 0;
    flyer->speed = 2.0;

    flyer->targx = 0;
    flyer->targy = 0;

    flyer->mode = mode;
}

/* Animate flyer */
void animate_flyer(struct Flyer *flyer,int lists, struct dllist *objs[]) {
    Vector v;
    /* Do physics simulation */
    animate_object(&flyer->physics,lists,objs);

    /* Fly towards targx,targy */
    v = makeVector(flyer->targx - flyer->physics.x, flyer->targy - flyer->physics.y);
    double d = hypot(v.x,v.y);
    v = multVector(v,flyer->speed/d);
    if(v.x<0) {
        if(flyer->physics.vel.x>v.x)
            flyer->physics.vel.x += v.x/(flyer->speed*6);
    } else {
        if(flyer->physics.vel.x<v.x)
            flyer->physics.vel.x += v.x/(flyer->speed*6);
    }
    if(v.y<0) {
        if(flyer->physics.vel.y>v.y)
            flyer->physics.vel.y += v.y/(flyer->speed*6);
    } else {
        if(flyer->physics.vel.y<v.y)
            flyer->physics.vel.y += v.y/(flyer->speed*6);
    }
}


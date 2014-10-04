/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : flyer.h
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

#ifndef FLYER_H
#define FLYER_H

#include "physics.h"

typedef enum {AIRBORNE, UNDERWATER} FlyerMode;

struct Flyer {
    struct Physics physics;     /* inherits Physics */
    FlyerMode mode;             /* Swimming things use this code as well */

    int bat;                    /* Perch upside down */

    float speed;               /* How fast can this thing fly (or swim) */
    float targx,targy;         /* Fly towards these coordinates */
};

/* Initialize flyer */
extern void init_flyer(struct Flyer *flyer,FlyerMode mode);

/* Animate flyer */
extern void animate_flyer(struct Flyer *flyer,int lists, struct dllist *objs[]);

#endif


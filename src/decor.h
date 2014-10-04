/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : decor.h
 * Description : Decorative particle effects
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

#ifndef DECOR_H
#define DECOR_H

#include "physics.h"

struct Decor {
    struct Physics physics;     /* inherits Physics */

    Uint32 color;               /* Particle color */
    int jitter;                 /* Jitter movement */
};

/* Prepare decorations (weather) for the next level */
extern void prepare_decorations (void);

/* Create decoration particles */
extern struct Decor *make_snowflake(double x,double y, Vector v);
extern struct Decor *make_blood(double x,double y, Vector v);
extern struct Decor *make_waterdrop(double x,double y, Vector v);
extern struct Decor *make_feather(double x,double y, Vector v);

/* Add a new decoration particle to list */
extern void add_decor(struct Decor *d);

/* Add a cluster of decoration particles */
extern void add_splash(double x,double y, double f,int count, Vector v,
        struct Decor *(*make_decor)(double x, double y, Vector v));

/* Animation */
extern void animate_decorations (void);

/* Globals */
extern double weather_wind_vector;

#endif

/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : particle.h
 * Description : Particle engine
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

#ifndef L_PARTICLE_H
#define L_PARTICLE_H

#include "vector.h"

typedef struct {
    int x, y;
    Vector vector;
    int age;
    int rd, gd, bd, ad;
    unsigned char color[4];
    unsigned char targ_color[4];
} Particle;

/* Initialization */
extern void init_particles (void);
extern void clear_particles (void);

/* Handling */
extern Particle *make_particle (int x, int y, int age);

/* Animation */
extern void animate_particles (void);

extern void calc_color_deltas (Particle * part);

#endif

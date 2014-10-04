/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
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

#ifndef PARTICLE_H
#define PARTICLE_H

#include "physics.h"

struct Particle {
    Vector vector;
    float x, y;
    int age;
    int rd, gd, bd, ad;
    unsigned char color[4];
};

/* Delete all particles */
extern void clear_particles (void);

/* Create a new particle and add it to the list */
extern struct Particle *make_particle (float x, float y, int age);

/* Animate and draw particles */
extern void animate_particles (void);

extern void calc_color_deltas (struct Particle * part,Uint8 r,Uint8 g,Uint8 b,Uint8 a);

#endif

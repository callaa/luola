/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : walker.h
 * Description : Physics for things that walk around
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

#ifndef WALKER_H
#define WALKER_H

#include "physics.h"

struct Walker {
    struct Physics physics;     /* inherits Physics */

    /* Settings */
    int walkspeed;              /* Walking speed in pixels per step */
    int slope;                  /* How high a slope the walker can climb */
    enum {JUMP,GLIDE} jumpmode; /* Jumping mode */
    float jumpstrength;         /* Jumping strength */

    /* Flags */
    int walking;                /* -1 walk left, 1 walk right */
    int jump;                   /* Set to 1 to initiate jump */
    int dive;                   /* Dive when underwater */
};

/* Initialize walker */
extern void init_walker(struct Walker *walker);

/* Animate walker */
extern void animate_walker(struct Walker *walker,int lists,struct dllist *objs[]);

/* Instruct a walker to jump */
extern void walker_jump(struct Walker *walker);

/* Instruct a walker to dive */
extern void walker_dive(struct Walker *walker);

/* Try to find foothold at x, y +/- h */
/* Returns the y coordinate where ground was found, or -1 if not */
extern int find_foothold(int x,int y,int h);

#endif


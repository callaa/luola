/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : spring.h
 * Description : Spring physics
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

#ifndef SPRING_H
#define SPRING_H

#include "physics.h"

struct Spring {
    struct Physics *head;   /* Starting point for the spring */
    struct Physics *tail;   /* End point for the spring */
    struct Physics *nodes;  /* Possible nodes in between */
    int nodecount;          /* Number of nodes */

    float sc;               /* Spring constant */
    float nodelen;          /* Max. length between two nodes */
    Uint32 color;           /* Color of the spring */
};

/* Create a new spring. A physics object is automatically created at the tail */
extern struct Spring *create_spring(struct Physics *head,float nodelen, int nodecount);

/* Destroy the spring */
extern void free_spring(struct Spring *spring);

/* Animate a spring */
extern void animate_spring(struct Spring *spring);

/* Draw a spring onto a viewport */
extern void draw_spring(struct Spring *spring, const SDL_Rect *camera, const SDL_Rect *viewport);

#endif


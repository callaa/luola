/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : particle.c
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

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "SDL.h"

#include "defines.h"
#include "console.h"
#include "level.h"
#include "player.h"
#include "particle.h"
#include "list.h"

/* Internally used globals */
static struct dllist *particles;

/* Internally used function prototypes */
static inline void draw_particle (Particle * part);

/* Initialize */
void init_particles (void) {
    particles = NULL;
}

/* Deinitialize */
void clear_particles (void) {
    dllist_free(particles,free);
    particles=NULL;
}

/* Utility function to create a new particle */
Particle *make_particle (int x, int y, int age)
{
    Particle *newpart;
    newpart = malloc (sizeof (Particle));
    newpart->x = x;
    newpart->y = y;
    newpart->age = age;
    newpart->color[0] = 200;
    newpart->color[1] = 200;
    newpart->color[2] = 255;
    newpart->color[3] = 255;
    newpart->targ_color[0] = 0;
    newpart->targ_color[1] = 0;
    newpart->targ_color[2] = 0;
    newpart->targ_color[3] = 0;
    newpart->vector.x=0;
    newpart->vector.y=0;
    calc_color_deltas (newpart);
    particles = dllist_prepend(particles,newpart);
    return newpart;
}

void animate_particles (void)
{
    struct dllist *list = particles, *next;
    if (list == NULL)
        return;
    while (list) {
        Particle *part = list->data;
        next = list->next;
        part->x -= Round (part->vector.x);
        part->y -= Round (part->vector.y);
        part->color[0] += part->rd;
        part->color[1] += part->gd;
        part->color[2] += part->bd;
        part->color[3] += part->ad;
        part->age--;
        if (part->age <= 0) {   /* Particle has expired, delete it */
                free (part);
                if(list==particles)
                    particles=dllist_remove(list);
                else
                    dllist_remove(list);
        } else
            draw_particle (part);
        list = next;
    }
}

static inline void draw_particle (Particle * part)
{
    Uint32 color;
    int x, y, p;

    color = map_rgba(part->color[0], part->color[1], part->color[2],
            part->color[3]);
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            x = part->x - cam_rects[p].x;
            y = part->y - cam_rects[p].y;
            if ((x > 0 && x < cam_rects[p].w)
                && (y > 0 && y < cam_rects[p].h))
                putpixel (screen, lev_rects[p].x + x, lev_rects[p].y + y,
                          color);
        }
    }
}

/* Calculate color delta values */
void calc_color_deltas (Particle * part)
{
    part->rd = (part->targ_color[0] - part->color[0]) / part->age;
    part->gd = (part->targ_color[1] - part->color[1]) / part->age;
    part->bd = (part->targ_color[2] - part->color[2]) / part->age;
    part->ad = (part->targ_color[3] - part->color[3]) / part->age;
}

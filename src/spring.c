/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : spring.c
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

#include <stdlib.h>
#include <math.h>

#include "defines.h" /* For Round() */
#include "console.h"
#include "spring.h"

/* Create a new spring */
struct Spring *create_spring(struct Physics *head,float nodelen, int nodecount)
{
    struct Spring *spring = malloc(sizeof(struct Spring));
    if(!spring) {
        perror("create_spring");
        return NULL;
    }

    spring->head = head;
    spring->tail = malloc(sizeof(struct Physics));
    init_physobj(spring->tail,head->x,head->y,makeVector(0,0));

    spring->sc = -0.6;
    spring->color = col_rope;

    spring->nodelen = nodelen;
    spring->nodecount = nodecount;
    if(nodecount) {
        int r;
        spring->nodes = malloc(sizeof(struct Physics)*nodecount);
        for(r=0;r<nodecount;r++) {
            init_physobj(&spring->nodes[r],
                spring->head->x,spring->head->y,makeVector(0,0));
            spring->nodes[r].sharpness = BOUNCY;
        }

    } else {
        spring->nodes = NULL;
    }

    return spring;
}

/* Destroy the spring */
void free_spring(struct Spring *spring) {
    /* TODO, when tail is another object, it shouldn't be freed */
    free(spring->tail);
    free(spring->nodes);
    free(spring);
}

/* Animate spring segment */
static void animate_segment(struct Spring *spring,struct Physics *s1, struct Physics *s2) {
    Vector springvec = makeVector(s1->x - s2->x, s1->y - s2->y);
    double dist = hypot(springvec.x,springvec.y);
    if(dist>0) {
        Vector force = multVector(springvec,1/dist);
        Vector friction = {s1->vel.x - s2->vel.x, s1->vel.y - s2->vel.y};

        force = multVector(force,(dist - spring->nodelen)*spring->sc);
        friction = multVector(friction, -0.1); /* Something wrong with this */
        force = addVectors(force,friction);

        s1->vel = addVectors(s1->vel,multVector(force,1/(s1->mass/s2->mass)));
        s2->vel = addVectors(s2->vel,multVector(force,1/(-s2->mass/s1->mass)));
    }
}

/* Simulate the spring */
void animate_spring(struct Spring *spring) {

    if(spring->nodes) {
        int r;
        animate_segment(spring,spring->head,&spring->nodes[0]);
        for(r=0;r<spring->nodecount-1;r++) {
            animate_segment(spring,&spring->nodes[r],&spring->nodes[r+1]);
            animate_object(&spring->nodes[r],0,NULL);
        }
        animate_segment(spring,&spring->nodes[r],spring->tail);
        animate_object(&spring->nodes[r],0,NULL);
    } else {
        animate_segment(spring,spring->head,spring->tail);
    }
    animate_object(spring->tail,0,NULL);
}

/* Draw a spring segment */
static void draw_segment(Uint32 color,const struct Physics *s1,
        const struct Physics *s2, const SDL_Rect *camera, const SDL_Rect *viewport)
{
    int x1 = Round(s1->x)-camera->x;
    int y1 = Round(s1->y)-camera->y;
    int x2 = Round(s2->x)-camera->x;
    int y2 = Round(s2->y)-camera->y;

    if(clip_line(&x1,&y1,&x2,&y2, 0, 0, camera->w,camera->h))
        draw_line(screen,x1+viewport->x,y1+viewport->y,
                x2+viewport->x,y2+viewport->y,color);
}

/* Draw the spring onto screen */
void draw_spring(struct Spring *spring, const SDL_Rect *camera, const SDL_Rect *viewport) {
    if(spring->nodes) {
        int r;
        draw_segment(spring->color,spring->head,&spring->nodes[0],camera,viewport);
        for(r=0;r<spring->nodecount-1;r++) {
            draw_segment(spring->color,&spring->nodes[r],
                    &spring->nodes[r+1],camera,viewport);
        }
        draw_segment(spring->color,&spring->nodes[r],
                spring->tail,camera,viewport);
    } else {
        draw_segment(spring->color,spring->head,spring->tail,camera,viewport);
    }
}


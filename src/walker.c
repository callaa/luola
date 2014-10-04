/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : walker.c
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

#include <stdlib.h>

#include "level.h"
#include "walker.h"

/* Initialize walker with default values */
void init_walker(struct Walker *walker) {
    init_physobj(&walker->physics,0,0,makeVector(0,0));
    walker->physics.radius = 4.1;
    walker->physics.mass = 5.0;
    walker->physics.sharpness = BOUNCY;
    walker->physics.semisolid = 1;

    walker->walkspeed = 3;
    walker->slope = 10;
    walker->jumpmode = JUMP;
    walker->jumpstrength = 3.0;

    walker->walking = 0;
    walker->jump = 0;
    walker->dive = 0;
}

/* Instruct a walker to jump or swim up */
void walker_jump(struct Walker *walker) {
    if(walker->physics.hitground || walker->physics.underwater)
        walker->jump=1;
}

/* Instruct a walker to dive */
void walker_dive(struct Walker *walker) {
    if(walker->physics.underwater)
        walker->dive = 1;
}

/* Try to find foothold at x, y +/- h */
/* Returns the y coordinate where ground was found, or -1 if not */
int find_foothold(int x,int y,int h) {
    int r;
    if(is_walkable(x,y) && is_breathable(x,y-1)) return y;
    for(r=1;r<h;r++) {
        if(is_walkable(x,y+r) && is_breathable(x,y+r-1)) return y+r;
        if(is_walkable(x,y-r) && is_breathable(x,y-r-1)) return y-r;
    }
    return -1;
}

/* Animate walker */
void animate_walker(struct Walker *walker,int lists,struct dllist *objs[]) {

    /* Jump if jump flag was set */
    if(ter_walkable(walker->physics.hitground)) {
        if(walker->jump==1) {
            walker->physics.y--; /* First, get off the ground */
            walker->physics.vel.y -= walker->jumpstrength;
            walker->jump=2; /* Set flag to indicate that jump has begun */
        } else if(walker->jump==2) {
            walker->jump=0;
        }
    }

    /* Do physics simulation */
    animate_object(&walker->physics,lists,objs);

    /* Walk, glide and swim */
    if(walker->walking) {
        if(ter_walkable(walker->physics.hitground)) {
            /* Walk if standing on solid ground */
            int foothold;
            walker->physics.x += walker->walking*walker->walkspeed;
            foothold = find_foothold(walker->physics.x,walker->physics.y,walker->slope);
            if(foothold==-1) {
                if(is_breathable(walker->physics.x,walker->physics.y)==0)
                    walker->physics.x -= walker->walking*walker->walkspeed;
            } else {
                walker->physics.y = foothold;
            }
        } else {
            if(walker->physics.underwater) {
                /* Swim if in water */
                walker->physics.vel.x += walker->walking*0.2;
            } else {
                /* Glide if in air */
                if(walker->jumpmode==JUMP && walker->jump==2) {
                    walker->physics.vel.x += walker->walking*0.4;
                    walker->jump=0;
                } else if(walker->jumpmode==GLIDE) {
                    walker->physics.vel.x += walker->walking*0.4;
                }
            }
        }
    }

    /* Swim up and down */
    if(walker->physics.underwater) {
        /* TODO bubbles */
        if(walker->jump) {
            walker->physics.vel.y -= 0.2;
        } else if(walker->dive) {
            walker->physics.vel.y += 0.2;
        }
    }
}


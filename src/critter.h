/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : critter.h
 * Description : Critters that walk,swim or fly around the level. Most are passive, but some attack (soldiers and helicopters)
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

#ifndef CRITTER_H
#define CRITTER_H

#include "walker.h"
#include "flyer.h"
#include "ldat.h"
#include "lconf.h"

struct Critter {
    union {
        struct Physics physics; /* Everything uses this */
        struct Walker walker;   /* Walking critters use this */
        struct Flyer flyer;     /* Flying and swimming critters use this */
    };
    enum {INERTCRITTER,GROUNDCRITTER,AIRCRITTER,WATERCRITTER} type;

    SDL_Rect gfx_rect;  /* Use only a piece of the gfx surface */
    SDL_Surface **gfx;  /* Graphics */
    int frames;         /* Total number of frames in animation */
    int frame;          /* Current frame of animation */
    int bidir;          /* Animation has separate frames for left & right */
    float health;       /* Critter health. Uses the same scale as ships */
    int owner;          /* Soldiers and helicopters don't attack their owners */
    int ship;           /* Does this critter collide with ships? */
    int frozen;         /* Draw a block of ice over the critter */

    /* Counters */
    int timer;          /* When hits 0, timerfunc is called */
    int ff;             /* Fight or Flight counter */
    int cooloff;        /* Weapon cooloff for armed critters */
    float cornered;     /* How cornered does the critter feel */

    /* Methods */
    void (*animate)(struct Critter *critter);
    void (*timerfunc)(struct Critter *critter);
    void (*die)(struct Critter *critter);
};

/* Load critter datafiles */
extern void init_critters (LDAT *datafile);

/* Initialize critters before a match starts */
extern void prepare_critters (struct LevelSettings *settings);

/* Create a new critter of the specified type */
extern struct Critter *make_critter (ObjectType species, float x, float y,int owner);

/* Add a new critter to the level */
extern void add_critter (struct Critter * newcritter);

/* A projectile hits a critter */
struct Projectile;
extern void hit_critter(struct Critter *critter, struct Projectile *p);

/* Animate and draw all critters */
extern void animate_critters (void);
extern void draw_bat_attack (void);

/* List of critters */
extern struct dllist *critter_list;

#endif

/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : projectile.h
 * Description : Projectile animation and handling
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
#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "SDL.h"

#include "ldat.h"
#include "physics.h"

struct Ship;

struct Projectile { /* inherits Physics */
    struct Physics physics;
    
    /* Settings */
    float damage;           /* Damage the projectile causes when it hits */
    float critical;         /* Probability of causing a critical hit */
    int cloak;              /* Hide from players until nearby */
    Uint32 color;           /* Color of the projectile */

    /* Special settings */
    float angle;
    int owner;              /* Some weapons are team-aware */
    struct Ship *src;       /* Some weapons need to know where they came from */

    /* Counters */
    int life;               /* Lifetime in frames left. -1 means unlimited */
    int timer;              /* When hits 0, timerfunc is called */
    int prime;              /* When hits 0, solid and obj will be set to 1 */
    int var;                /* A variable used by some projectiles */

    /* Flags */
    int hydrophobic;        /* Explode when touch water */
    int otherobj;           /* Collide with other projectiles */
    int critter;            /* Collide with critters and pilots */

    /* Methods */
    void (*draw)(struct Projectile *p,int x,int y, SDL_Rect viewport);
    void (*move)(struct Projectile *p);
    void (*explode)(struct Projectile *p);
    int  (*hitship)(struct Projectile *p,struct Ship *ship);
    void (*timerfunc)(struct Projectile *p);
    void (*destroy)(struct Projectile *p);

};

/* Load projectile related datafiles */
extern void init_projectiles(LDAT *explosionfile);

/* Clear all projectiles */
extern void clear_projectiles(void);

/* Set time until projectile becomes active */
extern void set_fuse(struct Projectile *p, int ticks);

/* Add a new projectile to list */
extern void add_projectile(struct Projectile *p);

/* Add a new explosion to list */
extern void add_explosion(int x,int y);

/* Animate and draw all projectiles */
extern void animate_projectiles(void);

/* List of projectiles. Look, don't touch please */
extern struct dllist *projectile_list,*last_projectile;

#endif


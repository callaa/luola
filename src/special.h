/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : special.h
 * Description : Level special objects
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

#ifndef SPECIAL_H
#define SPECIAL_H

#include "lconf.h"

struct SpecialObj {
    SDL_Surface **gfx;  /* Graphics */
    int x,y;            /* Special objects occupy a fixed position on the map */
    unsigned int frames;/* Number of frames in special object animation */
    unsigned int frame; /* Current frame */
    float health;       /* Object health. Uses the same scale as ships */
    int owner;          /* Player that owns this object. Usually -1 */
    int secret;         /* Only the owner can see this object */
    int life;           /* Age counter. Object is removed when hits 0 */
    int timer;          /* Generic timer */
    struct SpecialObj *link;    /* Jumpgate/wormhole pair */
    float angle;        /* Angle, used by turrets */
    float turn;         /* Turning direction, used by turrets */
    int type;           /* Turret type */

    void (*hitship)(struct SpecialObj*,struct Ship*);
    void (*hitprojectile)(struct SpecialObj*,struct Projectile*);
    void (*animate)(struct SpecialObj*);
    void (*destroy)(struct SpecialObj*);
};

/* Initialization */
extern void init_specials (LDAT *specialfile);
extern void clear_specials (void);
extern void prepare_specials (struct LevelSettings * settings);

/* Append a new special object to the list */
extern void add_special (struct SpecialObj * special);

/* Create a jump-point */
extern struct SpecialObj *make_jumppoint(int x,int y,int owner,int exit);

/* Drop a jumppoint. When two jumppoints belonging to the same player */
/* are dropped, the wormhole forms. First point dropped is the exit point. */
extern void drop_jumppoint (int x, int y, int player);

/* Animation */
extern void animate_specials (void);

#endif

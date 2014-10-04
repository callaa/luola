/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
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

#ifndef L_CRITTER_H
#define L_CRITTER_H

#include "vector.h"
#include "weapon.h"
#include "lconf.h"

typedef enum { Bird, Cow, Fish, Bat, Infantry, Helicopter } CritterSpecies;
typedef enum { FlyingCritter, GroundCritter, WaterCritter } CritterType;
typedef enum { Blood, SomeBlood, Feather, LotsOfFeathers } SplatterType;

struct Ship;

typedef struct {
    int x, y;
    int x2, y2;
    unsigned char frame;
    unsigned char wait;
    SDL_Rect rect;
    Vector vector;
    double angle;
    int timer;
    signed char owner;          /* So attacking critters know who not to shoot */
    unsigned char health;
    struct Ship *carried;
    char explode;
    int angry;
    CritterType type;
    CritterSpecies species;
} Critter;

/* Initialization */
extern int init_critters (void);
extern void clear_critters (void);
extern void prepare_critters (LevelSettings * settings);

/* Handling */
extern Critter *make_critter (CritterSpecies species, int x, int y,
                              signed char owner);
extern void add_critter (Critter * newcritter);
extern void cow_storm (int x, int y);   /* Gravity wells effect critters as well */
extern void splatter (int x, int y, SplatterType type);
extern int find_nearest_terrain (int x, int y, int h);

/* Animation */
extern void animate_critters (void);
extern void draw_bat_attack (void);
extern int hit_critter (int x, int y, ProjectileType proj);

#endif

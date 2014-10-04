/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : level.h
 * Description : Level handling and animation
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

#ifndef LEVEL_H
#define LEVEL_H

#include "SDL.h"

#include "defines.h"
#include "game.h"
#include "physics.h"

/* Free terrain */
#define TER_FREE        0
#define TER_TUNNEL      1

/* Semi solid terrain */
#define TER_WALKWAY     2

/* Solid terrain */
#define TER_GROUND      3
#define TER_INDESTRUCT  4
#define TER_BASE        5
#define TER_BASEMAT     6
#define TER_SNOW        7
#define TER_ICE         8
#define TER_EXPLOSIVE   9
#define TER_EXPLOSIVE2  10
#define TER_COMBUSTABLE 11
#define TER_COMBUSTABL2 12
#define TER_UNDERWATER  13

/* Water */
#define TER_WATER       14
#define TER_WATERFU     15
#define TER_WATERFR     16
#define TER_WATERFD     17
#define TER_WATERFL     18

#define LAST_TER        18

/* Fire animation */
#define FIRE_FRAMES     26
#define FIRE_SPREAD     17
#define FIRE_RANDOM     17

typedef enum { Fire, Ice, Earth, Explosive, Melt } LevelFXType;

struct LevelFile;

typedef struct {
    int x,y;        /* Coordinates for this base pixel */
    Uint32 c;       /* Color of this base pixel */
} RegenCoord;

typedef struct {
    int width;                  /* Width of the level in pixels */
    int height;                 /* Height of the level in pixels */
    SDL_Surface *terrain;       /* Level graphics */
    unsigned char **solid;      /* Collision map */
    int player_def_x[2][4];     /* Beginning x coordinate for players */
    int player_def_y[2][4];     /* Beginning y coordinate for players */
    int base_area;              /* How many pixels of base terrain we have */
    RegenCoord *base;           /* Coordinates for bases */
    int regen_area;             /* How much there is to regenerate */
    int regen_timer;            /* Base regeneration timer */
} Level;

/* Globals */
extern SDL_Rect cam_rects[4];        /* Camera rectangles for players */
extern SDL_Rect viewport_rects[4];   /* Where to draw playre screens. Use only x and y */
extern Uint32 burncolor[FIRE_FRAMES];
extern Uint32 lev_watercolrgb[3];
extern Uint32 lev_watercol;
extern Uint32 lev_basecol;

extern Level lev_level;

/* Initialization and loading */
extern void init_level (void);
extern void reinit_stars (void);
extern void load_level (struct LevelFile *lev);
extern void unload_level (void);

extern int find_rainy (int x);

/* Pixel perfect collision detection */
extern int hit_solid_line (int startx, int starty, int endx, int endy,
                            int *newx, int *newy);

/* Terrain type checks */
static inline int ter_free(int terrain) {
    return terrain==TER_FREE || terrain==TER_TUNNEL;
}

static inline int is_free(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 0;
    return ter_free(lev_level.solid[x][y]);
}

static inline int is_breathable(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 0;
    return lev_level.solid[x][y]<=TER_WALKWAY;
}

static inline int ter_semisolid(int terrain) {
    return terrain==TER_WALKWAY;
}

static inline int ter_solid(int terrain) {
    return terrain>=TER_WALKWAY && terrain<=TER_UNDERWATER;
}

static inline int is_solid(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 1;
    return ter_solid(lev_level.solid[x][y]);
}

static inline int ter_walkable(int terrain) {
    return terrain>=TER_GROUND && terrain<=TER_UNDERWATER;
}

static inline int is_walkable(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 1;
    return ter_walkable(lev_level.solid[x][y]);
}


static inline int is_explosive(int x,int y) {
    return lev_level.solid[x][y] == TER_EXPLOSIVE || lev_level.solid[x][y]==TER_EXPLOSIVE2;
}

static inline int is_burnable(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 0;
    return lev_level.solid[x][y] >= TER_EXPLOSIVE && lev_level.solid[x][y]<=TER_COMBUSTABL2;
}

static inline int ter_indestructable(int terrain) {
    return terrain==TER_INDESTRUCT || (level_settings.indstr_base &&
         (terrain==TER_BASE || terrain==TER_BASEMAT));
}

static inline int is_indestructable(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 1;
    return ter_indestructable(lev_level.solid[x][y]);
}

static inline int is_water(int x,int y) {
    if (x < 0 || x>=lev_level.width || y<0 || y>=lev_level.height)
        return 0;
    return lev_level.solid[x][y]>=TER_WATER &&
        lev_level.solid[x][y]<=TER_WATERFL;
}

/* Level effects */
extern void start_burning (int x, int y);
extern void start_melting (int x, int y, unsigned int recurse);
extern void alter_level (int x, int y, int recurse, LevelFXType type);
extern void make_hole(int x,int y);
extern void burn_hole(int x,int y);
extern void animate_level (void);

#endif

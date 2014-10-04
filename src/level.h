/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
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

#ifndef L_LEVEL_H
#define L_LEVEL_H

#include "SDL.h"

#include "defines.h"
#include "game.h"
#include "vector.h"

/* Luola palette */
#define TER_FREE        0
#define TER_GROUND      1
#define TER_UNDERWATER  2
#define TER_INDESTRUCT  3
#define TER_WATER       4
#define TER_BASE        5
#define TER_EXPLOSIVE   6
#define TER_EXPLOSIVE2  7
#define TER_WATERFU     8
#define TER_WATERFR     9
#define TER_WATERFD     10
#define TER_WATERFL     11
#define TER_COMBUSTABLE 12
#define TER_COMBUSTABL2 13
#define TER_SNOW        14
#define TER_ICE         15
#define TER_BASEMAT     16
#define TER_TUNNEL      17
#define TER_WALKWAY     18
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
extern SDL_Rect cam_rects[4];   /* Camera rectangles for players */
extern SDL_Rect lev_rects[4];   /* Where to draw playre screens (use only x+y) */
extern Uint32 burncolor[FIRE_FRAMES];
extern Uint32 lev_watercolrgb[3];
extern Uint32 lev_watercol;
extern Uint32 lev_basecol;

extern Level lev_level;
extern Vector gravity;

/* Initialization and loading */
extern void init_level (void);
extern void reinit_stars (void);
extern void load_level (struct LevelFile *lev);
extern void unload_level (void);

extern int is_walkable (int x, int y);
extern int is_water (int x, int y);
extern int find_rainy (int x);

/* A pixel perfect collision detection */
extern int hit_solid_line (int startx, int starty, int endx, int endy,
                            int *newx, int *newy);

/* Inlines */
static inline int hit_solid (int x, int y)
{
    if (x < 0)
        x = 0;
    else if (x >= lev_level.width)
        return TER_INDESTRUCT;
    if (y < 0)
        x = 0;
    else if (y >= lev_level.height)
        return TER_INDESTRUCT;
    if (lev_level.solid[x][y] == TER_TUNNEL)
        return 0;
    if (lev_level.solid[x][y] == TER_FREE)
        return 0;
    if (lev_level.solid[x][y] == TER_WATER)
        return -1;
    if (lev_level.solid[x][y] >= TER_WATERFU
        && lev_level.solid[x][y] <= TER_WATERFL)
        return -2 - (lev_level.solid[x][y] - TER_WATERFU);
    return lev_level.solid[x][y];
}

static inline void level_bounds(double *x,double *y) {
    if(Round(*x)<0) *x=0;
    else if(Round(*x)>=lev_level.width) *x=lev_level.width-1;
    if(Round(*y)<0) *y=0;
    else if(Round(*y)>=lev_level.height) *y=lev_level.height-1;
}

/* Level effects */
extern void start_burning (int x, int y);
extern void start_melting (int x, int y, unsigned int recurse);
extern void alter_level (int x, int y, int recurse, LevelFXType type);
extern void animate_level (void);

#endif

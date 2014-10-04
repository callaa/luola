/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : level.c
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "console.h"
#include "player.h"
#include "fs.h"
#include "level.h"
#include "levelfile.h"
#include "particle.h"
#include "weather.h"
#include "animation.h"
#include "ship.h"   /* for bump_ship() */

#define BASE_REGEN_SPEED 9 /* Delay between each regenerated pixel */

/* Level effects */
typedef struct {
    int x, y;
    int brake;
    unsigned char value;
    char icicle;
    LevelFXType type;
} LevelFX;

/* Linked lists for level effects */
struct LevelEffects {
    LevelFX *fx;
    struct LevelEffects *next;
    struct LevelEffects *prev;
} *level_effects;

static struct LevelEffects *lev_lastfx;

/* Stars */
typedef struct {
    int x, y;
} Star;

/* Internally used globals */
static Star lev_stars[15];

/* Exported globals */
Uint32 burncolor[FIRE_FRAMES];
Uint32 lev_watercolrgb[3];
Uint32 lev_watercol;
SDL_Rect cam_rects[4];
SDL_Rect lev_rects[4];
Level lev_level;
Vector gravity;

static int touch_wall (int x, int y)
{
    int x1, y1;
    unsigned char solid;
    for (x1 = x - 3; x1 < x + 4; x1++) {
        if (x1 >= lev_level.width)
            return 0;
        for (y1 = y - 3; y1 < y + 4; y1++) {
            if (y1 >= lev_level.height)
                return 0;
            solid = lev_level.solid[x1][y1];
            if (solid == TER_GROUND || solid == TER_INDESTRUCT
                || solid == TER_BASE || solid == TER_COMBUSTABLE
                || solid == TER_EXPLOSIVE || solid == TER_WALKWAY)
                return 1;
        }
    }

    return 0;
}

static void draw_stars (SDL_Rect * cam, SDL_Rect * targ)
{
    int r, x, y;
    Uint8 *col;
    for (r = 0; r < sizeof(lev_stars)/sizeof(Star); r++) {
        x = cam->x + lev_stars[r].x;
        y = cam->y + lev_stars[r].y;
        col =
            (Uint8 *) lev_level.terrain->pixels +
            y * lev_level.terrain->pitch +
            x * lev_level.terrain->format->BytesPerPixel;
        if (x < lev_level.width && y < lev_level.height)
          if (lev_level.solid[x][y] == TER_FREE && col[0] < 5 && col[1] < 5 && col[2] < 5)
            putpixel (screen, targ->x + lev_stars[r].x,
                      targ->y + lev_stars[r].y, col_white);
    }
}

/* Draw the level for all players */
static inline void draw_level (void)
{
    int p;
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            SDL_BlitSurface (lev_level.terrain, &cam_rects[p], screen,
                             &lev_rects[p]);
            if(level_settings.stars)
                draw_stars (&cam_rects[p], &lev_rects[p]);
        }
    }
}

/* Initialize level subsystem */
void init_level (void)
{
    int r,red, green, blue;
    gravity = makeVector (0, -GRAVITY);
    lev_watercol = map_rgba(0x64,0x64,0xff,0xff);
    level_effects = NULL;
    lev_lastfx = NULL;
    for (r = 0; r < FIRE_FRAMES; r++) {
        if (r == 0)
            red = 0;
        else
            red = 128 + (255.0 / FIRE_FRAMES) * r;
        green = (255.0 / FIRE_FRAMES) * r;
        blue = 0;
        if (red > 255)
            red = 255;
        burncolor[r] = map_rgba(red,green,blue,0xff);
    }
}

/* Calculate star positions. This must be done when player screen */
/* geometry changes. */
void reinit_stars(void) {
    SDL_Rect size = get_viewport_size();
    int r;
    for (r = 0; r < sizeof(lev_stars)/sizeof(Star); r++) {
        lev_stars[r].x = rand () % size.w;
        lev_stars[r].y = rand () % size.h;
    }
}

/* Find the first occurance of a color in the level palette */
static void find_color(Uint8 color,Uint8 *palette,SDL_Palette *source,SDL_Color **target) {
    int c=0;
    while (c < source->ncolors) {
        if (palette[c]==color) {
            *target=source->colors+c;
            break;
        }
        c++;
    }
}

/* Sort base regeneration array */
static int sort_regen(const void *ptr1,const void *ptr2) {
    const RegenCoord *c1=ptr1,*c2=ptr2;
    if(c1->y<c2->y) return 1;
    else if(c1->y==c2->y) return 0;
    else return -1;
}

/* Load level and prepare for new game */
void load_level (struct LevelFile *lev) {
    SDL_Surface *collmap;
    int x, y, p, r;
    int freepix, otherpix;
    SDL_Color *tmpcol, defaultwater;
    Uint8 *bits;
    int basebufsize;

    defaultwater.r = 0;
    defaultwater.g = 0;
    defaultwater.b = 255;

    /* Load level artwork */
    lev_level.terrain = load_level_art (lev);
    lev_level.width = lev_level.terrain->w;
    lev_level.height = lev_level.terrain->h;
    /* Load level collisionmap */
    collmap = load_level_coll (lev);
    if (!collmap) {
        printf ("An error occured while loading collisionmap\n");
        exit (1);
    }
    if (collmap->w != lev_level.width || collmap->h != lev_level.height) {
        printf
            ("Error Collision map image \"%s\" has incorrect size (%dx%d), should be %dx%d!\n",
             lev->settings->mainblock.collmap, collmap->w, collmap->h,
             lev_level.width, lev_level.height);
        exit (1);
    }
    /* Get palette entries */
    /* Get water colour */
    tmpcol = &defaultwater;
    find_color(TER_WATER,lev->settings->palette.entries,
            collmap->format->palette,&tmpcol);
    lev_watercol = map_rgba(tmpcol->r, tmpcol->g, tmpcol->b,0xff);
    lev_watercolrgb[0] = tmpcol->r;
    lev_watercolrgb[1] = tmpcol->g;
    lev_watercolrgb[2] = tmpcol->b;
    /* Calculate underwater clay colour */
    tmpcol->r = (lev_watercolrgb[0] + 255) / 2;
    tmpcol->g = (lev_watercolrgb[0] + 200) / 2;
    tmpcol->b = (lev_watercolrgb[0] + 128) / 2;
    col_clay_uw = map_rgba(tmpcol->r, tmpcol->g, tmpcol->b, 0xff);
    /* Get snow colour */
    tmpcol = NULL;
    find_color(TER_SNOW,lev->settings->palette.entries,
            collmap->format->palette,&tmpcol);
    if (tmpcol) {
        col_snow = map_rgba(tmpcol->r, tmpcol->g, tmpcol->b,0xff);
    }
    /* Prepare base regeneration array */
    if(game_settings.base_regen) {
        basebufsize=512;   /* Some arbitary size */
        lev_level.base = malloc(sizeof(RegenCoord)*basebufsize);
    } else {
        basebufsize=0;
        lev_level.base = NULL;
    }
    /* Load data map */
    lev_level.base_area = 0;
    lev_level.regen_area = 0;
    lev_level.solid = malloc (sizeof (char *) * lev_level.width);
    freepix = 0;
    otherpix = 0;
    for (x = 0; x < lev_level.width; x++) {
        bits = ((Uint8 *) collmap->pixels)+x;
        lev_level.solid[x] = malloc (lev_level.height);
        for (y = 0; y < lev_level.height; y++,bits+=collmap->pitch) {
            lev_level.solid[x][y] = lev->settings->palette.entries[*bits];
            if (lev_level.solid[x][y] == TER_FREE
                || lev_level.solid[x][y] == TER_WATER)
                freepix++;
            else {
                otherpix++;
                if (lev_level.solid[x][y] == TER_BASE) {
                    if(lev_level.base) {
                        Uint8 r,g,b;
                        lev_level.base[lev_level.base_area].x=x;
                        lev_level.base[lev_level.base_area].y=y;
                        SDL_GetRGB(getpixel(lev_level.terrain,x,y),
                                screen->format,
                                &r,
                                &g,
                                &b);
                        lev_level.base[lev_level.base_area].c=
                            map_rgba(r, g, b, 0xff);

                        if(lev_level.base_area==basebufsize-1) {
                            basebufsize+=512;
                            lev_level.base=realloc(lev_level.base,
                                    sizeof(RegenCoord)*basebufsize);
                        }
                    }
                    lev_level.base_area++;
                }
            }
        }
    }
    SDL_FreeSurface (collmap);
    /* Finalize the base regeneration buffer */
    if(lev_level.base) {
        lev_level.regen_timer = 0;
        lev_level.regen_area = lev_level.base_area;
        if(lev_level.base_area<basebufsize) {
            basebufsize=lev_level.base_area;
            lev_level.base=realloc(lev_level.base,
                    sizeof(RegenCoord)*basebufsize);
        }
        qsort(lev_level.base,lev_level.base_area,sizeof(RegenCoord),sort_regen);
    }
    /* Position players */
    if (game_settings.playmode == OutsideShip
        || game_settings.playmode == OutsideShip1)
        y = 2;
    else
        y = 1;
    for (p = 0; p < 4; p++)
        if (players[p].state != INACTIVE) {
            for (x = 0; x < y; x++) {
                r = 0;
                do {
                    lev_level.player_def_x[x][p] = rand () % lev_level.width;
                    lev_level.player_def_y[x][p] = rand () % lev_level.height;
                    r++;
                    if (r > 100000) {
                        fprintf(stderr,
                                "Warning: while searching for player %d startup position, loop counter reached 100000!\n",
                             p);
                        fprintf(stderr,
                            "Number of free space (including water) pixels: %d, number of other pixels: %d\n",
                             freepix, otherpix);
                        fprintf (stderr,
                                "%0.3f%% of the surface area is available\n",
                                ((double) freepix / (double) otherpix) * 100);
                        exit (0);
                    }
                } while (lev_level.
                         solid[lev_level.player_def_x[x][p]][lev_level.
                                                             player_def_y[x]
                                                             [p]] !=
                         TER_FREE);
            }
        }
}

void unload_level (void)
{
    int x;
    struct LevelEffects *next;
    SDL_FreeSurface (lev_level.terrain);
    for (x = 0; x < lev_level.width; x++)
        free (lev_level.solid[x]);
    free (lev_level.solid);
    if(lev_level.base)
        free(lev_level.base);
    while (level_effects) {
        next = level_effects->next;
        free (level_effects->fx);
        free (level_effects);
        level_effects = next;
    }
    lev_lastfx = NULL;
}

int is_walkable (int x, int y)
{
    int s;
    if (y >= lev_level.height - 1)
        return 1;
    s = lev_level.solid[x][y];
    if (s == TER_FREE || s == TER_WATER)
        return 0;
    if (s >= TER_WATERFU && s <= TER_WATERFL)
        return 0;
    if (s == TER_TUNNEL || s == TER_WALKWAY)
        return 0;
    return 1;
}

int is_water (int x, int y)
{
    if (lev_level.solid[x][y] == TER_WATER
        || (lev_level.solid[x][y] >= TER_WATERFU
            && lev_level.solid[x][y] <= TER_WATERFL))
        return 1;
    return 0;
}

/* Pixel perfect collision detection. */
int hit_solid_line (int startx, int starty, int endx, int endy, int *newx,
                     int *newy)
{
    int dx, dy, ax, ay, sx, sy, x, y, d;
    if (endx < 0 || endx >= lev_level.width) {
        *newx = endx<0?0:endx>=lev_level.width?lev_level.width-1:endx;
        *newy = endy<0?0:endy>=lev_level.height?lev_level.height-1:endy;
        return TER_INDESTRUCT;
    }
    if (endy < 0 || endy >= lev_level.height) {
        *newx = endx<0?0:endx>=lev_level.width?lev_level.width-1:endx;
        *newy = endy<0?0:endy>=lev_level.height?lev_level.height-1:endy;
        return TER_INDESTRUCT;
    }
    dx = endx - startx;
    dy = endy - starty;
    ax = abs (dx) << 1;
    ay = abs (dy) << 1;
    sx = (dx >= 0) ? 1 : -1;
    sy = (dy >= 0) ? 1 : -1;
    x = startx;
    y = starty;
    if (ax > ay) {
        d = ay - (ax >> 1);
        while (x != endx) {
            if (hit_solid (x, y) > 0) {
                *newx = x;
                *newy = y;
                return lev_level.solid[x][y];
            }
            if (d > 0 || (d == 0 && sx == 1)) {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    } else {
        d = ax - (ay >> 1);
        while (y != endy) {
            if (hit_solid (x, y) > 0) {
                *newx = x;
                *newy = y;
                return lev_level.solid[x][y];
            }
            if (d > 0 || (d == 0 && sy == 1)) {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
    *newx = endx;
    *newy = endy;
    return hit_solid (startx, starty);
}

int find_rainy (int x)
{
    int y, loops = 0, r, c;
    do {
        y = 15 + rand () % (screen->w/4);
        loops++;
        if (loops > 100)
            return -1;
        c = 0;
        for (r = y; r < y + 25; r++)
            c += lev_level.solid[x][r] != TER_FREE;
    } while (c > 17);
    return y;
}

void start_burning (int x, int y)
{
    struct LevelEffects *newentry;
    LevelFX *fx;
    if (x <= 0 || y <= 0 || x >= lev_level.width || y >= lev_level.height)
        return;
    /* Note this one here. It assumes that the level palette entries go in the order of
       indestructable,water,base .
       This order should not change, so it should be safe to do this 'if' lazily
     */
    if ((lev_level.solid[x][y] >= TER_INDESTRUCT
         && lev_level.solid[x][y] <= TER_BASE)
        || lev_level.solid[x][y] == TER_BASEMAT)
        return;
    newentry = malloc (sizeof (struct LevelEffects));
    newentry->next = NULL;
    newentry->prev = lev_lastfx;
    fx = malloc (sizeof (LevelFX));
    fx->x = x;
    fx->y = y;
    fx->type = Fire;
    fx->value = FIRE_FRAMES;
    if (lev_level.solid[x][y] == TER_COMBUSTABL2)
        lev_level.solid[x][y] = TER_GROUND;
    else
        lev_level.solid[x][y] = TER_FREE;
    newentry->fx = fx;
    if (lev_lastfx == NULL)
        level_effects = newentry;
    else
        lev_lastfx->next = newentry;
    lev_lastfx = newentry;
}

void start_melting (int x, int y, unsigned int recurse)
{
    struct LevelEffects *newentry;
    LevelFX *fx;
    if (x <= 0 || y <= 0 || x >= lev_level.width || y >= lev_level.height
        || recurse == 0)
        return;
    if (lev_level.solid[x][y] == TER_INDESTRUCT
        || lev_level.solid[x][y] == TER_WATER)
        return;
    if ((lev_level.solid[x][y] == TER_BASEMAT
         || lev_level.solid[x][y] == TER_BASE) && level_settings.indstr_base)
        return;
    newentry = malloc (sizeof (struct LevelEffects));
    newentry->next = NULL;
    newentry->prev = lev_lastfx;
    fx = malloc (sizeof (LevelFX));
    fx->x = x;
    fx->y = y;
    fx->type = Melt;
    fx->value = 4;
    fx->brake = recurse;
    if(game_settings.base_regen && lev_level.solid[x][y]==TER_BASE)
        lev_level.base_area--;
    lev_level.solid[x][y] = TER_FREE;
    putpixel (lev_level.terrain, x, y, col_green);
    newentry->fx = fx;
    if (lev_lastfx == NULL)
        level_effects = newentry;
    else
        lev_lastfx->next = newentry;
    lev_lastfx = newentry;
}

void alter_level (int x, int y, int recurse, LevelFXType type)
{
    struct LevelEffects *newentry;
    LevelFX *fx;
    if (recurse == 0)
        return;
    newentry = malloc (sizeof (struct LevelEffects));
    newentry->next = NULL;
    newentry->prev = lev_lastfx;
    fx = malloc (sizeof (LevelFX));
    fx->x = x;
    fx->y = y;
    fx->value = 3;
    fx->type = type;
    fx->brake = recurse;
    fx->icicle = 0;
    if (type == Explosive) {
        lev_level.solid[x][y] = TER_EXPLOSIVE;
    } else if (type == Ice) {
        if (lev_level.solid[x][y] == TER_WATER)
            lev_level.solid[x][y] = TER_ICE;
        else if (lev_level.solid[x][y] == TER_FREE
                 || lev_level.solid[x][y] == TER_TUNNEL)
            lev_level.solid[x][y] = TER_SNOW;
        if (rand () % 15 == 0) {
            fx->icicle = 1;
            fx->value = 6;
        }
    } else {
        if (lev_level.solid[x][y] == TER_FREE
            || lev_level.solid[x][y] == TER_TUNNEL)
            lev_level.solid[x][y] = TER_GROUND;
        else if (lev_level.solid[x][y] == TER_WATER)
            lev_level.solid[x][y] = TER_UNDERWATER;
    }
    newentry->fx = fx;
    if (lev_lastfx == NULL)
        level_effects = newentry;
    else
        lev_lastfx->next = newentry;
    lev_lastfx = newentry;
}

void animate_level (void)
{
    struct LevelEffects *list = level_effects;
    struct LevelEffects *next = NULL;
    double f;
    int v, tx, ty, nx, ny;
    char solid;
    Particle *part;
    /* Base regeneration */
    if(lev_level.base && lev_level.base_area<lev_level.regen_area) {
        if(lev_level.regen_timer>BASE_REGEN_SPEED) {
            int r,x,y;
            lev_level.regen_timer=0;
            for(r=0;r<lev_level.regen_area;r++) {
                x=lev_level.base[r].x;
                y=lev_level.base[r].y;
                if(lev_level.solid[x][y]==TER_FREE) {
                    bump_ship(x,y);
                    lev_level.solid[x][y]=TER_BASE;
                    putpixel (lev_level.terrain, x, y, lev_level.base[r].c);
                    putpixel (lev_level.terrain, x, y, lev_level.base[r].c);
                    lev_level.base_area++;
                    break;
                }
            }
        } else {
            lev_level.regen_timer++;
        }
    }
    /* Level effects */
    while (list) {
        next = list->next;
        if (list->fx->type == Fire) {   /* Fire */
            list->fx->value--;
            if (list->fx->value > 0)
                v = list->fx->value + rand () % FIRE_RANDOM;
            else
                v = list->fx->value;
            if (list->fx->x < 3 || list->fx->y < 3
                || list->fx->x > lev_level.width - 3
                || list->fx->y > lev_level.height - 3) {
                list->fx->value = 0;
            } else {
                solid = lev_level.solid[list->fx->x][list->fx->y];
                if (solid != TER_INDESTRUCT && solid != TER_BASE
                    && solid != TER_BASEMAT)
                    solid = 1;
                else
                    solid = 0;
                if (solid
                    && lev_level.solid[list->fx->x][list->fx->y + 1] ==
                    TER_FREE && list->fx->value < FIRE_SPREAD) {
                    if (lev_level.solid[list->fx->x][list->fx->y] == TER_GROUND)        /* Combustable2 turns into ground, remember ? */
                        putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                                  col_gray);
                    else
                        putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                                  burncolor[0]);
                    list->fx->y -= rand () % 3;
                }
                solid = lev_level.solid[list->fx->x][list->fx->y];
                if (v < FIRE_FRAMES && solid != TER_INDESTRUCT
                    && solid != TER_BASE && solid != TER_BASEMAT) {
                    if (v == 0
                        && lev_level.solid[list->fx->x][list->fx->y] ==
                        TER_GROUND)
                        putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                                  col_gray);
                    else
                        putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                                  burncolor[v]);
                }
                if (list->fx->value == FIRE_SPREAD) {
                    for (f = -M_PI; f < M_PI; f += M_PI / 4.0) {
                        nx = list->fx->x + Round(sin (f) * 3.0);
                        ny = list->fx->y + Round(cos (f) * 3.0);
                        if (nx >= lev_level.width || ny >= lev_level.height
                            || nx <= 0 || ny <= 0)
                            continue;
                        solid = lev_level.solid[nx][ny];
                        if (solid == TER_COMBUSTABLE
                            || solid == TER_COMBUSTABL2)
                            start_burning (nx, ny);
                        else if (solid == TER_EXPLOSIVE)
                            spawn_clusters (nx, ny, 6, Cannon);
                        else if (solid == TER_EXPLOSIVE2)
                            spawn_clusters (nx, ny, 6, 
                                            ((rand () % 11) ==
                                             0) ? Grenade : Cannon);
                        else if (game_settings.enable_smoke
                                 && solid == TER_FREE && cos (f) < 0
                                 && rand () % 3 == 1) {
                            part = make_particle (nx, ny, 9);
                            part->vector.y = 2.5;
                            part->vector.x = -weather_wind_vector;
                            part->color[0] = 255;
                            part->color[1] = 178;
                            part->color[2] = 0;
#if HAVE_LIBSDL_GFX
                            part->rd = 0;
                            part->gd = 0;
                            part->bd = 11;
                            part->ad = -15;
#else
                            part->rd = -17;
                            part->gd = -8;
                            part->bd = 11;
#endif
                        } else if (solid == TER_ICE) {
                            lev_level.solid[nx][ny] = TER_WATER;
                            putpixel (lev_level.terrain, nx, ny,
                                      lev_watercol);
                        } else if (solid == TER_SNOW) {
                            lev_level.solid[nx][ny] = TER_FREE;
                            putpixel (lev_level.terrain, nx, ny,
                                      burncolor[0]);
                        } else if (solid == TER_WALKWAY)
                            putpixel (lev_level.terrain, nx, ny, col_gray);
                    }
                }
            }
        } else if (list->fx->type == Melt) {    /* Acid */
            list->fx->value--;
            if (list->fx->value == 1) {
                if (list->fx->brake)
                    list->fx->brake--;
                putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                          col_black);
                for (f = -M_PI; f < M_PI; f += M_PI / 4.0) {
                    nx = list->fx->x + Round(sin (f) * 3.0);
                    ny = list->fx->y + Round(cos (f) * 3.0);
                    if (nx >= lev_level.width || ny >= lev_level.height
                        || nx <= 0 || ny <= 0)
                        continue;
                    solid = lev_level.solid[nx][ny];
                    if (solid == TER_GROUND || solid == TER_COMBUSTABLE
                        || solid == TER_COMBUSTABL2 || solid == TER_SNOW
                        || solid == TER_TUNNEL || solid == TER_WALKWAY)
                        start_melting (nx, ny, list->fx->brake);
                    else if ((solid == TER_BASE || solid == TER_BASEMAT)
                             && !level_settings.indstr_base)
                        start_melting (nx, ny, list->fx->brake);
                    else if (game_settings.enable_smoke && solid == TER_FREE
                             && cos (f) < 0 && rand () % 3 == 1) {
                        part = make_particle (nx, ny, 9);
                        part->vector.y = 2.5;
                        part->vector.x = -weather_wind_vector;
                        part->color[0] = 0;
                        part->color[1] = 255;
                        part->color[2] = 0;
                        part->rd = -22;
                        part->gd = -28;
                        part->bd = 22;
                    }
                }
            }
        } else {                /* Ice, Earth or Explosive */
            solid = lev_level.solid[list->fx->x][list->fx->y];
            if (list->fx->value > 0)
                list->fx->value--;
            if (list->fx->icicle
                && lev_level.solid[list->fx->x][list->fx->y + 1] ==
                TER_FREE) {
                list->fx->y++;
                putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                          col_snow);
            }
            if (list->fx->value == 1) {
                if (list->fx->type == Earth) {
                    if (solid == TER_UNDERWATER)
                        putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                                  col_clay_uw);
                    else
                        putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                                  col_clay);
                } else if (list->fx->type == Ice)
                    putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                              col_snow);
                else
                    putpixel (lev_level.terrain, list->fx->x, list->fx->y,
                              col_gray);
                if (list->fx->brake > 0)
                    list->fx->brake--;
                if (list->fx->x > 5 && list->fx->y > 5
                    && list->fx->x < lev_level.width - 5
                    && list->fx->y < lev_level.height - 5) {
                    if (list->fx->type == Ice
                        && (solid == TER_GROUND || solid == TER_FREE
                            || solid == TER_SNOW || solid == TER_INDESTRUCT
                            || solid == TER_COMBUSTABLE
                            || solid == TER_COMBUSTABL2
                            || solid == TER_WALKWAY)) {
                        for (tx = list->fx->x - 3; tx < list->fx->x + 4; tx++)
                            for (ty = list->fx->y - 3; ty < list->fx->y + 4;
                                 ty++) {
                                if (lev_level.solid[tx][ty] == TER_WATER)
                                    alter_level (tx, ty, list->fx->brake,
                                                 Ice);
                                else if ((lev_level.solid[tx][ty] == TER_FREE
                                          || lev_level.solid[tx][ty] ==
                                          TER_TUNNEL) && touch_wall (tx, ty))
                                    alter_level (tx, ty, list->fx->brake,
                                                 list->fx->type);
                            }
                    } else {
                        for (f = -M_PI; f < M_PI; f += M_PI / 4.0) {
                            solid =
                                lev_level.
                                solid[(int) (list->fx->x + sin (f) * 4.0)][(int) (list->fx->y + cos (f) * 4.0)];
                            if (list->fx->type == Ice) {
                                if (solid == TER_WATER && list->fx->brake)
                                    alter_level ((int)
                                                 (list->fx->x +
                                                  sin (f) * 4.0),
                                                 (int) (list->fx->y +
                                                        cos (f) * 4.0),
                                                 list->fx->brake,
                                                 list->fx->type);
                            } else if (list->fx->type == Earth) {
                                if ((solid == TER_FREE || solid == TER_WATER
                                     || solid == TER_TUNNEL)
                                    && list->fx->brake)
                                    alter_level ((int)
                                                 (list->fx->x +
                                                  sin (f) * 4.0),
                                                 (int) (list->fx->y +
                                                        cos (f) * 4.0),
                                                 list->fx->brake,
                                                 list->fx->type);
                            } else {
                                if ((solid == TER_GROUND
                                     || solid == TER_COMBUSTABLE
                                     || solid == TER_COMBUSTABL2)
                                    && list->fx->brake)
                                    alter_level ((int)
                                                 (list->fx->x +
                                                  sin (f) * 4.0),
                                                 (int) (list->fx->y +
                                                        cos (f) * 4.0),
                                                 list->fx->brake,
                                                 list->fx->type);
                            }
                        }
                    }
                }
            }
        }
        if (list->fx->value == 0) {
            free (list->fx);
            if (list->prev)
                list->prev->next = list->next;
            else
                level_effects = list->next;
            if (list->next)
                list->next->prev = list->prev;
            else
                lev_lastfx = list->prev;
            free (list);
        }
        list = next;
    }
    draw_level ();
}


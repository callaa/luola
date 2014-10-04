/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : weather.c
 * Description : Weather effects
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

#include "console.h"
#include "list.h"
#include "game.h"
#include "level.h"
#include "player.h"
#include "decor.h"

#define SNOWFLAKE_INTERVAL      20
#define MAX_WIND_TIME	400     /* Maximium time in frames that a breeze can last */

struct SnowSource {
    int x, y;
    int disable;
    int snowtimer;
    int movetimer;
};

/* Internally used globals */
static struct dllist *decor_list;
static struct SnowSource snowsource[32];
static int weather_wind_targ_vector;
static int weather_windy=1;

/* Exported globals */
double weather_wind_vector=0; /* Ok, so this really isn't a vector, but we only need the X component */

/* Prepare weather for the next level */
void prepare_decorations (void)
{
    static const int sscount = sizeof(snowsource)/sizeof(struct SnowSource);
    int r;

    /* Clear up any old decorations */
    dllist_free(decor_list,free);
    decor_list = NULL;

    /* Currently, snowfall is the only type of weather supported */
    if (level_settings.snowfall == 0)
        return;
    for (r = 0; r < sscount; r++) {
        snowsource[r].x = r * (lev_level.width / sscount) + rand () % 15;
        if (snowsource[r].x+sscount >= lev_level.width) {
            snowsource[r].disable = 1;
            continue;
        }
        snowsource[r].y = find_rainy (snowsource[r].x);
        if (snowsource[r].y < 0) {
            snowsource[r].disable = 1;
            continue;
        } else {
            snowsource[r].disable = 0;
        }

        snowsource[r].snowtimer = SNOWFLAKE_INTERVAL;
        snowsource[r].movetimer = 0;
    }
}

/* Make a snowflake */
struct Decor *make_snowflake(double x,double y, Vector v) {
    struct Decor *d = malloc(sizeof(struct Decor));
    if(!d) {
        perror("make_snowflake");
        return NULL;
    }

    init_physobj(&d->physics,x,y,v);
    d->physics.mass = 0.55;

    d->color = col_snow;
    d->jitter = 1;

    return d;
}

/* Make a drop of water */
struct Decor *make_waterdrop(double x,double y, Vector v) {
    struct Decor *d = malloc(sizeof(struct Decor));
    if(!d) {
        perror(__func__);
        return NULL;
    }

    init_physobj(&d->physics,x,y,v);

    d->color = lev_watercol;
    d->jitter = 0;

    return d;
}

/* Make a feather */
struct Decor *make_feather(double x,double y, Vector v) {
    struct Decor *d = malloc(sizeof(struct Decor));
    if(!d) {
        perror(__func__);
        return NULL;
    }

    init_physobj(&d->physics,x,y,v);
    d->physics.solidity = WRAITH;
    d->physics.mass = 0.8;

    d->color = col_translucent;
    d->jitter = 1;

    return d;
}

/* Make a drop of blood */
struct Decor *make_blood(double x,double y, Vector v) {
    struct Decor *d = make_waterdrop(x,y,v);
    d->color = col_red;

    return d;
}

/* Add a decoration particle to list */
void add_decor(struct Decor *d) {
    decor_list = dllist_prepend(decor_list,d);
}

/* Add a cluster of decoration particles */
void add_splash (double x, double y, double f,int count, Vector v,
    struct Decor *(*make_decor)(double x, double y, Vector v))
{
    double angle,incr = (2.0 * M_PI) / count;
    for(angle=0;angle<2.0*M_PI;angle+=incr) {
        double r = (rand () % 5) / 5.0;
        double dx = sin(angle + r);
        double dy = cos(angle + r);
        add_decor (make_decor (x + dx*2, y + dy*2,
                    addVectors(v,makeVector (dx * f, dy * f))));

    }
}

/* Draw on screen */
static inline void draw_decor(struct Decor *d)
{
    int x, y, p;
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            x = Round(d->physics.x) - cam_rects[p].x + viewport_rects[p].x;
            y = Round(d->physics.y) - cam_rects[p].y + viewport_rects[p].y;
            if ((x > viewport_rects[p].x &&
                        x < viewport_rects[p].x + cam_rects[p].w) &&
                    (y > viewport_rects[p].y &&
                     y < viewport_rects[p].y + cam_rects[p].h)) {
                putpixel (screen, x, y, d->color);
            }
        }
    }
}

/* Create new snow */
static void snowfall(void) {
    static const int sscount = sizeof(snowsource)/sizeof(struct SnowSource);
    int r;
    for(r=0;r<sscount;r++) {
        if(snowsource[r].disable==0) {
            snowsource[r].snowtimer--;
            snowsource[r].movetimer++;

            if(snowsource[r].movetimer < sscount) {
                snowsource[r].x++;
            } else {
                snowsource[r].x--;
                if (snowsource[r].movetimer ==  sscount * 2)
                    snowsource[r].movetimer = 0;
            }

            if(snowsource[r].snowtimer == 0) {
                add_decor(make_snowflake(snowsource[r].x,snowsource[r].y,
                            makeVector(0,0)));
                snowsource[r].snowtimer = SNOWFLAKE_INTERVAL;
            }
        }
    }
}

/* Animate */
void animate_decorations(void) {
    struct dllist *list=decor_list;
    /* Update wind vector */
    if (weather_windy <= 0) {
        int tmpi;
        weather_windy = rand () % MAX_WIND_TIME;
        tmpi = rand () % 5;
        if (rand () % 3 == 0) {
            if (weather_wind_targ_vector < 0)
                weather_wind_targ_vector = tmpi;
            else
                weather_wind_targ_vector = -tmpi;
        } else {
            if (weather_wind_targ_vector < 0)
                weather_wind_targ_vector = -tmpi;
            else
                weather_wind_targ_vector = tmpi;
        }
    } else {
        weather_windy--;
    }
    if (weather_wind_targ_vector < weather_wind_vector)
        weather_wind_vector -= 0.05;
    else if (weather_wind_targ_vector > weather_wind_vector)
        weather_wind_vector += 0.05;

    /* Animate snowfall if enabled */
    if(level_settings.snowfall)
        snowfall();

    /* Animate decoration particles */
    while (list) {
        struct dllist *next=list->next;
        struct Decor *d=list->data;

        /* Add wind and jitter */
        d->physics.vel.x += weather_wind_vector/100.0;
        if(d->jitter)
            d->physics.x += 2 - rand()%4;

        /* Animate object */
        animate_object(&d->physics,0,NULL);
        if(d->physics.hitground || d->physics.underwater) {
            if(d->color == col_snow)
                alter_level (Round(d->physics.x), Round(d->physics.y) - 1,
                        1, Ice);
            free(d);
            if(list==decor_list)
                decor_list = dllist_remove(list);
            else
                dllist_remove(list);
        } else {
            draw_decor(d);
        }
        list = next;
    }
}


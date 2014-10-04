/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
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

#include "console.h"
#include "list.h"
#include "game.h"
#include "level.h"
#include "player.h"
#include "weather.h"

#define SNOWFLAKE_INTERVAL      20
#define MAX_WIND_TIME	400     /* Maximium time in frames that a breeze can last */

/* List of weather effects */
struct WeatherFX {
    WeatherFX type;
    int x, y;
    int timer1;
    int timer2;
};

/* Internally used globals */
static struct dllist *weather_list=NULL;
static int weather_snowsource_i2;
static int weather_wind_targ_vector;
static int weather_windy=1;

/* Exported globals */
double weather_wind_vector=0; /* Ok, so this really isn't a vector, but we only need the X component */

/* Prepare weather for the next level */
void prepare_weather (void)
{
    struct WeatherFX *newfx;
    int r;
    /* Clear up old weather */
    dllist_free(weather_list,free);
    weather_list=NULL;

    /* Currently, snowfall is the only type of weather supported */
    if (level_settings.snowfall == 0)
        return;
    weather_snowsource_i2 = lev_level.width / 15;
    for (r = 1; r < weather_snowsource_i2; r++) {
        newfx = malloc (sizeof (struct WeatherFX));
        newfx->x = r * lev_level.width / 15 + rand () % 15;
        if (newfx->x >= lev_level.width) {
            free (newfx);
            continue;
        }
        newfx->y = find_rainy (newfx->x);
        if (newfx->y < 0) {
            free (newfx);
            continue;
        }
        newfx->type = Snowsource;
        newfx->timer1 = SNOWFLAKE_INTERVAL;
        newfx->timer2 = 0;
        if(weather_list)
            dllist_append(weather_list,newfx);
        else
            weather_list=dllist_append(weather_list,newfx);
    }
}

/* Add a snowflake */
void add_snowflake (int x, int y)
{
    struct WeatherFX *newfx;
    newfx = malloc (sizeof (struct WeatherFX));
    newfx->x = x;
    newfx->y = y;
    newfx->type = Snowflake;
    if (weather_list)
        dllist_append(weather_list,newfx);
    else
        weather_list=dllist_append(weather_list,newfx);
}

/* Draw on screen */
static inline void draw_weather_fx (struct WeatherFX *fx)
{
    int x, y, p;
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            x = fx->x - cam_rects[p].x + lev_rects[p].x;
            y = fx->y - cam_rects[p].y + lev_rects[p].y;
            if ((x > lev_rects[p].x && x < lev_rects[p].x + cam_rects[p].w)
                && (y > lev_rects[p].y
                    && y < lev_rects[p].y + cam_rects[p].h)) {
                putpixel (screen, x, y, col_snow);
            }
        }
    }
}

/* Animate */
void animate_weather (void) {
    struct dllist *list=weather_list;
    signed char solid;
    int dx;
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
    while (list) {
        struct WeatherFX *fx=list->data;
        struct dllist *next=list->next;
        switch (fx->type) {
        case Snowsource:
            fx->timer1--;
            fx->timer2++;
            if (fx->timer2 < weather_snowsource_i2)
                fx->x++;
            else {
                fx->x--;
                if (fx->timer2 == weather_snowsource_i2 * 2)
                    fx->timer2 = 0;
            }
            if (fx->timer1 == 0) {
                add_snowflake (fx->x, fx->y);
                fx->timer1 = SNOWFLAKE_INTERVAL;
            }
            break;
        case Snowflake:
            fx->y++;
            dx = (rand () % 4) - 2 + Round (weather_wind_vector);
            fx->x += dx;
            if (fx->y >= lev_level.height || fx->x <= 1 || fx->x >= lev_level.width-1) {
                free(fx);
                if(list==weather_list)
                    weather_list=dllist_remove(list);
                else
                    dllist_remove(list);
                break;
            }
            solid = hit_solid (fx->x, fx->y);
            if (solid) {
                if( hit_solid(fx->x+(dx<0?1:-1),fx->y) ) {
                    alter_level (fx->x, fx->y - 1, 1, Ice);     /* change the recurse value (1) into 2 to get a nice thick coating of snow... ;) */
                    free(fx);
                    if(list==weather_list)
                        weather_list=dllist_remove(list);
                    else
                        dllist_remove(list);
                }
            }
            break;
        }
        if (fx && fx->type == Snowflake)
            draw_weather_fx (fx);
        list = next;
    }
}


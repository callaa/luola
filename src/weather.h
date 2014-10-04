/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : weather.h
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

#ifndef L_WEATHER_H
#define L_WEATHER_H

/* Types */
typedef enum { Snowsource, Snowflake } WeatherFX;

/* Prepare weather for the next level */
extern void prepare_weather (void);

/* Handling */
extern void add_snowflake (int x, int y);
/* Animation */
extern void animate_weather (void);

/* Globals */
extern double weather_wind_vector;

#endif

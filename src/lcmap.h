/*
 * LCMAP - An image format designed to store Luola collision maps.
 * Copyright (C) 2003 Calle Laakkonen
 *
 * File        : lcmap.h
 * Description : Convert LCMAPs into and from SDL_Surfaces
 * Author(s)   : Calle Laakkonen
 *
 * LCMAP is free software; you can redistribute it and/or modify
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

#ifndef LCMAP_H
#define LCMAP_H

#include "SDL.h"

extern SDL_Surface *lcmap_to_surface (Uint8 * lcmap, int len);
extern SDL_Surface *lcmap_to_surface_rw (SDL_RWops * rw);
extern Uint8 *surface_to_lcmap (Uint32 * len, SDL_Surface * surface);

#endif

/*
 * SBmp - Small bitmap library
 * Copyright (C) 2003 Calle Laakkonen
 *
 * File        : sbmp.h
 * Description : Convert Small Bitmaps into and from SDL_Surfaces
 * Author(s)   : Calle Laakkonen
 *
 * SBmp is free software; you can redistribute it and/or modify
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

#ifndef SBMP_H
#define SBMP_H

#include <SDL.h>

/* This converts a Small Bitmap binary to an SDL surface */
SDL_Surface *sbmp_to_surface (Uint8 * sbmp);

/* This converts an SDL surface into a Small Bitmap binary */
Uint8 *surface_to_sbmp (Uint32 * len, SDL_Surface * surface);

#endif

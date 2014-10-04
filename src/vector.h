/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003 Calle Laakkonen
 *
 * File        : vector.h
 * Description : Vector math
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

#ifndef L_VECTOR_H
#define L_VECTOR_H

#ifdef WIN32
#define inline __inline
#endif

typedef struct {
    double x;
    double y;
} Vector;

extern Vector makeVector (double X, double Y);
extern Vector oppositeVector (Vector * vec);
extern void rotateVector(Vector *vec, double th);

static inline Vector addVectors (Vector * v1, Vector * v2)
{
    Vector result;
    result.x = v1->x + v2->x;
    result.y = v1->y + v2->y;
    return result;
}

#endif

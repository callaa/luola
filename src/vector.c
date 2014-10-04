/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : vector.c
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

#include <math.h>

#include "vector.h"

Vector makeVector (double X, double Y)
{
    Vector newVec;
    newVec.x = X;
    newVec.y = Y;
    return newVec;
}

Vector oppositeVector (Vector * vec)
{
    Vector opp;
    opp.x = -vec->x;
    opp.y = -vec->y;
    return opp;
}

void rotateVector(Vector *vec, double th) {
    double c = cos(th);
    double s = sin(th);
    double tx = c * vec->x - s * vec->y;
    vec->y = s * vec->x + c * vec->y;
    vec->x = tx;
}


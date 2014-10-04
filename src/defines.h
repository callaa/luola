/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : defines.h
 * Description : Miscallenous definitions
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

#ifndef DEFINES_H
#define DEFINES_H

/* Speeds */
#define FADE_STEP               35 /* How many steps in fade animation */

/* Inlines instead of macros for type safety and to prevent against ++args */
/* Function names start with uppercase letter to prevent name clash */
/* with C99 round functions */
static inline int Round (double a)
{
    return (a < 0 ? a - 0.5 : a + 0.5);
}
static inline int Roundplus (double a)
{
    return (a < 0 ? 0 : a + 0.5);
}

#if 0
/*static inline int LUOLA_MAX(int a, int b) { return (a<b ? b : a); }*/
static inline int LUOLA_MIN (int a, int b)
{
    return (a > b ? b : a);
}
#endif

#endif


/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : stringutil.h
 * Description : Miscallenous string operations
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

#ifndef STRINGUTIL_H
#define STRINGUTIL_H

/* Remove whitespace from beginning and end of the string */
extern char *strip_white_space (const char *str);

/* Split a string in two parts. Whitespace is removed */
extern int split_string (char *str, char delim, char **left, char **right);

extern const char *controller_name (int type);
extern const char *weap2str (int weapon);
extern const char *sweap2str (int weapon);
extern const char *critical2str (int critical);
extern const char *critter2str (int critter);
extern int name2terrain(const char *name);

#endif

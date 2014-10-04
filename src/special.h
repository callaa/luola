/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : special.h
 * Description : Level special (eg. turrets and jump-gates) handling and animation
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

#ifndef L_SPECIAL_H
#define L_SPECIAL_H

#include "weapon.h"
#include "lconf.h"

typedef enum { Unknown, JumpGate, WarpExit, WarpEntry, Turret } SpecialType;

typedef struct _SpecialObj {
    int x, y;
    SpecialType type;
    signed char owner;
    int age;
    int frame;
    int timer;
    int var1;
    int var2;
    float health;
    struct _SpecialObj *link;
} SpecialObj;

/* Initialization */
extern int init_specials (void);
extern void clear_specials (void);
extern void prepare_specials (LevelSettings * settings);

/* Append a new special object to the list */
extern void addspecial (SpecialObj * special);
/* Convenience functions */
extern void drop_jumppoint (int x, int y, char player);

/* Animation */
extern void animate_specials (void);
extern int projectile_hit_special (Projectile * proj);    /* Weapons */

#endif

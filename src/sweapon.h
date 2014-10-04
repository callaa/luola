/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : sweapon.h
 * Description : Special weapon specific code
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

#ifndef L_SWEAPON_H
#define L_SWEAPON_H

#include "weapon.h"

/* Certain weapons have special movement code */
/* Note. This code might not have actually anything to do with movement, */
/* it might as well be code for a cloaking device or a smoketrail */
extern void missile_movement (Projectile * proj);
extern void magmine_movement (Projectile * proj);
extern void dividingmine_movement (Projectile * proj,struct dllist *others);
extern void mine_movement (Projectile * proj);
extern void rocket_movement (Projectile * proj);
extern void spear_movement (Projectile * proj);
extern void energy_movement (Projectile * proj);
extern void magwave_movement (Projectile * proj);
extern void tag_movement (Projectile * proj);
extern void acid_movement (Projectile * proj);
extern void ember_movement (Projectile * proj);
extern void napalm_movement (Projectile * proj);
extern void water_movement (Projectile * proj);
extern void boomerang_movement (Projectile * proj);
extern void mush_movement (Projectile * proj);
extern void mirv_movement (Projectile * proj);

#endif

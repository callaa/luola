/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : bullet.h
 * Description : Weapon projectile creation code
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

#ifndef BULLET_H
#define BULLET_H

#include "projectile.h"

/* Spawn a cluster of projectiles */
extern void spawn_clusters (double x, double y, double f,int count,
            struct Projectile *(*make_projectile)(double x, double y, Vector v));

/* Create projectiles */
extern struct Projectile *make_firestarter(double x,double y, Vector v);
extern struct Projectile *make_bullet(double x,double y, Vector v);
extern struct Projectile *make_grenade(double x,double y, Vector v);
extern struct Projectile *make_megabomb(double x,double y, Vector v);
extern struct Projectile *make_missile(double x,double y, Vector v);
extern struct Projectile *make_mine(double x,double y, Vector v);
extern struct Projectile *make_magmine(double x,double y, Vector v);
extern struct Projectile *make_divmine(double x,double y, Vector v);
extern struct Projectile *make_claybomb(double x,double y, Vector v);
extern struct Projectile *make_snowball(double x,double y, Vector v);
extern struct Projectile *make_plastique(double x,double y, Vector v);
extern struct Projectile *make_spear(double x,double y, Vector v);
extern struct Projectile *make_acid(double x,double y, Vector v);
extern struct Projectile *make_zap(double x,double y, Vector v);
extern struct Projectile *make_waterjet(double x,double y, Vector v);
extern struct Projectile *make_rocket(double x,double y, Vector v);
extern struct Projectile *make_mirv(double x,double y, Vector v);
extern struct Projectile *make_boomerang(double x,double y, Vector v);
extern struct Projectile *make_mush(double x,double y, Vector v);
extern struct Projectile *make_ember(double x,double y, Vector v);
extern struct Projectile *make_napalm(double x,double y, Vector v);
extern struct Projectile *make_mwave(double x,double y, Vector v);
extern struct Projectile *make_emwave(double x,double y, Vector v);
extern struct Projectile *make_gwell(double x,double y, Vector v);
extern struct Projectile *make_landmine(double x,double y, Vector v);

/* Detonate a land mine that belongs to a specific ship. If no landmines */
/* are found, returns 0 */
extern int detonate_landmine(struct Ship *owner);

#endif


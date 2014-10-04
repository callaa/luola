/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : weapon.h
 * Description : Weapon firing code
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

#ifndef WEAPON_H
#define WEAPON_H

#include "bullet.h"
#include "audio.h"

struct NormalWeapon {
    const char *name;
    int cooloff;

    void (*fire)(struct Ship *ship);
};

struct SpecialWeapon {
    int id;             /* Some weapons need extra identification */
    const char *name;   /* Name of the weapon that is shown to user */
    float energy;       /* How much energy does a single shot consume */
    int cooloff;        /* Required cooloff time between shots */
    int not_waterproof; /* Does the weapon not function underwater */
    int singleshot;     /* Disable autofire */
    AudioSample sfx;    /* Sound effect played when weapon is fired */

    /* Create a bullet. Simple weapons that are fired from the nose of */
    /* the ship with normal velocity (+ship velocity) use this */
    struct Projectile *(*make_bullet)(double x, double y, Vector v);

    /* More complex weapons use this */
    void (*fire)(struct Ship *ship, double x, double y, Vector v);
};

/* Special weapon identification */
/* These weapons have special code elsewhere */
#define WEAP_AUTOREPAIR 1

/* Get the number of weapons available */
extern int normal_weapon_count(void);
extern int special_weapon_count(void);

/* Primary weapons */
extern const struct NormalWeapon normal_weapon[];

/* Special weapons */
extern const struct SpecialWeapon special_weapon[];

extern Vector get_bullet_offset(double angle);
extern Vector get_muzzle_vel(double angle);

#endif


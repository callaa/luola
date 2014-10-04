/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : weapon.h
 * Description : Weapon code
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

#ifndef L_WEAPON_H
#define L_WEAPON_H

#include "SDL.h"
#include "vector.h"
#include "game.h"
#include "weapon.h"
#include "list.h"

typedef enum { Noproj, Decor, FireStarter, Handgun, Napalm, Cannon, Grenade,
        MegaBomb,
    Missile, MagMine, Mine, Claybomb, Plastique,
    Snowball, Landmine, Spear, GravityWell,
    Zap, Rocket, Energy, Boomerang,
    DividingMine, Tag, Mush, Waterjet,
    Ember, Acid, Mirv, MagWave
} ProjectileType;

typedef enum { WCannon, WGrenade, WMegaBomb, WShotgun,
    WMissile, WCloak, WMagMine,
    WMine, WShield, WGhostShip,
    WAfterBurner, WWarp,
    WClaybomb, WPlastique, WSnowball,
    WDart, WLandmine, WRepair,
    WInfantry, WHelicopter, WSpeargun,
    WGravGun, WGravMine, WZapper,
    WRocket, WEnergy, WBoomerang,
    WRemote, WDivide, WTag, WMush,
    WWatergun, WEmber, WAcid, WMirv,
    WFlame, WEMP, WAntigrav
} WeaponType;

#define WeaponCount 38

/* Define types for the standard weapons  */
typedef enum { SShot, S3ShotWide, S3ShotTight, SSweep, S4Way } SWeaponType;
#define SWeaponCount 5

struct Ship;

typedef struct {
    double x,y;
    Vector vector;
    Vector *gravity;
    float angle;
    double maxspeed;
    Uint32 color;
    ProjectileType type;
    char wind_affects;
    unsigned char primed;
    int ownerteam;
    struct Ship *owner;
    int life;
    Uint32 var1;
} Projectile;

/* Initialization */
extern void init_weapons (LDAT *explosionfile);
extern void clear_weapons (void);

/* Handling */
extern Projectile *make_projectile (double x, double y, Vector v);
extern void add_projectile (Projectile * proj);
extern void spawn_clusters (int x, int y, int count, ProjectileType type);
extern void add_explosion (int x, int y, ProjectileType cluster);
extern int detonate_remote (struct Ship *plr);
extern const char *weap2str (int weapon);
extern const char *sweap2str (int weapon);

/* Animation */
extern void animate_weapons (void);
extern void draw_weapons (void);

#endif


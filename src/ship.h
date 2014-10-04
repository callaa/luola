/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : ship.h
 * Description : Ship information and animation
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

#ifndef SHIP_H
#define SHIP_H

#include "SDL.h"
#include "vector.h"
#include "weapon.h"
#include "list.h"

typedef enum { Grey, Red, Blue, Green, Yellow, White, Frozen } PlayerColor;
typedef enum { NormalWeapon, SpecialWeapon, SpecialWeapon2 } WeaponClass;

struct Ship {
    double x, y;
    PlayerColor color;
    SDL_Surface **ship;
    SDL_Surface *shield;
    int anim;
    Vector vector;
    double angle;
    int thrust;
    float turn;
    float maxspeed;
    double health;
    double energy;
    int tagged;
    Uint8 repeat_audio;
    Uint8 vwing_exhaust;
    int carrying;
    char onbase;
    /* Mostly weapons related stuff */
    unsigned char criticals;
    int no_power;
    int cooloff, special_cooloff, eject_cooloff;
    int fire_weapon;
    int fire_special_weapon;
    int dead;
    int visible;
    int white_ship;
    int shieldup;
    int ghost;
    int afterburn;
    int frozen;
    enum {NODART,DARTING,SPEARED} darting;
    int repairing;
    int antigrav;
    struct Ship *remote_control;
    double sweep_angle;
    int sweeping_down;
    WeaponType special;
    SWeaponType standard;
};

/* Initialization */
extern int init_ships (void);
extern void clean_ships (void);
extern void reinit_ships (LevelSettings * settings);
extern struct Ship *create_ship (PlayerColor color, SWeaponType weapon,
                          WeaponType special);

/* Animation */
extern void animate_ships (void);
extern void draw_ships (void);

/* Handling */
extern void ship_fire (struct Ship * ship, WeaponClass weapon);
extern struct Ship *hit_ship (double x, double y, struct Ship *ignore, double radius);
extern int find_nearest_player (int myX, int myY, int not_this, double *dist);
struct Ship *find_nearest_ship (int myX, int myY, struct Ship * not_this, double *dist);
extern signed char ship_damage (struct Ship * ship, Projectile * proj);
extern void killship (struct Ship * ship);
extern void claim_ship (struct Ship * ship, int plr);
extern void ship_critical (struct Ship * ship, int repair);
extern void bump_ship(int x,int y);

/* Globals */
extern SDL_Surface **ship_gfx[7];
extern struct dllist *ship_list;

#endif

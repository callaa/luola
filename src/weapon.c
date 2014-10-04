/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : weapon.c
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

#include <math.h>
#include <stdlib.h>

#include "ship.h"
#include "weapon.h"
#include "special.h"
#include "critter.h"
#include "player.h"
#include "audio.h"

/* Get an offset at the tip of the ship */
Vector get_bullet_offset(double angle) {
    Vector offset = {cos(angle) * 5.0, -sin(angle) * 5.0};
    return offset;
}

/* Calculate muzzle velocity */
Vector get_muzzle_vel(double angle) {
    Vector vel = {cos(angle) * 11.5, -sin(angle) * 11.5};
    return vel;
}

/*** PRIMARY WEAPONS ***/
static void fire_cannon(struct Ship *ship) {
    Vector offset = get_bullet_offset(ship->angle);
    Vector mvel = get_muzzle_vel(ship->angle);
    add_projectile(make_bullet(ship->physics.x + offset.x,
                ship->physics.y + offset.y,
                addVectors(mvel, ship->physics.vel)));
}

static void fire_sweep(struct Ship *ship) {
    Vector offset = get_bullet_offset(ship->angle+ship->sweep_angle);
    Vector mvel = get_muzzle_vel(ship->angle+ship->sweep_angle);
    if((ship->sweep_angle >= -0.40) && (ship->sweeping_down)) {
        ship->sweep_angle -= 0.1;
        if(ship->sweep_angle < -0.40)
            ship->sweeping_down = 0;
    } else if(ship->sweep_angle <= 0.40) {
        ship->sweep_angle += 0.1;
        if(ship->sweep_angle > 0.40)
            ship->sweeping_down = 1;
    }

    add_projectile(make_bullet(ship->physics.x + offset.x,
                ship->physics.y + offset.y,
                addVectors(mvel, ship->physics.vel)));
}

static void fire_wtriple(struct Ship *ship) {
    Vector offset = get_bullet_offset(ship->angle);
    Vector mvel = get_muzzle_vel(ship->angle);
    add_projectile(make_bullet(ship->physics.x+offset.x,
                ship->physics.y+offset.y,
                addVectors(mvel, ship->physics.vel)));

    rotateVector(&mvel, -0.10);
    add_projectile(make_bullet(ship->physics.x+offset.x,
                ship->physics.y+offset.y,
                addVectors(mvel, ship->physics.vel)));

    rotateVector(&mvel, 0.20);
    add_projectile(make_bullet(ship->physics.x+offset.x,
                ship->physics.y+offset.y,
                addVectors(mvel, ship->physics.vel)));
}

static void fire_ttriple(struct Ship *ship) {
    Vector offset = get_bullet_offset(ship->angle);
    Vector mvel = get_muzzle_vel(ship->angle);
    add_projectile(make_bullet(ship->physics.x+offset.x,
                ship->physics.y+offset.y,
                addVectors(mvel, ship->physics.vel)));

    offset.x = cos(ship->angle - 1.2) * 3.0;
    offset.y = -sin(ship->angle - 1.2) * 3.0;
    add_projectile(make_bullet(ship->physics.x-offset.x,
                ship->physics.y-offset.y,
                addVectors(mvel, ship->physics.vel)));

    offset.x = cos(ship->angle + 1.2) * 3.0;
    offset.y = -sin(ship->angle + 1.2) * 3.0;
    add_projectile(make_bullet(ship->physics.x-offset.x,
                ship->physics.y-offset.y,
                addVectors(mvel, ship->physics.vel)));

}

static void fire_quad(struct Ship *ship) {
    int r;

    for(r=0;r<4;r++) {
        Vector offset = get_bullet_offset(ship->angle + r*M_PI_2);
        Vector mvel = get_muzzle_vel(ship->angle + r*M_PI_2);

        add_projectile(make_bullet(ship->physics.x+offset.x,
                    ship->physics.y+offset.y,
                    addVectors(mvel, ship->physics.vel)));
    }

}

/*** SPECIAL WEAPONS ***/
static void drop_megabomb(struct Ship *ship, double x, double y, Vector v)
{
    add_projectile(make_megabomb(ship->physics.x,ship->physics.y+11,
                multVector(ship->physics.vel,0.5)));
}

static void drop_ember(struct Ship *ship, double x, double y, Vector v)
{
    add_projectile(make_ember(ship->physics.x,ship->physics.y+11,
                multVector(ship->physics.vel,0.5)));
}

static void fire_shotgun(struct Ship *ship, double x, double y, Vector v)
{
    double a;
    for(a=-0.25;a<0.25;a+=0.08) {
        Vector mvel = get_muzzle_vel(ship->angle+a);
        add_projectile(make_bullet(x,y,addVectors(mvel, ship->physics.vel)));
    }
    ship->physics.vel.x -= cos (ship->angle) * 5.0;  /* Recoil */
    ship->physics.vel.y += sin (ship->angle) * 5.0;
}

static void fire_missile(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p = make_missile(x,y,v);
    p->angle = 2*M_PI - ship->angle;
    p->owner = find_player(ship);
    add_projectile(p);
}


static void fire_boomerang(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p = make_boomerang(x,y,v);
    p->angle = 2*M_PI - ship->angle;
    p->src = ship;
    add_projectile(p);
}

static void toggle_cloak(struct Ship *ship, double x, double y, Vector v)
{
    if (ship->visible == 0) {
        ship->visible = 4;
        cloaking_device (ship, 0);
    } else if (ship->energy > 0) {
        ship->visible = 0;
        cloaking_device (ship, 1);
    }
}

static void drop_magmine(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p=make_magmine(ship->physics.x, ship->physics.y,
            makeVector(cos(ship->angle+M_PI)*2,
                -sin(ship->angle+M_PI)*2));

    set_fuse(p,0.63 * GAME_SPEED);
    add_projectile(p);
}

static void drop_mine(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p=make_mine(ship->physics.x, ship->physics.y,
            makeVector(0,0));

    set_fuse(p,0.63 * GAME_SPEED);
    add_projectile(p);
}

static void drop_divmine(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p=make_divmine(ship->physics.x, ship->physics.y,
            makeVector(cos(ship->angle+M_PI)*2,
                -sin(ship->angle+M_PI)*2));

    set_fuse(p,0.63 * GAME_SPEED);
    add_projectile(p);
}

static void toggle_shield(struct Ship *ship, double x, double y, Vector v)
{
    if (ship->shieldup) {
        ship_stop_special(ship);
    } else if (ship->energy > 0) {
        ship->shieldup = new_ga_link(&ship->physics.x,&ship->physics.y,
                ship->physics.radius,-600.0);
    }
}

static void toggle_ghostship(struct Ship *ship, double x, double y, Vector v)
{
    if (ship->physics.solidity == IMMATERIAL) {
        ghostify(ship,0);
    } else if (ship->energy > 0) {
        ghostify(ship,1);
    }
}

static void toggle_afterburner(struct Ship *ship, double x, double y, Vector v)
{
    ship->repeat_audio = 0;
    if (ship->afterburn) {
        ship->afterburn = 0;
    } else if (ship->energy > 0) {
        ship->afterburn = 1;
        spawn_clusters (ship->physics.x, ship->physics.y,5.6, 5, make_firestarter);
    }
}

static void drop_warp(struct Ship *ship, double x, double y, Vector v)
{
    drop_jumppoint(ship->physics.x, ship->physics.y, find_player(ship));
}

static void start_darting(struct Ship *ship, double x, double y, Vector v)
{
    ship->darting = DARTING;
    ship->physics.sharpness = BLUNT;
    ship->turn = 0;
    ship->fire_special_weapon = 0;
    ship->physics.vel.x += cos(ship->angle) * 30.0;
    ship->physics.vel.y -= sin(ship->angle) * 30.0;
}

static void toggle_autorepair(struct Ship *ship, double x, double y, Vector v)
{
    if (ship->repairing == 0 && ship->energy > 0 && ship->health < 1)
        ship->repairing = 1;
    else if (ship->repairing == 1)
        ship->repairing = 0;
    ship->anim = 0;
}

static void drop_soldier(struct Ship *ship, double x, double y, Vector v)
{
    add_critter (make_critter(OBJ_SOLDIER,ship->physics.x, ship->physics.y,
                find_player (ship)));
}

static void drop_helicopter(struct Ship *ship, double x, double y, Vector v)
{
    add_critter (make_critter(OBJ_HELICOPTER,ship->physics.x, ship->physics.y,
                find_player (ship)));
}

static void toggle_remote(struct Ship *ship, double x, double y, Vector v)
{
    if (ship->remote_control)
        remote_control(ship,0);
    else if(ship->energy>0)
        remote_control(ship,1);
}

static void toggle_antigrav(struct Ship *ship, double x, double y, Vector v)
{
    if (ship->antigrav)
        ship->antigrav = 0;
    else if (ship->energy > 0)
        ship->antigrav = 1;
}

static void zapper(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p;
    double tx,ty,d;
    struct Ship *target;

    if (ship->repeat_audio == 0) {
        playwave (WAV_ZAP);
        ship->repeat_audio = 9;
    } else
        ship->repeat_audio--;

    /* Find the nearest ship and attack that if it is close enough */
    target = find_nearest_ship(ship->physics.x,ship->physics.y, ship, &d);
    if(d<45.0) {
        tx = target->physics.x;
        ty = target->physics.y;
    } else {
        d = (rand()%628) / 100.0;
        tx = ship->physics.x + cos(d) * 25.0;
        ty = ship->physics.y + sin(d) * 25.0;
    }

    p = make_zap(tx,ty, ship->physics.vel);
    p->src = ship;

    add_projectile(p);
}

static void fire_watergun(struct Ship *ship, double x, double y, Vector v)
{
    static const double energy = 1.0/(3*GAME_SPEED);
    if(ship->physics.underwater) {
        if(ship->energy<1.0)
            ship->energy += energy;
    } else if(ship->energy>=energy) {
        double a = 0.3 - (rand()%6)/10.0;
        Vector mvel = get_muzzle_vel(ship->angle+a);
        add_projectile(make_waterjet(x,y,addVectors(mvel, ship->physics.vel)));
        ship->energy -= energy;
    }
}

static void fire_flamethrower(struct Ship *ship, double x, double y, Vector v)
{
    struct Projectile *p;
    double a = 0.3 - (rand()%6)/10.0;
    p=make_napalm(x,y,addVectors(multVector(get_muzzle_vel(ship->angle+a),0.5),
                ship->physics.vel));
    p->life = 0.9*GAME_SPEED;
    add_projectile(p);
}

static void fire_emp(struct Ship *ship, double x, double y, Vector v)
{
    int plr = find_player(ship);
    double a;

    for(a=0;a<2*M_PI;a+=0.1) {
        x = ship->physics.x + cos(a) * 6.0;
        y = ship->physics.y - sin(a) * 6.0;
        v = get_muzzle_vel(a);

        add_projectile(make_emwave(x,y,addVectors(v, ship->physics.vel)));
    }

    /* Firing the EMP causes the ship to temporarily lose power */
    ship->no_power = 2*GAME_SPEED;
    if(plr>=0)
        set_player_message(plr, Bigfont, font_color_red,
            ship->no_power, critical2str (CRITICAL_POWER));

}

static void fire_plastique(struct Ship *ship, double x, double y, Vector v) {
    /* Fire backwards */
    v = get_muzzle_vel(ship->angle + M_PI);
    add_projectile(make_plastique(ship->physics.x,ship->physics.y,
                addVectors(v, ship->physics.vel)));
}

static void drop_landmine(struct Ship *ship, double x, double y, Vector v) {
    /* Try detonating first */
    if(detonate_landmine(ship)==0) {
        struct Projectile *p;
        /* Fire backwards */
        v = get_muzzle_vel(ship->angle + M_PI);
        p = make_landmine(ship->physics.x,ship->physics.y,
                    addVectors(v, ship->physics.vel));
        p->src = ship;
        p->angle = ship->angle;
        add_projectile(p);
    }
}

static void fire_gravgun(struct Ship *ship, double x, double y, Vector v)
{
    Vector vel = {-cos(ship->angle) * 2.0, sin(ship->angle) * 2.0};
    add_projectile(make_gwell(ship->physics.x,ship->physics.y,vel));
}

static void drop_gravmine(struct Ship *ship, double x, double y, Vector v)
{
    add_projectile(make_gwell(ship->physics.x,ship->physics.y,makeVector(0,0)));
}

/* Normal weapon types */
const struct NormalWeapon normal_weapon[] = {
    {"Cannon", 0.1*GAME_SPEED, fire_cannon},
    {"Sweeping shot", 0.1*GAME_SPEED, fire_sweep},
    {"Tight triple shot", 0.33*GAME_SPEED, fire_ttriple},
    {"Wide triple shot", 0.33*GAME_SPEED, fire_wtriple},
    {"Quad shot", 0.23*GAME_SPEED, fire_quad},
};

/* Special weapon types */
const struct SpecialWeapon special_weapon[] = {
    {-1, "Grenade", 0.09, 0.3*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        make_grenade,NULL},
    {-1, "Rocket", 0.18, 0.33*GAME_SPEED, 0, 0, WAV_MISSILE,
        make_rocket, NULL},
    {-1, "MIRV", 0.28, 0.16*GAME_SPEED, 0, 0, WAV_MISSILE,
        make_mirv, NULL},
    {-1, "Homing missile", 0.18, 0.36*GAME_SPEED, 0, 0, WAV_MISSILE,
        0,fire_missile},
    {-1, "Boomerang bomb", 0.06, 0.2*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        NULL, fire_boomerang},
    {-1, "Megabomb", 0.25, 0.16*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        0,drop_megabomb},
    {-1, "Ember", 0.15, 0.33*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        NULL, drop_ember},
    {-1, "Mush", 0.09, 0.53*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        make_mush, NULL},
    {-1, "Afterburner", 0, 0, 0, 1, WAV_NONE,
        0, toggle_afterburner},
    {-1, "Jump engine", 0.10, 0.33*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        0, drop_warp},
    {WEAP_AUTOREPAIR, "Autorepair system", 0, 0, 0, 1, WAV_NONE,
        NULL,toggle_autorepair},
    {-1, "Gravity polarizer", 0, 0, 0, 1, WAV_NONE,
        NULL, toggle_antigrav},
    {-1, "Cloaking device", 0, 0, 0, 1, WAV_NONE,
        0,toggle_cloak},
    {-1, "Ghost ship", 0, 0, 0, 1, WAV_NONE,
        0, toggle_ghostship},
    {-1, "Shield", 0, 0, 0, 1, WAV_NONE,
        0, toggle_shield},
    {-1, "Grav-gun", 0.24, 0.5*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        NULL,fire_gravgun},
    {-1, "Gravity mine", 0.30, 0.8*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        NULL,drop_gravmine},
    {-1, "Landmine", 0.05, 0.16*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        NULL,drop_landmine},
    {-1, "Magnetic mine", 0.06, 0.3*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        0,drop_magmine},
    {-1, "Mine", 0.06, 0.3*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        0,drop_mine},
    {-1, "Dividing mine", 0.33, 0.33*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        NULL, drop_divmine},
    {-1, "Shotgun", 0.11, 0.5*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        0,fire_shotgun},
    {-1, "Speargun", 0.08, 0.2*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        make_spear, NULL},
    {-1, "Microwave cannon", 0.01, 0.13*GAME_SPEED, 0, 0, WAV_LASER,
        make_mwave, NULL},
    {-1, "Watergun", 0.0, 0.0, 0, 0, WAV_NONE,
        NULL, fire_watergun},
    {-1, "Flamethrower", 1/(5.3*GAME_SPEED), 0, 1, 0, WAV_NONE,
        NULL, fire_flamethrower},
    {-1, "Claybomb", 0.125, 0.16*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        make_claybomb,NULL},
    {-1, "Plastic explosive", 0.125, 0.16*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        NULL,fire_plastique},
    {-1, "Snowball", 0.09, 0.23*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        make_snowball,NULL},
    {-1, "Acid", 0.1, 0.33*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        make_acid, NULL},
    {-1, "Dart", 0.1, 0.66*GAME_SPEED, 0, 0, WAV_DART,
        NULL,start_darting},
    {-1, "Infantry", 0.05, 0.16*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        NULL, drop_soldier},
    {-1, "Helicopters", 0.25, 0.5*GAME_SPEED, 1, 0, WAV_SPECIALWEAP,
        NULL, drop_helicopter},
    {-1, "Thunderbolt", 1.0/(2*GAME_SPEED), 0, 0, 0, WAV_NONE,
        NULL, zapper}, /* Deplete energy in 2s */
    {-1, "Remote control", 0, 0, 0, 1, WAV_NONE,
        NULL, toggle_remote},
    {-1, "EMP", 0.3, 1*GAME_SPEED, 0, 0, WAV_SPECIALWEAP,
        NULL, fire_emp},
};

int normal_weapon_count(void) {
    return sizeof(normal_weapon)/sizeof(struct NormalWeapon);
}

int special_weapon_count(void) {
    return sizeof(special_weapon)/sizeof(struct SpecialWeapon);
}

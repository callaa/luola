/*
* Luola - 2D multiplayer cave-flying game
* Copyright (C) 2003-2006 Calle Laakkonen
*
* File        : bullet.c
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "console.h"
#include "particle.h"
#include "decor.h"
#include "player.h"
#include "bullet.h"
#include "level.h"
#include "game.h"
#include "ship.h"
#include "audio.h"
#include "defines.h" /* For Round() */

#define DIVIDINGMINE_INTERVAL (7*GAME_SPEED)
#define DIVIDINGMINE_RAND (2*GAME_SPEED)

/* Draw a simple one dot projectile */
static void draw_simple_projectile(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    putpixel (screen, x + viewport.x, y + viewport.y, p->color);
}

static void draw_big_projectile(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    putpixel (screen, x + viewport.x, y + viewport.y, p->color);
    if (x + 1 < viewport.w) {
        putpixel (screen, x + viewport.x + 1, y + viewport.y, p->color);
    }
    if (x + - 1 >= 0) {
        putpixel (screen, x + viewport.x - 1, y + viewport.y, p->color);
    }
    if (y + 1 < viewport.h) {
        putpixel (screen, x + viewport.x, y + viewport.y + 1, p->color);
    }
    if (y - 1 >= 0) {
        putpixel (screen, x + viewport.x, y + viewport.y - 1, p->color);
    }
}

/* Draw a round projectile in proper scale. */
static void draw_round_projectile(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    int r = Round(p->physics.radius)/2;
    int i,j;
    if(r<1) r=1;
    for(i=-r;i<0;i++)
        for(j=r+i;j>-r-i;j--)
            putpixel(screen, j+x+viewport.x, i+y+viewport.y, p->color);
    for(i=0;i<r;i++)
        for(j=r-i;j>-r+i;j--)
            putpixel(screen, j+x+viewport.x, i+y+viewport.y, p->color);
}

/* Draw a mega bomb */
static void draw_megabomb(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    x+=viewport.x;
    y+=viewport.y;
    putpixel (screen, x, y, p->color);
    putpixel (screen, x, y - 1, p->color);
    putpixel (screen, x, y + 1, p->color);
    putpixel (screen, x - 1, y - 1, p->color);
    putpixel (screen, x + 1, y - 1, p->color);
}

/* Draw a missile */
static void draw_missile(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    int dx, dy;
    dx = cos (p->angle) * 3.0;
    dy = sin (p->angle) * 3.0;
    draw_line (screen, x + viewport.x - dx, y + viewport.y - dy,
            x + viewport.x + dx, y + viewport.y + dy, p->color);
}

/* Draw a thunderbolt */
static void draw_zap(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    int targx, targy, dx, dy, seg;
    targx = x + Round(p->src->physics.x - p->physics.x);
    targy = y + Round(p->src->physics.y - p->physics.y);
    for (seg = 0; seg < 4; seg++) {
        dx = (targx - x + ((rand () % 20) - 10)) / 3;
        dy = (targy - y + ((rand () % 20) - 10)) / 3;
        if (x > 0 && x < viewport.w && y > 0 && y < viewport.h)
            if (x + dx > 0 && x + dx < viewport.w && y + dy > 0
                && y + dy < viewport.h)
                draw_line (screen, x+viewport.x, y+viewport.y,
                        x + viewport.x + dx,
                        y + viewport.y + dy, col_yellow);
        x += dx;
        y += dy;
    }
}

/* Draw a vortex */
static void draw_vortex(struct Projectile *p,int x,int y, SDL_Rect viewport) {
    int i;
    if(p->var>8) p->var=0;
    else p->var++;
    for(i=0;i<12;i++) {
        double d = -M_PI_4 * i + p->var / 4.0;
        int j;
        for(j=4;j<=16;j++) {
            putpixel(screen, x+viewport.x + cos(d-j/6.0) * j,
                    y + viewport.y + sin(d-j/6.0) * j, p->color);
        }
    }
}

/* Spawn a cluster of projectiles */
void spawn_clusters (double x, double y, double f,int count,
    struct Projectile *(*make_projectile)(double x, double y, Vector v))
{
    double angle,incr = (2.0 * M_PI) / count;
    for(angle=0;angle<2.0*M_PI;angle+=incr) {
        struct Projectile *p;
        double r = (rand () % 5) / 5.0;
        double dx = sin(angle + r);
        double dy = cos(angle + r);
        p = make_projectile (x + dx*2, y + dy*2,
                    makeVector (dx * f, dy * f));
        p->prime=1;
        add_projectile(p);

    }
}


/* A basic explosion method for projectiles */
static void projectile_std_explode(struct Projectile *p) {
    add_explosion(Round(p->physics.x),Round(p->physics.y));
}

/* Burn a grey patch into ground */
static void fry_ground(struct Projectile *p) {
    burn_hole(Round(p->physics.x),Round(p->physics.y));
}

/* Firestarter explosion method */
static void start_fire(struct Projectile *p) {
    if(p->physics.hitground==TER_COMBUSTABLE ||
            p->physics.hitground==TER_COMBUSTABL2 ||
            p->physics.hitground==TER_EXPLOSIVE ||
            p->physics.hitground==TER_EXPLOSIVE2) {
        start_burning(Round(p->physics.x), Round(p->physics.y));
        p->life=0;
    }
}

/* Explode into cluster of bullets */
static void projectile_explode_cluster1(struct Projectile *p) {
    add_explosion(Round(p->physics.x),Round(p->physics.y));
    spawn_clusters(p->physics.x,p->physics.y, 5.6, 16, make_bullet);
    spawn_clusters (p->physics.x, p->physics.y, 5.6, 16, make_firestarter);
}

/* Explode into cluster of grenades */
static void projectile_explode_cluster2(struct Projectile *p) {
    add_explosion(Round(p->physics.x),Round(p->physics.y));
    spawn_clusters(p->physics.x,p->physics.y,5.6, 8, make_grenade);
    spawn_clusters (p->physics.x, p->physics.y,5.6, 16, make_firestarter);
}

/* Explode into a ball of fire */
static void projectile_explode_fireball(struct Projectile *p) {
    add_explosion(Round(p->physics.x),Round(p->physics.y));
    spawn_clusters(p->physics.x,p->physics.y,5.6, 8, make_napalm);
    spawn_clusters (p->physics.x, p->physics.y,5.6, 16, make_firestarter);
}

/* Claybomb explosion: Generate a glob of land */
static void claybomb_explode(struct Projectile *p) {
    alter_level (Round(p->physics.x), Round(p->physics.y), 15, Earth);
    playwave_3d (WAV_SWOOSH, p->physics.x, p->physics.y);
}

/* Plastique explosion: Turn terrain into plastic explosive */
static void plastique_explode(struct Projectile *p) {
    alter_level (Round(p->physics.x), Round(p->physics.y), 14, Explosive);
    playwave_3d (WAV_SWOOSH, p->physics.x, p->physics.y);
}

/* Snowball explosion: Add a frost coating to terrain */
static void snowball_explode(struct Projectile *p) {
    alter_level (Round(p->physics.x), Round(p->physics.y), 15, Ice);
    playwave_3d (WAV_FREEZE, p->physics.x, p->physics.y);
}

/* Snowball freezes ships */
static int snowball_hitship(struct Projectile *p, struct Ship *ship) {
    freeze_ship(ship);
    playwave (WAV_FREEZE);
    return 1;
}

/* Spear explosion: Leave a mark in ground */
static void spear_explode(struct Projectile *p) {
    int dx, dy;
    dx = cos (p->angle) * 6;
    dy = sin (p->angle) * 6;
    draw_line (lev_level.terrain, Round(p->physics.x) - dx,
               Round(p->physics.y) - dy, Round(p->physics.x) + dx,
               Round(p->physics.y) + dy, p->color);
}

/* Spear a ship */
static int spear_hitship(struct Projectile *p, struct Ship *ship) {
    damage_ship(ship,p->damage,0.1);
    spear_ship(ship,p);
    return 1;
}

/* Acid eats away ground */
static void acid_explode(struct Projectile *p) {
    start_melting (Round(p->physics.x), Round(p->physics.y), 10);
    playwave_3d (WAV_STEAM, p->physics.x, p->physics.y);
}

/* Adhere to ship */
static int sticky_hitship(struct Projectile *p,struct Ship *ship) {
    /* TODO */
#if 0
    p->physics.x = ship->physics.x;
    p->physics.y = ship->physics.y;
#endif
    damage_ship(ship,p->damage,0.001);
    return 0;
}

/* Basic ship collision method */
static int projectile_std_hitship(struct Projectile *p, struct Ship *ship) {
    damage_ship(ship,p->damage,p->critical);
    if(p->explode)
        p->explode(p);
    return 1;
}

/* Boomerang ship collision. If hits owner, is absorbed */
static int boomerang_hitship(struct Projectile *p, struct Ship *ship) {
    if(ship == p->src) {
        ship->energy += special_weapon[ship->special].energy;
    } else {
        projectile_std_hitship(p,ship);
    }
    return 1;
}

/* Homing missile movement logic */
static void missile_move(struct Projectile *p) {
    static const double TURNSPEED = 0.2;
    double d,a;
    int plr;
    plr = find_nearest_enemy (p->physics.x, p->physics.y, p->owner, 0);
    if (plr < 0) {
        /* No visible enemies around, shut down engines */
        p->physics.thrust.x = 0;
        p->physics.thrust.y = 0;
        return;
    }
    /* Get target */
    a = atan2 ( players[plr].ship->physics.y - p->physics.y,
            players[plr].ship->physics.x - p->physics.x);
    /* TODO: Better AI */

    /* Turn towards target */
    if(a<0) a = 2*M_PI + a;
    d = shortestRotation(p->angle,a);

    if(d<-0.01)
        p->angle -= TURNSPEED;
    else if(d>0.01)
        p->angle += TURNSPEED;

    if(p->angle<0) p->angle = 2*M_PI+p->angle;
    else if(p->angle>2*M_PI) p->angle-= 2*M_PI;

    /* Accelerate if on course */
    if(fabs(d) < 0.2) {
        p->physics.thrust.x = cos(p->angle) * 0.5;
        p->physics.thrust.y = sin(p->angle) * 0.5;
        make_particle (p->physics.x-cos(p->angle)*3,
                p->physics.y-sin(p->angle)*3, 12);
    }
}

/* Boomerang movement logic */
/* TODO share more code with homing missile */
static void boomerang_move(struct Projectile *p) {
    static const double TURNSPEED = 0.2;
    struct dllist *ships = ship_list;
    double d,a;
    /* Check that the originating ship still exists */
    while(ships) {
        if(ships->data == p->src) break;
        ships = ships->next;
    }
    if(!ships) {
        /* Ship destroyed */
        p->move = NULL;
        return;
    }
    /* Get target */
    a = atan2 ( p->src->physics.y - p->physics.y,
            p->src->physics.x - p->physics.x);

    /* Turn towards target */
    if(a<0) a = 2*M_PI + a;
    d = shortestRotation(p->angle,a);

    if(d<-0.01)
        p->angle -= TURNSPEED;
    else if(d>0.01)
        p->angle += TURNSPEED;

    if(p->angle<0) p->angle = 2*M_PI+p->angle;
    else if(p->angle>2*M_PI) p->angle-= 2*M_PI;

    /* Accelerate if on course */
    if(fabs(d) < 0.2) {
        p->physics.thrust.x = cos(p->angle) * 0.5;
        p->physics.thrust.y = sin(p->angle) * 0.5;
    }
}

/* Update angle based on velocity */
static void update_angle(struct Projectile *p) {
    p->angle = atan2(p->physics.vel.y,p->physics.vel.x);
}

/* Rocket animation */
static void rocket_move(struct Projectile *p) {
    p->angle = atan2(p->physics.vel.y,p->physics.vel.x);
    make_particle (p->physics.x-cos(p->angle)*3,
            p->physics.y-sin(p->angle)*3, 12);
}

/* Magnetic mine movement */
/* TODO: improve */
static void magmine_move(struct Projectile *p) {
    static const double RANGE = 90.0;
    if(p->prime<0) {
        struct Ship *ship;
        double d;
        ship = find_nearest_ship(p->physics.x, p->physics.y, NULL, &d);
        if(ship && d < RANGE) {
            Vector f;
            f.x = ship->physics.x - p->physics.x;
            f.y = ship->physics.y - p->physics.y;
            p->physics.vel.x += f.x/RANGE;
            p->physics.vel.y += f.y/RANGE;
        }
    }
}

/* Steaming */
static void projectile_steam(struct Projectile *p) {
    struct Particle *part = make_particle(p->physics.x,p->physics.y, 15);
    part->color[0] = 255;
    part->color[1] = 255;
    part->color[2] = 255;
    part->ad = -17;
    part->rd = 0;
    part->gd = 0;
    part->bd = 0;
    part->vector.x = (rand () % 4 - 2) / 2.0 - weather_wind_vector;
    part->vector.y = -2;
}

/* Napalm burning animation */
static void napalm_burn(struct Projectile *p) {
    start_burning(Round(p->physics.x),Round(p->physics.y));
}

/* Ember burning animation */
static void ember_burn(struct Projectile *p) {
    int r;
    p->color = burncolor[rand() % FIRE_FRAMES];
    for(r=0;r<2+p->var;r++) {
        int dx = 3 - rand()%6;
        int dy = 3 - rand()%6;
        start_burning(p->physics.x+dx,p->physics.y+dy);
    }
}

/* Boomerang enters return mode */
static void boomerang_return(struct Projectile *boomerang) {
    boomerang->move = boomerang_move;
}

/* Ember split */
static void ember_split(struct Projectile *ember) {
    int split;
    ember->var--;
    for(split=-2;split<3;split++) {
        struct Projectile *p = malloc(sizeof(struct Projectile));
        memcpy(p,ember,sizeof(struct Projectile));

        p->physics.vel.x -= split*2 - (rand()%20)/10.0;
        if(ember->var>0) {
            p->timer = 2.1 * GAME_SPEED;
            p->timerfunc = ember_split;
        } else {
            p->draw = draw_simple_projectile;
        }
        add_projectile(p);
    }
    ember->life = 0;
}

/* MIRV split */
static void mirv_split(struct Projectile *mirv) {
    int split;
    mirv->var--;
    if(mirv->var>0) {
        mirv->timer = 0.6 * GAME_SPEED;
        mirv->timerfunc = mirv_split;
    }
    for(split=-1;split<2;split+=2) {
        struct Projectile *p = malloc(sizeof(struct Projectile));
        memcpy(p,mirv,sizeof(struct Projectile));

        p->explode = projectile_explode_cluster1;
        rotateVector(&p->physics.thrust,split*M_PI/16);
        rotateVector(&p->physics.vel,split*M_PI/16);
        add_projectile(p);
    }
}

/* Dividing mine explosion */
static void dividing_mine_explode(struct Projectile *p) {
    int clusters = p->physics.radius * 2;

    if(clusters<1) clusters=1;
    add_explosion(Round(p->physics.x),Round(p->physics.y));
    spawn_clusters(p->physics.x,p->physics.y,5.6, clusters, make_bullet);
    spawn_clusters (p->physics.x, p->physics.y,5.6, 8, make_firestarter);
}

/* Randomize a color component */
static Uint8 rand_col(Uint8 c) {
    int tmp = c;
    tmp += 30 - rand()%60;
    if(tmp<0) tmp=0;
    else if(tmp>255) tmp=255;
    return tmp;
}

/* Dividing mine division */
/* Note. some variables from Projectile are heavily reused here */
/* for other purposes than what they were intended for:
 * angle -> separation force
 * var -> division interval
 * ownerteam -> crowdedness treshold
 */
static void mine_divide(struct Projectile *mine) {
    struct Projectile *newmine = malloc(sizeof(struct Projectile));
    double splitangle = rand()%628/100.0;
    struct dllist *lst;
    int crowd=0;
    Vector v;

    /* How many other dividing mines there are nearby? */
    lst = projectile_list;
    while(lst) {
        struct Projectile *p = lst->data;
        if(p!=mine && p->timerfunc == mine_divide
                && hypot(p->physics.x-mine->physics.x,
                    p->physics.y-mine->physics.y) < 14 * mine->physics.radius)
            crowd++;
        lst=lst->next;
    }

    /* If too crowded, don't divide */
    if(crowd > mine->owner) {
        mine->timer = mine->var;
        return;
    }
    
    /* Randomize division interval */
    if(rand()%6==0) {
        mine->var += DIVIDINGMINE_RAND/2 - rand()% DIVIDINGMINE_RAND;
        if(mine->var<DIVIDINGMINE_RAND/2) mine->var=DIVIDINGMINE_RAND/2;
    }

    /* Randomize separation force */
    if(rand()%7==0) {
        mine->angle += 0.6 - (rand()%12)/10.0;
    }

    /* Randomize crowdedness treshold */
    if(rand()%15==0) {
        mine->owner += 3 - rand()%6;
        if(mine->owner<1) mine->owner=1;
    }

    /* Randomize radius */
    if(rand()%20==0) {
        mine->physics.radius += 1.0 - (rand()%20)/10.0;
        if(mine->physics.radius<0.2) mine->physics.radius = 0.2;
        mine->physics.mass = get_floating_mass(mine->physics.radius);
    }
    /* Randomize color */
    if(rand()%6==0) {
        Uint8 r,g,b,a;
        unmap_rgba(mine->color,&r,&g,&b,&a);
        r = rand_col(r);
        g = rand_col(g);
        b = rand_col(b);
        mine->color = map_rgba(r,g,b,a);
    }

    /* Split force */
    v.x = cos(splitangle) * mine->angle;
    v.y = sin(splitangle) * mine->angle;

    mine->timer = mine->var;
    set_fuse(mine,4); /* Become insensitive for a while so mines can separate */

    /* Split */
    memcpy(newmine,mine,sizeof(struct Projectile));
    
    mine->physics.vel.x += v.x;
    mine->physics.vel.y += v.y;
    newmine->physics.vel.x -= v.x;
    newmine->physics.vel.y -= v.y;

    add_projectile(newmine);
}

/* Landmine explosion */
static void landmine_explode(struct Projectile *mine) {
    double a;
    for(a=-0.25;a<0.25;a+=0.08) {
        add_projectile(make_bullet(mine->physics.x,
                    mine->physics.y,
                    get_muzzle_vel(mine->angle+a)));
    }
    mine->life = 0;
}

/* Landmine gets stuck in terrain */
static void landmine_getstuck(struct Projectile *mine) {
    mine->draw = NULL;
    mine->physics.vel = makeVector(0,0);
    mine->timerfunc = landmine_explode;
}

/* Detonate landmine */
int detonate_landmine(struct Ship *owner) {
    struct dllist *ptr = projectile_list;
    while(ptr) {
        struct Projectile *p=ptr->data;
        if(p->src == owner && p->timerfunc==landmine_explode) {
            p->timer=1;
            return 1;
        }
        ptr=ptr->next;
    }
    return 0;
}

/* Drop a grenade */
static void drop_grenade(struct Projectile *mush) {
    add_projectile(make_grenade(mush->physics.x,mush->physics.y,makeVector(0,0)));
    mush->timer = 0.3*GAME_SPEED;
}

/* Microwave beam trail */
static void mwave_trail(struct Projectile *p) {
    struct Particle *part;
    int r;
    double dx = p->physics.vel.x/3.0;
    double dy = p->physics.vel.y/3.0;
    for (r = 0; r < 3; r++) {
        part =
            make_particle (p->physics.x - dx*r,
                           p->physics.y - dy*r, 15);
        part->color[0] = 255;
        part->color[1] = 255;
        part->color[2] = 0;
#ifndef HAVE_LIBSDL_GFX
        part->rd = part->gd = -4;
        part->bd = 0;
#else
        part->rd = part->gd = part->bd = 0;
#endif
        part->vector.x = 0;
        part->vector.y = 0;
    }
}

/* EM pulse trail */
static void emp_trail(struct Projectile *p) {
    struct Particle *part;
    part = make_particle (p->physics.x, p->physics.y, 5);
    part->color[0] = 0;
    part->color[1] = 0;
    part->color[2] = 255;
#ifndef HAVE_LIBSDL_GFX
    part->rd = part->gd = 0;
    part->bd = -51;
#else
    part->rd = part->gd = part->bd = 0;
#endif
    part->vector = multVector(p->physics.vel,0.7);
}

/* Deactivate a gravity weapon */
static void destroy_gwell(struct Projectile *gwell) {
    remove_ga((struct GravityAnomaly*)gwell->src);
}

/* Activate a gravity weapon */
static void activate_gwell(struct Projectile *gwell) {
    gwell->draw = draw_vortex;
    gwell->src = (void*)new_ga_link(&gwell->physics.x, &gwell->physics.y, 1,
            15000.0);
    gwell->destroy = destroy_gwell;
}

/* Make a bare bones bullet */
/* Remember to set these yourself:
* color
* damage
* critical
*/
static struct Projectile *make_base(double x,double y, Vector v) {
    struct Projectile *p=malloc(sizeof(struct Projectile));
    if(!p) {
        perror("make_base");
        return NULL;
    }

    init_physobj(&p->physics, x,y,v);
    p->physics.mass = 2.0;

    p->cloak = 0;
    p->life = -1;
    p->timer = -1;
    p->prime = -1;

    p->otherobj = 0;
    p->hydrophobic = 0;
    p->critter = 1;

    if (game_settings.large_bullets)
        p->draw = draw_big_projectile;
    else
        p->draw = draw_simple_projectile;
    p->move = NULL;
    p->explode = projectile_std_explode;
    p->hitship = projectile_std_hitship;
    p->destroy = NULL;

    return p;
}

/* Create a firestarter */
/* Firestarters are invisible projectiles that go through solid ground. */
/* They affect nothing but combustable and explosive terrain */
struct Projectile *make_firestarter(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->physics.solidity = WRAITH;
    p->physics.obj = 0;
    p->life = 0.8*GAME_SPEED;
    p->critter = 0;

    p->explode = start_fire;
    p->draw = NULL;
    p->hitship = NULL;
    return p;
}

/* Create a simple bullet */
struct Projectile *make_bullet(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.03;
    p->critical = 0.03;
    p->color = col_default;

    return p;
}

/* Create a grenade */
struct Projectile *make_grenade(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.0;
    p->critical = 0.1;
    p->color = col_gray;
    p->explode = projectile_explode_cluster1;

    return p;
}

/* Create a megabomb */
struct Projectile *make_megabomb(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.03;
    p->critical = 0.2;
    p->physics.radius = 4.0;
    p->physics.mass = 10.0;
    p->color = col_default;
    p->draw = draw_megabomb;
    p->explode = projectile_explode_cluster2;

    return p;
}

/* Create a missile */
struct Projectile *make_missile(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.02;
    p->critical = 0.10;
    p->physics.radius = 6.0;
    p->physics.mass = 5.0;
    p->color = col_gray;
    p->owner = -1;
    p->draw = draw_missile;
    p->move = missile_move;
    p->explode = projectile_explode_cluster1;

    return p;
}

/* Create a mine */
struct Projectile *make_mine(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.01;
    p->critical = 0.1;
    p->otherobj = 1;
    p->physics.radius = 8;
    p->physics.mass = get_floating_mass(p->physics.radius);
    p->color = col_gray;
    p->cloak = 1;
    p->draw = draw_big_projectile;
    p->explode = projectile_explode_cluster1;

    return p;
}

/* Create a magnetic mine */
struct Projectile *make_magmine(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.01;
    p->critical = 0.1;
    p->otherobj = 1;
    p->physics.radius = 8;
    p->physics.mass = get_floating_mass(p->physics.radius);
    p->color = map_rgba(149,158,255,255);
    p->move = magmine_move;
    p->draw = draw_big_projectile;
    p->explode = projectile_explode_cluster1;

    return p;
}

/* Create a dividing mine */
struct Projectile *make_divmine(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.01;
    p->critical = 0.05;
    p->otherobj = 1;
    p->physics.radius = 4;
    p->physics.mass = get_floating_mass(p->physics.radius);
    p->color = col_green;
    p->draw = draw_round_projectile;
    p->explode = dividing_mine_explode;

    p->var = DIVIDINGMINE_INTERVAL;
    p->angle = 1.4; /* separation force */
    p->owner = 6;   /* crowdedness threshold */

    p->timer = p->var;
    p->timerfunc = mine_divide;

    return p;
}

/* Create a claybomb */
struct Projectile *make_claybomb(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.0;
    p->critical = 0.0;
    p->color = col_clay;
    p->explode = claybomb_explode;

    return p;
}

/* Create a snowball */
struct Projectile *make_snowball(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.0;
    p->critical = 0.0;
    p->hydrophobic = 1;
    p->color = col_snow;
    p->explode = snowball_explode;
    p->hitship = snowball_hitship;

    return p;
}

/* Create a glob of plastic explosive */
struct Projectile *make_plastique(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.0;
    p->critical = 0.0;
    p->physics.obj = 0;
    p->hydrophobic = 1;
    p->life = 0.16 * GAME_SPEED;
    p->draw = NULL;
    p->explode = plastique_explode;

    return p;
}

/* Create a spear */
struct Projectile *make_spear(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.0;
    p->critical = 0.2;
    p->color = col_gray;
    p->draw = draw_missile;
    p->explode = spear_explode;
    p->hitship = spear_hitship;
    p->move = update_angle;

    return p;
}

/* Create an acid blob */
struct Projectile *make_acid(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.002;
    p->critical = 0.0005;
    p->color = col_green;
    p->hydrophobic = 1;
    p->draw = draw_big_projectile;
    p->explode = acid_explode;
    p->hitship = sticky_hitship;
    p->move = projectile_steam;

    return p;
}

/* Create thunderbolt */
/* Remember to set p->src! */
struct Projectile *make_zap(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.9 / (2*GAME_SPEED);
    p->critical = 0.001;
    p->life = 3;
    p->draw = draw_zap;
    p->explode = fry_ground;

    return p;
}

/* Create a waterjet */
struct Projectile *make_waterjet(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0;
    p->critical = 0.0;
    p->life = 40;
    p->color = lev_watercol;
    p->explode = NULL;
    p->hitship = NULL;

    return p;
}

/* Create a rocket */
struct Projectile *make_rocket(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.03;
    p->critical = 0.15;
    p->physics.radius = 7.0;
    p->physics.mass = 8.0;
    /* Target speed is a bit less than initial */
    p->physics.thrust = get_constant_vel(multVector(p->physics.vel,0.5),
            p->physics.radius, p->physics.mass);
    p->color = col_gray;
    p->draw = draw_missile;
    p->move = rocket_move;
    p->explode = projectile_explode_cluster2;

    return p;
}
/* Create a MIRV */
struct Projectile *make_mirv(double x,double y, Vector v) {
    struct Projectile *p = make_rocket(x,y,v);

    p->timer = 0.6*GAME_SPEED;
    p->var = 3;
    p->timerfunc = mirv_split;

    return p;
}


/* Create a mush bullet */
struct Projectile *make_mush(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.03;
    p->critical = 0.1;
    p->physics.radius = 2.0;
    p->physics.mass = 2.0;
    p->color = col_yellow;

    p->timer = 0.3*GAME_SPEED;
    p->timerfunc = drop_grenade;

    return p;
}

/* Create a boomerang */
struct Projectile *make_boomerang(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.03;
    p->critical = 0.05;
    p->physics.radius = 5.0;
    p->physics.mass = 4.0;
    p->color = col_yellow;
    p->draw = draw_big_projectile;
    p->timer = 1*GAME_SPEED;
    p->timerfunc = boomerang_return;
    p->explode = projectile_explode_cluster1;
    p->hitship = boomerang_hitship;

    return p;
}

/* Create a firebomb */
struct Projectile *make_ember(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->damage = 0.03;
    p->critical = 0.05;
    p->physics.radius = 4.0;
    p->physics.mass = 2.5;
    p->physics.sharpness = BLUNT;
    p->hydrophobic = 1;

    p->move = ember_burn;
    p->draw = draw_big_projectile;

    /* Explode to napalm cluster */
    p->explode = projectile_explode_fireball;
    p->timer = 1.1 * GAME_SPEED;
    p->timerfunc = ember_split;
    p->var=2;

    return p;
}

/* Make a glob of napalm */
struct Projectile *make_napalm(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);

    p->damage = 0.0005;
    p->critical = 0.0001;
    p->hydrophobic = 1;
    p->color = col_white;

    p->move = napalm_burn;
    p->hitship = sticky_hitship;
    p->explode = NULL;

    return p;
}

/* Make a microwave beam */
struct Projectile *make_mwave(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->physics.solidity = IMMATERIAL;
    p->physics.radius = 0.5;
    p->physics.mass = get_floating_mass(p->physics.radius);
    p->physics.thrust = get_constant_vel(p->physics.vel,p->physics.radius,
            p->physics.mass);

    p->damage = 0.02;
    p->critical = 0.01;
    p->critter = 0;

    p->color = col_white;
    p->explode = NULL;
    p->move = mwave_trail;

    return p;
}

/* Make an electromagnetic wave */
struct Projectile *make_emwave(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->physics.solidity = IMMATERIAL;
    p->physics.radius = 0.5;
    p->physics.mass = get_floating_mass(p->physics.radius);
    p->physics.thrust = get_constant_vel(p->physics.vel,p->physics.radius,
            p->physics.mass);

    p->damage = 0.0;
    p->critical = 0.6;
    p->life = 0.43*GAME_SPEED;
    p->critter = 0;

    p->color = col_blue;
    p->explode = NULL;
    p->move = emp_trail;

    return p;
}

/* Make a gravity well */
struct Projectile *make_gwell(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->physics.solidity = IMMATERIAL;
    p->physics.obj = 0;

    p->physics.mass = get_floating_mass(p->physics.radius);
    p->physics.thrust = get_constant_vel(p->physics.vel,p->physics.radius,
            p->physics.mass);

    p->color = col_gray;
    p->explode = NULL;
    p->hitship = NULL;
    p->var = 0;
    p->life = 61*GAME_SPEED;
    p->src = NULL; /* We reuse the src variable to hold the GA */
    p->draw = draw_big_projectile;
    
    p->timer = 0.9*GAME_SPEED;
    p->timerfunc = activate_gwell;

    return p;
}

/* Make a landmine */
/* Remember to set p->src and p->angle! */
struct Projectile *make_landmine(double x,double y, Vector v) {
    struct Projectile *p = make_base(x,y,v);
    p->physics.obj = 0;
    p->physics.solidity = WRAITH;

    p->damage = 0.0;
    p->critical = 0.0;
    p->color = col_gray;
    p->draw = draw_big_projectile;
    p->explode = landmine_getstuck;
    p->timer=-1;

    return p;
}


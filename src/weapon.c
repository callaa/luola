/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : weapon.c
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fs.h"
#include "console.h"
#include "level.h"
#include "weapon.h"
#include "sweapon.h"
#include "player.h"
#include "particle.h"
#include "game.h"
#include "special.h"
#include "critter.h"
#include "weather.h"
#include "ship.h"

#include "audio.h"

#define HOLE_W 9
#define HOLE_H 9

static Uint8 wea_gnaw[HOLE_H][HOLE_W] = {
    {1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1}
};

/* Number of frames in explosion animation */
#define EXPL_FRAMES     7
/* How soon an explosion sends out shrapnel */
#define EXPLOSION_CLUSTER_SPEED 5

/* Explosion */
struct Explosion {
    int x;
    int y;
    int frame;
    ProjectileType cluster;
};

/* Internally used globals */
static struct dllist *projectiles;
static struct dllist *last_proj;
static struct dllist *explosions;

static Vector wea_gravity;
static SDL_Surface *wea_explosion[EXPL_FRAMES];
static int gravity_weapon_count;        /* How many gravity weapons there are */

const char *weap2str (int weapon)
{
    switch (weapon) {
    case WCannon:
        return "Cannon";     /* This is the ordinary weapon, it should newer get here */
    case WGrenade:
        return "Grenade";
    case WMegaBomb:
        return "Mega bomb";
    case WMissile:
        return "Homing missile";
    case WCloak:
        return "Cloaking device";
    case WMagMine:
        return "Magnetic mine";
    case WMine:
        return "Mine";
    case WShield:
        return "Shield";
    case WGhostShip:
        return "Ghost Ship";
    case WAfterBurner:
        return "After Burner";
    case WWarp:
        return "Jump engine";
    case WClaybomb:
        return "Claybomb";
    case WPlastique:
        return "Plastic explosive";
    case WSnowball:
        return "Snowball";
    case WDart:
        return "Dart";
    case WLandmine:
        return "Landmine";
    case WRepair:
        return "Autorepair system";
    case WInfantry:
        return "Infantry";
    case WHelicopter:
        return "Helicopter";
    case WSpeargun:
        return "Speargun";
    case WGravGun:
        return "Grav-gun";
    case WGravMine:
        return "Gravity mine";
    case WZapper:
        return "Thunderbolt";
    case WShotgun:
        return "Shotgun";
    case WRocket:
        return "Rocket";
    case WEnergy:
        return "Microwave cannon";
    case WBoomerang:
        return "Boomerang bomb";
    case WRemote:
        return "Remote control";
    case WDivide:
        return "Dividing mine";
    case WTag:
        return "Tag-gun";
    case WMush:
        return "Mush";
    case WWatergun:
        return "Watergun";
    case WEmber:
        return "Ember";
    case WAcid:
        return "Acid";
    case WMirv:
        return "MIRV";
    case WFlame:
        return "Flamethrower";
    case WEMP:
        return "EMP";
    case WAntigrav:
        return "Gravity coil";
    }
    return "foo";
}

const char *sweap2str (int weapon)
{
    switch (weapon) {
    case SShot:
        return "Cannon";
    case S3ShotWide:
        return "Wide triple shot";
    case S3ShotTight:
        return "Tight triple shot";
    case SSweep:
        return "Sweeping shots";
    case S4Way:
        return "4 Way";
    }
    return "foo";
}

/* Can this type of a weapon collide with a ship ? */
static char can_collide_with_ship (ProjectileType type)
{
    if (type == GravityWell)
        return 0;
    if (type == FireStarter)
        return 0;
    if (type == Decor)
        return 0;
    if (type == Napalm)
        return 0;               /* Napalm has its own collision handler */
    if (type == Acid)
        return 0;               /* Acid too has its own collision handler */
    return 1;                   /* Rest hit the ship */
}

/* How near this projectile has to be to the ship to explode ? */
static unsigned char hit_ship_proximity (ProjectileType type)
{
    if (type == Energy)
        return 10;
    if (type == Mine || type == MagMine)
        return 16;
    if (type == Grenade)
        return 16;
    if (type == Missile)
        return 16;
    if (type == Tag)
        return 16;
    return 8;
}

/* Initialize */
void init_weapons (LDAT *explosionfile) {
    int r;
    projectiles = NULL;
    last_proj = NULL;
    explosions = NULL;
    wea_gravity.y = -WEAP_GRAVITY;
    wea_gravity.x = 0;
    for (r = 0; r < EXPL_FRAMES; r++) {
        wea_explosion[r] = load_image_ldat (explosionfile, 0, T_ALPHA, "EXPL", r);
    }
}

/* Clear weapons */
void clear_weapons (void)
{
    dllist_free(explosions,free);
    dllist_free(projectiles,free);
    explosions=NULL;
    projectiles=NULL;
    last_proj = NULL;
    gravity_weapon_count = 0;
}

Projectile *make_projectile (double x, double y, Vector v) {
    Projectile *newproj;
    newproj = malloc (sizeof (Projectile));

    newproj->x = x;
    newproj->y = y;
    newproj->gravity = NULL;
    newproj->vector.x = v.x * 7.0;
    newproj->vector.y = v.y * 7.0;
    newproj->color = col_default;
    newproj->type = Cannon;
    newproj->owner = NULL;
    newproj->ownerteam = -1;
    newproj->primed = 0;
    newproj->life = -1;
    newproj->var1 = 0;
    newproj->maxspeed = PROJ_MAXSPEED;
    if (game_settings.gravity_bullets)
        newproj->gravity = &wea_gravity;
    else
        newproj->gravity = NULL;
    newproj->wind_affects = game_settings.wind_bullets;
    return newproj;
}

/* Add a new projectile to list */
void add_projectile (Projectile * proj)
{
    last_proj=dllist_append(last_proj,proj);
    if(!projectiles) projectiles=last_proj;
    if (proj->type == GravityWell)
        gravity_weapon_count++;
}

/* Remove a projectile */
static struct dllist *remove_projectile(struct dllist *lst) {
    struct dllist *next=lst->next;
    free(lst->data);
    if(lst==last_proj) last_proj=last_proj->prev;
    if(lst==projectiles)
        projectiles=dllist_remove(lst);
    else
        dllist_remove(lst);
    return next;
}

void add_explosion (int x, int y, ProjectileType cluster) {
    struct Explosion *newentry;
    char solid;
    int fx, fy, rx, ry;
    playwave_3d (WAV_EXPLOSION, x, y);
    if (game_settings.explosions || (cluster != Noproj && cluster != Zap)) {
        newentry = malloc (sizeof (struct Explosion));
        newentry->x = x;
        newentry->y = y;
        newentry->frame = 0;
        if (cluster == Zap)
            newentry->cluster = Noproj;
        else
            newentry->cluster = cluster;
        explosions=dllist_prepend(explosions,newentry);
    }
    /* Gnaw a hole in terrain */
    if (cluster != Zap) {
        if (lev_level.solid[x][y] != TER_INDESTRUCT
            && !(level_settings.indstr_base
                 && (lev_level.solid[x][y] == TER_BASE
                     || lev_level.solid[x][y] == TER_BASEMAT)))
            for (fx = 0; fx < HOLE_W; fx++)
                for (fy = 0; fy < HOLE_H; fy++) {
                    if (wea_gnaw[fx][fy])
                        continue;
                    rx = fx - HOLE_W/2 + x;
                    ry = fy - HOLE_H/2 + y;
                    if (rx < 0 || ry < 0 || rx >= lev_level.width
                        || ry >= lev_level.height)
                        continue;
                    solid = lev_level.solid[rx][ry];
                    if (solid == TER_FREE)
                        continue;
                    if (solid == TER_INDESTRUCT || is_water (rx, ry))
                        continue;
                    if (level_settings.indstr_base
                        && (solid == TER_BASE || solid == TER_BASEMAT))
                        continue;
                    if (solid == TER_BASE && lev_level.base_area > 0)
                        lev_level.base_area--;
                    if (solid == TER_UNDERWATER || solid == TER_ICE) {
                        lev_level.solid[rx][ry] = TER_WATER;
                        putpixel (lev_level.terrain, rx, ry, lev_watercol);
                    } else {
                        lev_level.solid[rx][ry] = TER_FREE;
                        putpixel (lev_level.terrain, rx, ry, col_black);
                    }
                }
    } else {                    /* Burn terrain */
        for (fx = 0; fx < HOLE_W; fx++)
            for (fy = 0; fy < HOLE_H; fy++) {
                if (wea_gnaw[fx][fy])
                    continue;
                rx = fx - HOLE_W/2 + x;
                ry = fy - HOLE_H/2 + y;
                if (rx < 0 || ry < 0 || rx >= lev_level.width
                    || ry >= lev_level.height)
                    continue;
                solid = lev_level.solid[rx][ry];
                if (solid == TER_GROUND || solid == TER_UNDERWATER
                    || solid == TER_COMBUSTABLE || solid == TER_COMBUSTABL2
                    || solid == TER_TUNNEL || solid == TER_WALKWAY)
                    putpixel (lev_level.terrain, rx, ry, col_gray);
            }
        rx = x + HOLE_W/2;
        ry = y + HOLE_H/2;
        solid = lev_level.solid[rx][ry];
        if (solid == TER_COMBUSTABLE || solid == TER_COMBUSTABL2)
            start_burning (rx, ry);
    }
}

static void draw_special_weapons (int x, int y, Projectile * proj) {
    switch (proj->type) {
    case MegaBomb:
        putpixel (screen, x, y - 1, proj->color);
        putpixel (screen, x, y + 1, proj->color);
        putpixel (screen, x - 1, y - 1, proj->color);
        putpixel (screen, x + 1, y - 1, proj->color);
        break;
    case DividingMine:
    case Mine:
    case MagMine:
    case Acid:
        if (proj->color != 0) {
            putpixel (screen, x, y - 1, proj->color);
            putpixel (screen, x, y + 1, proj->color);
            putpixel (screen, x - 1, y, proj->color);
            putpixel (screen, x + 1, y, proj->color);
        }
        break;
    case Ember:
        if (proj->var1 > 1) {
            putpixel (screen, x, y - 2, proj->color);
            putpixel (screen, x, y + 2, proj->color);
            putpixel (screen, x - 2, y, proj->color);
            putpixel (screen, x + 2, y, proj->color);
        }
        if (proj->var1 > 0) {
            putpixel (screen, x, y - 1, proj->color);
            putpixel (screen, x, y + 1, proj->color);
            putpixel (screen, x - 1, y, proj->color);
            putpixel (screen, x + 1, y, proj->color);
        }
        break;
    case GravityWell:{
            double d;
            int r, r2;
            if (proj->var1 <= 0)
                proj->var1 = 8;
            else
                proj->var1--;
            for (r = 0; r < 12; r++) {
                d = -M_PI_4 * r + proj->var1 / 4.0;
                for (r2 = 4; r2 <= 16; r2 += 4)
                    putpixel (screen, x + sin (d - r2 / 6.0) * r2,
                              y + cos (d - r2 / 6.0) * r2, proj->color);
            }
        }
        break;
    case Spear:
    case Missile:
    case Rocket:
    case Mirv:
        {
            int dx, dy;
            dx = sin (proj->angle) * 3;
            dy = cos (proj->angle) * 3;
            draw_line (screen, x - dx, y - dy, x + dx, y + dy, proj->color);
        }
        break;
    case Zap:
        {
            int targx, targy, dx, dy, seg;
            targx = x + (proj->owner->x - proj->x);
            targy = y + (proj->owner->y - proj->y);
            for (seg = 0; seg < 4; seg++) {
                dx = (targx - x + ((rand () % 20) - 10)) / 3;
                dy = (targy - y + ((rand () % 20) - 10)) / 3;
                if (x > 0 && x < screen->w && y > 0 && y < screen->h)
                    if (x + dx > 0 && x + dx < screen->w && y + dy > 0
                        && y + dy < screen->h)
                        draw_line (screen, x, y, x + dx, y + dy, col_yellow);
                x += dx;
                y += dy;
            }
        }
        break;
    case Boomerang:
        putpixel (screen, x - Round (cos (proj->angle - 0.5)),
                  y - Round (sin (proj->angle - 0.5)), proj->color);
        putpixel (screen, x - Round (cos (proj->angle + 0.5)),
                  y - Round (sin (proj->angle + 0.5)), proj->color);
        break;
    default:
        break;
    }
}

static inline void draw_projectile (Projectile * projectile)
{
    int x, y, p;
    /* Some projectiles are not drawn */
    if (projectile->type == FireStarter)
        return;
    /* Draw the projectile */
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            x = Round(projectile->x) - cam_rects[p].x;
            y = Round(projectile->y) - cam_rects[p].y;
            if ((x > 0 && x < cam_rects[p].w) && (y > 0 && y < cam_rects[p].h)) {
                putpixel (screen, x + lev_rects[p].x, y + lev_rects[p].y,
                          projectile->color);
                if (game_settings.large_bullets) {
                    if (x + lev_rects[p].x + 1 < cam_rects[p].w) {
                        putpixel (screen, x + lev_rects[p].x + 1, y + lev_rects[p].y,
                                  projectile->color);
                    }
                    if (x + lev_rects[p].x - 1 > 0) {
                        putpixel (screen, x + lev_rects[p].x - 1, y + lev_rects[p].y,
                                  projectile->color);
                    }
                    if (y + lev_rects[p].y + 1 < cam_rects[p].h) {
                        putpixel (screen, x + lev_rects[p].x, y + lev_rects[p].y + 1,
                                  projectile->color);
                    }
                    if (y + lev_rects[p].y - 1 > 0) {
                        putpixel (screen, x + lev_rects[p].x, y + lev_rects[p].y - 1,
                                  projectile->color);
                    }
                }
                draw_special_weapons (x + lev_rects[p].x, y + lev_rects[p].y,
                                      projectile);
            }
        }
    }
}

static inline void draw_explosions (void) {
    struct dllist *elist = explosions;
    SDL_Surface *tmpsurf;
    SDL_Rect rect;
    int p;
    while (elist) {
        struct Explosion *expl=elist->data;
        tmpsurf = wea_explosion[expl->frame];
        for (p = 0; p < 4; p++) {
            if (players[p].state==ALIVE || players[p].state==DEAD) {
                rect.x = lev_rects[p].x + expl->x - cam_rects[p].x - 8;
                rect.y = lev_rects[p].y + expl->y - cam_rects[p].y - 8;
                if ((rect.x > lev_rects[p].x
                     && rect.x < lev_rects[p].x + cam_rects[p].w - tmpsurf->w)
                    && (rect.y > lev_rects[p].y
                        && rect.y <
                        lev_rects[p].y + cam_rects[p].h - tmpsurf->h))
                    SDL_BlitSurface (tmpsurf, NULL, screen, &rect);
            }
        }
        elist = elist->next;
    }
}

/*** WEAPON ANIMATION CODE ***/
void animate_weapons (void)
{
    struct dllist *plist = projectiles, *plist2, *plist3;
    struct dllist *elist = explosions, *enext;
    ProjectileType expl_cluster;
    signed int explode, solid;
    double f1;
    Projectile *proj;
    struct Explosion *expl;
    while (plist) {
        proj = plist->data;
        if (proj->type != Zap)
            expl_cluster = Noproj;
        else
            expl_cluster = Zap;
        /* Collision detection */
        if (proj->primed == 0) {
            explode = 0;
            /* Collide with a ship ? */
            if (can_collide_with_ship (proj->type)) {
                explode =
                    (int) hit_ship (proj->x, proj->y,
                                    (proj->type !=
                                     Boomerang) ? proj->owner : NULL,
                                    hit_ship_proximity (proj->type));
                if (explode)
                    explode = ship_damage ((struct Ship *) explode, proj);
                if (explode) {
                    if (explode == -2) {
                        /* Did the ship just swallow the projectile? */
                        plist=remove_projectile(plist);
                        continue;
                    }
                }
                /* Collide with a level special? (Same types of projectiles that collide with ships) */
                if (explode == 0)
                    explode = projectile_hit_special (proj);
            }
            /* Collide with a critter or a pilot? Also check if the projectile is underwater */
            if (explode == 0 && proj->type != GravityWell
                && proj->type != Energy && proj->type != MagWave
                && proj->type != FireStarter) {
                if (proj->type != Decor)
                    explode = hit_critter (Round(proj->x), Round(proj->y), proj->type);
                if (explode
                    && (proj->type == Spear || proj->type == FireStarter
                        || proj->type == Acid))
                    explode = 0;
                else if (explode
                         && (proj->type == Tag || proj->type == Napalm)) {
                    explode = 0;
                    if (proj->vector.x > proj->vector.y)
                        proj->vector.y *= -1;
                    else
                        proj->vector.x *= -1;
                }
                if (explode == 0 && (proj->type != Decor && proj->type!=FireStarter))
                    explode = hit_pilot (Round(proj->x), Round(proj->y),proj->ownerteam);
            }
            /* Check if the projectile goes outside its boundaries */
            if (proj->x <= 1)
                explode = -1;
            else if (proj->x >= lev_level.width - 1)
                explode = -1;
            if (proj->y <= 1)
                explode = -1;
            else if (proj->y >= lev_level.height - 1)
                explode = -1;
            /* Mines collide with other projectiles */
            if (explode == 0 && (proj->type == DividingMine || proj->type == Mine || proj->type == MagMine)) {/* Mines detonate when they hit other projectiles */
                plist3 = projectiles;
                while (plist3) {
                    Projectile *proj3=plist3->data;
                    if (proj == proj3) {
                        plist3 = plist3->next;
                        continue;
                    }
                    if (proj3->type == GravityWell || proj3->type == Energy
                        || proj3->type == MagWave) {
                        plist3 = plist3->next;
                        continue;
                    }
                    if (fabs (proj->x - proj3->x) < 2)
                        if (fabs (proj->y - proj3->y) < 2) {
                            explode++;
                            /* The other projectile is destroyed as well
                             * (except gravitywells and energybolts) */
                            remove_projectile(plist3);
                            break;
                        }
                    plist3 = plist3->next;
                }
            }
            /* A hack to make Claybomb react with things other than ground */
            if (explode && proj->type == Claybomb) {
                lev_level.solid[Round(proj->x)][Round(proj->y)] =
                    (lev_level.solid[Round(proj->x)][Round(proj->y)] ==
                     TER_WATER) ? TER_UNDERWATER : TER_GROUND;
                explode = 0;
            }
            /* Terrain collision detection */
            if (explode == 0 && proj->type != Energy
                && proj->type != GravityWell && proj->type != MagWave) {
                int tmpx,tmpy;
                solid =
                    hit_solid_line (Round(proj->x), Round(proj->y),
                                    Round(proj->x - proj->vector.x),
                                    Round(proj->y - proj->vector.y),
                                    &tmpx, &tmpy);
                if (solid < 0) { /* Hit water */
                    switch (proj->type) {
                        case DividingMine:
                        case Tag:
                        case Ember:
                        case Napalm:
                            explode = 1;
                            proj->x=tmpx; proj->y=tmpy;
                            break;
                        case Acid:
                        case Decor:
                        case Landmine:
                            explode = -1;
                            proj->x=tmpx; proj->y=tmpy;
                            break;
                        case Snowball:
                            explode = -1;
                            proj->x=tmpx; proj->y=tmpy;
                            alter_level (tmpx, tmpy,
                                     solid == TER_GROUND ? 30 : 15, Ice);
                            playwave_3d (WAV_FREEZE, proj->x, proj->y);
                        default:
                            break;
                    }
                } else if(solid>0) { /* Hit ground */
                    proj->x=tmpx; proj->y=tmpy;
                    if (solid == TER_EXPLOSIVE)
                        expl_cluster = Cannon;
                    else if (solid == TER_EXPLOSIVE2)
                        expl_cluster = Grenade;
                    switch (proj->type) {
                        /* Different types of reactions when projectiles hit something solid */
                        case FireStarter:
                            if ((solid == TER_COMBUSTABLE
                                 || solid == TER_COMBUSTABL2)
                                || (solid == TER_EXPLOSIVE
                                    || solid == TER_EXPLOSIVE2)) {
                                start_burning (Round(proj->x), Round(proj->y));
                                explode = -1;
                            }
                            break;
                        case Landmine:
                        case Tag:
                        case Napalm:
                            if (proj->type == Landmine)
                                proj->life = -1;
                            else if (solid == TER_COMBUSTABLE
                                     || solid == TER_COMBUSTABL2
                                     || solid == TER_EXPLOSIVE
                                     || solid == TER_EXPLOSIVE2)
                                start_burning (Round(proj->x), Round(proj->y));
                            proj->vector.x = 0;
                            proj->vector.y = 0;
                            proj->wind_affects = 0;
                            break;
                        case Plastique:
                            explode = -1;
                            alter_level (Round(proj->x), Round(proj->y),
                                         10, Explosive);
                            playwave_3d (WAV_SWOOSH, proj->x, proj->y);
                            break;
                        case Claybomb:
                            explode = -1;
                            alter_level (Round(proj->x), Round(proj->y), 15, Earth);
                            playwave_3d (WAV_SWOOSH, proj->x, proj->y);
                            break;
                        case Snowball:
                            explode = -1;
                            alter_level (Round(proj->x), Round(proj->y),
                                         solid == TER_GROUND ? 30 : 15, Ice);
                            playwave_3d (WAV_FREEZE, proj->x, proj->y);
                            break;
                        case Acid:
                            explode = -1;
                            start_melting (Round(proj->x), Round(proj->y), 10);
                            playwave_3d (WAV_STEAM, proj->x, proj->y);
                            break;
                        case Waterjet:
                        case Decor:
                            explode = -1;
                            break;
                        case Spear:{
                                int dx, dy;
                                dx = sin (proj->angle) * 6;
                                dy = cos (proj->angle) * 6;
                                draw_line (lev_level.terrain, Round(proj->x) - dx,
                                           proj->y - dy, Round(proj->x) + dx,
                                           proj->y + dy, proj->color);
                                explode = -1;
                            } break;
                        default:
                            explode = 1;
                            break;
                    }
                }
            }
            /* End terrain code */
            /* Larger explosions */
            if (explode) {
                if (explode > 0)
                    add_explosion (Round(proj->x), Round(proj->y), expl_cluster);
                switch (proj->type) {
                    case Grenade:
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        break;
                    case MegaBomb:
                        spawn_clusters (Round(proj->x), Round(proj->y),
                                        8, Grenade);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        playwave_3d(WAV_EXPLOSION2,proj->x,proj->y);
                        break;
                    case Missile:
                        spawn_clusters (Round(proj->x), Round(proj->y),
                                        10, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16,
                                        FireStarter);
                        break;
                    case Boomerang:
                        spawn_clusters (Round(proj->x), Round(proj->y), 4,  Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        break;
                    case Rocket:
                        spawn_clusters (Round(proj->x), Round(proj->y), 3, Grenade);
                        spawn_clusters (Round(proj->x), Round(proj->y), 6, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        playwave_3d(WAV_EXPLOSION2,proj->x,proj->y);
                        break;
                    case DividingMine:
                        spawn_clusters (Round(proj->x), Round(proj->y), 4, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        break;
                    case MagMine:
                    case Mine:
                        spawn_clusters (Round(proj->x), Round(proj->y), 8, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        break;
                    case GravityWell:
                        gravity_weapon_count--;
                        break;
                    case Mush:
                        spawn_clusters (Round(proj->x), Round(proj->y), 5, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16, FireStarter);
                        break;
                    case Ember:
                        spawn_clusters (Round(proj->x), Round(proj->y),
                                        3 * (proj->var1 + 1), Napalm);
                        spawn_clusters (Round(proj->x), Round(proj->y), 16,
                                        FireStarter);
                        break;
                    case Mirv:
                        if (proj->var1 == 3)
                            spawn_clusters (Round(proj->x), Round(proj->y), 3, Grenade);
                        else if (proj->var1 == 2)
                            spawn_clusters (Round(proj->x), Round(proj->y), 32, Cannon);
                        else if (proj->var1 == 1)
                            spawn_clusters (Round(proj->x), Round(proj->y), 12, Cannon);
                        else
                            spawn_clusters (Round(proj->x), Round(proj->y), 6, Cannon);
                        spawn_clusters (Round(proj->x), Round(proj->y), 8, FireStarter);
                        break;
                    case Zap:
                        proj->primed = proj->life;
                        break;
                    default:
                        break;
                }
                if ((proj->type != Zap
                     || (proj->type == Zap && proj->life <= 1))) {
                    plist=remove_projectile(plist);
                }
                continue;
            }
        }
        /* Weapon specific movement/fx code */
        switch (proj->type) {
        case Missile:
            missile_movement (proj);
            break;
        case MagMine:
            if (proj->primed <= 0)
                magmine_movement (proj);
            break;
        case DividingMine:
            dividingmine_movement (proj, projectiles);
            break;
        case Mine:
            if (proj->primed <= 0)
                mine_movement (proj);
            break;
        case Rocket:
            rocket_movement (proj);
            break;
        case Spear:
            if (game_settings.gravity_bullets || gravity_weapon_count)
                spear_movement (proj);
            break;
        case Energy:
            energy_movement (proj);
            break;
        case Tag:
            tag_movement (proj);
            break;
        case Acid:
            acid_movement (proj);
            break;
        case Waterjet:
            water_movement (proj);
            break;
        case Ember:
            ember_movement (proj);
            break;
        case Napalm:
            napalm_movement (proj);
            break;
        case Boomerang:
            boomerang_movement (proj);
            break;
        case Mush:
            mush_movement (proj);
            break;
        case Zap:
            expl_cluster = Zap; /* This is to tell add_explosion to char the ground */
            break;
        case Mirv:
            mirv_movement (proj);
            break;
        case MagWave:
            magwave_movement (proj);
            break;
        default:
            break;
        }
        /* End weapon specific movement/fx code */
        /* Check if someone is operating a force field nearby */
        if (proj->type != GravityWell) {
            struct dllist *ships = ship_list;
            while (ships) {
                struct Ship *ship=ships->data;
                if (ship->shieldup) {
                    f1 = hypot (fabs (ship->x - proj->x),
                                fabs (ship->y - proj->y));
                    if (f1 < 100) {
                        proj->vector.x +=
                            ((ship->x -
                              proj->x) / (fabs (ship->x - proj->x) +
                                          0.1)) / 1.3;
                        proj->vector.y +=
                            ((ship->y -
                              proj->y) / (fabs (ship->y - proj->y) +
                                          0.1)) / 1.3;
                        if (proj->type == Rocket || proj->type == Mirv)
                            proj->angle =
                                atan2 (proj->vector.x, proj->vector.y);
                    }
                } else if (ship->antigrav) {
                    f1 = hypot (fabs (ship->x - proj->x),
                                fabs (ship->y - proj->y));
                    if (f1 < 250 && proj->gravity) {
                        proj->vector.y -= proj->gravity->y * 2.0;
                    }
                }
                ships = ships->next;
            }
        }
        /* Is this is a gravity weapon ? */
        if (proj->type == GravityWell && proj->primed == 0) {
            struct dllist *ships = ship_list;
            plist2 = projectiles;   /* Attract other projectiles */
            while (plist2) {
                Projectile *proj2=plist2->data;
                if (proj2->type != GravityWell
                    && proj2->type != Energy) {
                    if (fabs (proj2->x - proj->x) < 150)
                        if (fabs (proj2->y - proj->y) < 150) {
                            proj2->vector.x -=
                                ((proj->x - proj2->x) / (abs (proj->x -
                                                                 proj2->x) +
                                                            0.1));
                            proj2->vector.y -= ((proj->y -
                                  proj2->y) / (abs (proj->y -
                                                                 proj2->y) +
                                                            0.1));
                            if (proj2->type == Rocket || proj2->type == Mirv)
                                proj2->angle = atan2 (proj2->vector.x,
                                           proj2->vector.y);
                        }
                }
                plist2 = plist2->next;
            }
            while (ships) {  /* Attract ships */
                struct Ship *ship=ships->data;
                if (fabs (ship->x - proj->x) < 100)
                    if (fabs (ship->y - proj->y) < 100) {
                        ship->vector.x +=
                            (ship->x -
                             proj->x) / (fabs (ship->x - proj->x) +
                                         0.01) / 1.8;
                        ship->vector.y +=
                            (ship->y -
                             proj->y) / (fabs (ship->y - proj->y) +
                                         0.01) / 1.8;
                    }
                ships = ships->next;
            }
            cow_storm (proj->x, proj->y);   /* Attract critters */
        }
        /* End gravity weapon code */
        /* Common movement code */
        proj->x -= proj->vector.x;
        proj->y -= proj->vector.y;
        if (Round(proj->x) < 0)
            proj->x = 0;
        else if (Round(proj->x) >= lev_level.width)
            proj->x = lev_level.width - 1;
        if (Round(proj->y) < 0)
            proj->y = 0;
        else if (Round(proj->y) >= lev_level.height)
            proj->y = lev_level.height - 1;
        if (proj->wind_affects
            && lev_level.solid[Round(proj->x)][Round(proj->y)] == TER_FREE) {
            proj->vector.x -= weather_wind_vector / 50.0;
        }
        if (proj->gravity) {
            proj->vector = addVectors (&proj->vector, proj->gravity);
            proj->vector.x /= 1.005;
            proj->vector.y /= 1.005;
            switch (lev_level.solid[Round(proj->x)][Round(proj->y)]) {
                case TER_FREE:
                    if(proj->wind_affects) proj->vector.x -= weather_wind_vector / 50.0;
                    break;
                case TER_WATERFD:
                    proj->vector.y -= 3;
                    proj->vector.x *= 0.93;
                    break;
                case TER_WATERFR:
                    proj->vector.x -= 3;
                    proj->vector.y *= 0.93;
                    break;
                case TER_WATERFL:
                    proj->vector.x += 3;
                    proj->vector.y *= 0.93;
                    break;
                case TER_WATERFU:
                    proj->vector.y += 3;
                    proj->vector.x *= 0.93;
                    break;
                case TER_WATER:
                    if (proj->vector.x > PROJ_UW_MAXSPEED)
                        proj->vector.x = PROJ_UW_MAXSPEED;
                    else if (proj->vector.x < -PROJ_UW_MAXSPEED)
                        proj->vector.x = -PROJ_UW_MAXSPEED;
                    if (proj->vector.y > PROJ_UW_MAXSPEED)
                        proj->vector.y = PROJ_UW_MAXSPEED;
                    else if (proj->vector.y < -PROJ_UW_MAXSPEED)
                        proj->vector.y = -PROJ_UW_MAXSPEED;
                    break;
                default:
                    if (proj->vector.x > proj->maxspeed)
                        proj->vector.x = proj->maxspeed;
                    else if (proj->vector.x < -proj->maxspeed)
                        proj->vector.x = -proj->maxspeed;
                    if (proj->vector.x > proj->maxspeed)
                        proj->vector.y = proj->maxspeed;
                    else if (proj->vector.y < -proj->maxspeed)
                        proj->vector.y = -proj->maxspeed;
                    break;
            }
        }
        if (proj->life > 0)
            proj->life--;
        if (proj->primed > 0)
            proj->primed--;
        /* Draw the projectile */
        draw_projectile (proj);
        /* Remove old projectiles */
        if (proj->life == 0) {
            plist=remove_projectile(plist);
        } else {
            plist = plist->next;
        }
    }
    while (elist) {
        expl = elist->data;
        enext = elist->next;
        expl->frame++;
        if (expl->frame == EXPLOSION_CLUSTER_SPEED
            && expl->cluster != Noproj)
            spawn_clusters (expl->x, expl->y,
                            expl->cluster == Grenade ? 3 : 6,
                            expl->cluster);
        if (expl->frame == EXPL_FRAMES) {   /* Animation is over */
            free (expl);
            if(elist==explosions)
                explosions=dllist_remove(elist);
            else
                dllist_remove(elist);
        }
        elist = enext;
    }
    draw_explosions ();
}

void spawn_clusters (int x, int y, int count, ProjectileType type) {
    Projectile *cluster;
    double angle, incr, r;
    angle = 0;
    incr = (2.0 * M_PI) / (double) count;
    while (angle < 2.0 * M_PI) {
        r = (rand () % 10) / 10.0;
        cluster =
            make_projectile (x, y,
                             makeVector (sin (angle + r) * 0.8,
                                         cos (angle + r) * 0.8));
        cluster->type = type;
        cluster->primed = 1;
        if (type == Grenade)
            cluster->color = col_grenade;
        else if (type == Napalm) {
            cluster->life = 30;
            cluster->color = col_white;
        } else if (type == FireStarter) {
            cluster->vector.y /= 2.0;
            cluster->life = 30;
        }
        cluster->gravity = &wea_gravity;
        add_projectile (cluster);
        angle += incr;
    }
}

int detonate_remote (struct Ship *plr) {
    struct dllist *list = projectiles;
    Projectile *proj;
    float f;
    while (list) {
        Projectile *projectile=list->data;
        if (projectile->type == Landmine
            && projectile->owner == plr) {
            spawn_clusters (projectile->x, projectile->y, 5,Cannon);
            for (f = projectile->angle - 0.25;
                 f < projectile->angle + 0.25; f += 0.05) {
                proj =
                    make_projectile (projectile->x, projectile->y,
                                     makeVector (sin (f) * 1.5,
                                                 cos (f) * 1.5));
                add_projectile (proj);
            }
            remove_projectile(list);
            return 1;
        }
        list = list->next;
    }
    return 0;
}

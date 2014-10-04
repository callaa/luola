/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : ship.c
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "defines.h"
#include "console.h"
#include "player.h"
#include "level.h"
#include "list.h"
#include "ldat.h"
#include "fs.h"
#include "weather.h"
#include "animation.h"
#include "particle.h"
#include "audio.h"
#include "special.h"
#include "critter.h"
#include "levelfile.h"
#include "stringutil.h"
#include "ship.h"

/* Exported globals */
SDL_Surface **ship_gfx[7];      /* 0=grey, 1-4=coloured, 5 = white, 6 =  frozen */
SDL_Surface **ghost_gfx[4];
struct dllist *ship_list;

/* Internally used globals */
static SDL_Surface *shield_gfx[4];      /* 0-3 coloured */
static SDL_Surface **remocon_gfx;
static Vector really_weak_gravity, strong_gravity;

/* Internally used functions */
static char ship_fire_special_weapon (struct Ship * ship);
static char ship_fire_special_noproj (struct Ship * ship);
static void ship_fire_standard_weapon (struct Ship * ship, Projectile * p, Vector v,
                                       Vector ps);
static void ship_exhaust (struct Ship * ship, Vector vec, signed char solid);
static void ship_tagged (struct Ship * ship);
static void ship_specials (struct Ship * ship);
static void ghostify (struct Ship * ship, char status);
static void cloaking_device (struct Ship * ship, char status);
static void ship_splash (struct Ship * ship);

/* Initialize */
int init_ships (void) {
    LDAT *playerdata;
    int r, p;
    ship_list = NULL;
    really_weak_gravity = makeVector (0, -WEAP_GRAVITY / 4.0);
    strong_gravity = makeVector (0, -GRAVITY * 2.0);
    /* Load ship graphics */
    playerdata =
        ldat_open_file (getfullpath (GFX_DIRECTORY, "player.ldat"));
    if(!playerdata) return 1;
    for (r = 0; r < 7; r++) {
        ship_gfx[r] = malloc (sizeof (SDL_Surface) * SHIP_POSES);
        for (p = 0; p < SHIP_POSES; p++) {
            ship_gfx[r][p] = load_image_ldat (playerdata, 0, 1, "VWING", p);
            switch (r) {
            case Grey:
                recolor (ship_gfx[r][p], 0.6, 0.6, 0.6, 1);
                break;
            case Frozen:
                recolor (ship_gfx[r][p], 0, 1, 1, 1);
                break;
            case White:
                recolor (ship_gfx[r][p], 1.25, 1.25, 1.25, 1);
                break;
            case Blue:
                recolor (ship_gfx[r][p], 0, 0.2, 1, 1);
                break;
            case Red:
                recolor (ship_gfx[r][p], 1, 0.2, 0, 1);
                break;
            case Green:
                recolor (ship_gfx[r][p], 0.2, 1, 0.2, 1);
                break;
            case Yellow:
                recolor (ship_gfx[r][p], 1, 1, 0.4, 1);
                break;
            }
        }
    }
    /* Create ghost ship graphics */
    for (r = 0; r < 4; r++) {
        ghost_gfx[r] = malloc (sizeof (SDL_Surface) * SHIP_POSES);
        for (p = 0; p < SHIP_POSES; p++) {
            ghost_gfx[r][p] = copy_surface (ship_gfx[r + 1][p]);
            recolor (ghost_gfx[r][p], 1.0, 1.0, 1.0, 0.5);
        }
    }
    /* Load Shield graphics */
    for (r = 0; r < 4; r++) {
        shield_gfx[r] = load_image_ldat (playerdata, 0, 1, "SHIELD", 0);
        switch (r + Red) {
        case Red:
            recolor (shield_gfx[r], 1, 0.4, 0.4, 1);
            break;
        case Blue:
            recolor (shield_gfx[r], 0.4, 0.4, 1, 1);
            break;
        case Green:
            recolor (shield_gfx[r], 0.4, 1, 0.4, 1);
            break;
        case Yellow:
            recolor (shield_gfx[r], 1, 1, 0.6, 1);
            break;
        default:
            printf ("Warning ! Value of 'r' is %d in init_ships()!\n", r);
            break;
        }
    }
    /* Load Remote Control graphics */
    ldat_free (playerdata);
    playerdata =
        ldat_open_file (getfullpath (GFX_DIRECTORY, "xmit.ldat"));
    if(!playerdata) return 1;
    remocon_gfx =
        load_image_array (playerdata, 0, 1, "XMIT", 0, REMOTE_FRAMES - 1);
    ldat_free (playerdata);
    return 0;
}

/* Remove ships */
void clean_ships (void)
{
    dllist_free(ship_list,free);
    ship_list=NULL;
}

/* Prepare for a new level */
void reinit_ships (LevelSettings * settings)
{
    LSB_Objects *object = NULL;
    SWeaponType standard;
    WeaponType special;
    PlayerColor color;
    struct Ship *newship;
    if (settings)
        object = settings->objects;
    while (object) {
        if (object->type == 0x20) {
            switch (object->value) {
            case 1:
                color = Red;
                break;
            case 2:
                color = Blue;
                break;
            case 3:
                color = Green;
                break;
            case 4:
                color = Yellow;
                break;
            default:
                color = Grey;
                standard = SShot;
                special = WGrenade;
                break;
            }
            if (color != Grey) {
                standard = players[color - 1].standardWeapon;
                special = players[color - 1].specialWeapon;
            } else {
                standard = SShot;
                special = WGrenade;
            }
            newship = create_ship (color, standard, special);
            newship->x = object->x;
            newship->y = object->y;
        }
        object = object->next;
    }
}

/* Create a new ship. It is automatically added to the ship list */
struct Ship *create_ship (PlayerColor color, SWeaponType weapon, WeaponType special)
{
    struct Ship *newship = NULL;
    newship = malloc (sizeof (struct Ship));
    memset (newship, 0, sizeof (struct Ship));
    newship->ship = ship_gfx[color];
    newship->shield = shield_gfx[color - Red];
    newship->health = 1.0;
    newship->energy = 1.0;
    newship->visible = 1;
    newship->afterburn = 1;     /* afterburn==1 normal, afterburn==2 BURN ! */
    newship->maxspeed = MAXSPEED;
    newship->standard = weapon;
    newship->special = special;
    newship->color = color;

    if(ship_list)
        dllist_append(ship_list,newship);
    else
        ship_list=dllist_append(ship_list,newship);
    return newship;
}

/* Draw ships on screen */
void draw_ships (void)
{
    struct dllist *current=ship_list;
    SDL_Rect rect, rect2;
    int plr, pose;
    struct Ship *ship;
    while (current) {
        ship = current->data;
        if (ship->visible > 0) {
            for (plr = 0; plr < 4; plr++) {
                rect.w = ship_gfx[Grey][0]->w;
                rect.h = ship_gfx[Grey][0]->h;
                if (players[plr].state==ALIVE||players[plr].state==DEAD) {
                    rect.x =
                        lev_rects[plr].x + Round(ship->x) - cam_rects[plr].x - 8;
                    rect.y =
                        lev_rects[plr].y + Round(ship->y) - cam_rects[plr].y - 8;
                    if (rect.x < lev_rects[plr].x - 16
                        || rect.y < lev_rects[plr].y - 16
                        || rect.x > lev_rects[plr].x + cam_rects[plr].w
                        || rect.y > lev_rects[plr].y + cam_rects[plr].h)
                        continue;
                    rect2 =
                        cliprect (rect.x, rect.y, rect.w, rect.h,
                                  lev_rects[plr].x, lev_rects[plr].y,
                                  lev_rects[plr].x + cam_rects[plr].w,
                                  lev_rects[plr].y + cam_rects[plr].h);
                    if (rect.x < lev_rects[plr].x)
                        rect.x = lev_rects[plr].x;
                    if (rect.y < lev_rects[plr].y)
                        rect.y = lev_rects[plr].y;
                    if (players[plr].ship == ship && radars_visible)
                        draw_radar (rect, plr);
                    pose =
                        Roundplus ((35 - ship->angle / (2.0 * M_PI / 35.0)));
                    if (pose > 35)
                        pose = 35;
                    if (ship->dead)
                        SDL_BlitSurface (ship_gfx[Grey][pose], &rect2, screen,
                                         &rect);
                    else if (ship->frozen)
                        SDL_BlitSurface (ship_gfx[Frozen][pose], &rect2,
                                         screen, &rect);
                    else if (ship->white_ship)
                        SDL_BlitSurface (ship_gfx[White][pose], &rect2,
                                         screen, &rect);
                    else
                        SDL_BlitSurface (ship->ship[pose], &rect2, screen,
                                         &rect);
                    if (ship->shieldup) {
                        SDL_Rect sr, tr;
                        tr.x =
                            lev_rects[plr].x + Round(ship->x) - cam_rects[plr].x
                            - 16;
                        tr.y =
                            lev_rects[plr].y + Round(ship->y) - cam_rects[plr].y
                            - 16;
                        tr.w = ship->shield->w;
                        tr.h = ship->shield->h;
                        sr = cliprect (tr.x, tr.y, tr.w, tr.h,
                                       lev_rects[plr].x, lev_rects[plr].y,
                                       lev_rects[plr].x + cam_rects[plr].w,
                                       lev_rects[plr].y + cam_rects[plr].h);
                        if (tr.x < lev_rects[plr].x)
                            tr.x = lev_rects[plr].x;
                        if (tr.y < lev_rects[plr].y)
                            tr.y = lev_rects[plr].y;
                        SDL_BlitSurface (ship->shield, &sr, screen, &tr);
                    }
                    if (ship->remote_control) {
                        SDL_Rect sr, tr;
                        tr.x =
                            lev_rects[plr].x + Round(ship->x) - cam_rects[plr].x -
                            16;
                        tr.y =
                            lev_rects[plr].y + Round(ship->y) - cam_rects[plr].y -
                            16;
                        tr.w = remocon_gfx[ship->anim]->w;
                        tr.h = remocon_gfx[ship->anim]->h;
                        sr = cliprect (tr.x, tr.y, tr.w, tr.h,
                                       lev_rects[plr].x, lev_rects[plr].y,
                                       lev_rects[plr].x + cam_rects[plr].w,
                                       lev_rects[plr].y + cam_rects[plr].h);
                        if (tr.x < lev_rects[plr].x)
                            tr.x = lev_rects[plr].x;
                        if (tr.y < lev_rects[plr].y)
                            tr.y = lev_rects[plr].y;
                        SDL_BlitSurface (remocon_gfx[ship->anim], &sr, screen,
                                         &tr);
                    }
                    if (ship->darting) {
                        int cx, cy;
                        float dx, dy;
                        cx = rect.x + ship_gfx[Grey][0]->w / 2;
                        cy = rect.y + ship_gfx[Grey][0]->h / 2;
                        if (ship->darting == DARTING) {
                            dx = sin (ship->angle);
                            dy = cos (ship->angle);
                        } else {
                            dx = ship->vector.x / 7;
                            dy = ship->vector.y / 7;
                        }
                        if (ship->darting == DARTING) {
                            draw_line (screen, cx - dx * 5, cy - dy * 5,
                                       cx - dx * 10, cy - dy * 10, col_gray);
                        } else {
                            draw_line (screen, cx + dx * 3, cy + dy * 3,
                                       cx - dx * 7, cy - dy * 7, col_gray);
                        }
                    }
                }
            }
        }
        current = current->next;
    }
}

/** Finish off a dead ship ***/
static void finalize_ship(struct Ship *ship) {
    int num = find_player (ship);
    ship->dead = 2;
    ship->visible = 0;

    /* Mark the player as dead if he has not yet ejected */
    if (num >= 0) {
        buryplayer (num);
    }

    /* Make sure no pilots have roped this ship */
    for(num=0;num<4;num++) {
        if(players[num].state == ALIVE && players[num].pilot.rope_ship==ship) {
            pilot_detach_rope(&players[num].pilot);
        }
    }

    /* Explosion and sound effect */
    spawn_clusters (Round(ship->x), Round(ship->y), 32, Cannon);
    playwave(WAV_EXPLOSION2);
}

/** Stop special weapons **/
static void ship_stop_special(struct Ship *ship) {
    if (ship->remote_control) {
        if ((int) ship->remote_control > 1) {
            ship_fire (ship, SpecialWeapon);
        } else {
            struct dllist *tmp2 = ship_list;
            while (tmp2) {
                if (((struct Ship*)tmp2->data)->remote_control == ship) {
                    ship_fire (tmp2->data, SpecialWeapon);
                    break;
                }
                tmp2 = tmp2->next;
            }
        }
    }
    ship->fire_special_weapon = 0;
    ship->antigrav = 0;
    ship->shieldup = 0;
    ship->afterburn = 1;
    ship->repairing = 0;
    ship->visible = 1;
    if (ship->ghost) {
        ship->ghost = 0;
        ghostify (ship, 0);
    }
}

/** Ship animation **/
void animate_ships (void) {
    struct dllist *current = ship_list, *tmplist = NULL;
    double newx,newy;
    Vector tmpv, sv;
    struct Ship *ship;
    int solid;
    double f1;
    while (current) {
        struct dllist *next=current->next;
        ship = current->data;
        if (ship->dead == 0) {
            /* Counters */
            if (ship->cooloff)
                ship->cooloff--;
            if (ship->special_cooloff)
                ship->special_cooloff--;
            if (ship->eject_cooloff)
                ship->eject_cooloff--;
            if (ship->tagged)
                ship->tagged--;
            if (ship->white_ship > 0)
                ship->white_ship--;
            if (ship->no_power > 0)
                ship->no_power--;
            if (ship->visible < 0) {
                if (ship->visible == -1)
                    ship->visible = 1;
                else
                    ship->visible++;
            }
            if ((ship->criticals & CRITICAL_CARGO) && ship->energy > 0)
                ship->energy -= 0.001;
            if(ship->no_power == 0) {
                /* Turn the ship */
                ship->angle += ship->turn;
                /* Fire normal weapon */
                if (ship->fire_weapon && ship->cooloff == 0)
                    ship_fire (ship, NormalWeapon);
                /* Fire special weapons. */
                if (ship->fire_special_weapon) {
                    if (ship_fire_special_weapon (ship))
                        ship_fire (ship, SpecialWeapon2);
                    else if (ship->special != WWatergun
                             && ship->special != WFlame)
                        ship_fire (ship, SpecialWeapon);
                }
            }
            sv = ship->vector;
            /* Player specials like cloaking device and such */
            ship_specials (ship);
            /* Motion */
            if (ship->angle > 2 * M_PI)
                ship->angle = 0;
            if (ship->angle < 0)
                ship->angle = 2 * M_PI;
            if ((ship->thrust && ship->no_power==0) || ship->afterburn > 1 ||
                    ship->darting || ((ship->criticals & CRITICAL_FUELCONTROL)
                    && rand () % 6 == 0)) {
                if (ship->darting == DARTING)
                    tmpv =
                        makeVector (sin (ship->angle) * THRUST * 3.75,
                                    cos (ship->angle) * THRUST * 3.75);
                else if (ship->darting == SPEARED)
                    tmpv = ship->vector;
                else if (ship->afterburn == 1
                         && !(ship->criticals & CRITICAL_FUELCONTROL))
                    tmpv =
                        makeVector (sin (ship->angle) * THRUST * ship->thrust,
                                    cos (ship->angle) * THRUST *
                                    ship->thrust);
                else
                    tmpv =
                        makeVector (sin (ship->angle) * THRUST *
                                    ship->afterburn,
                                    cos (ship->angle) * THRUST *
                                    ship->afterburn);
                if ((ship->criticals & CRITICAL_ENGINE)) {      /* Critical engine core */
                    Particle *smoke;
                    int s;
                    ship->health -= 0.001;
                    if (ship->health <= 0.0) {
                        killship (ship);
                        spawn_clusters (Round(ship->x), Round(ship->y), 6, Napalm);
                    }
                    for (s = 0; s < 4; s++) {
                        smoke =
                            make_particle (ship->x + 8 - rand () % 16,
                                           ship->y + 8 - rand () % 16, 15);
                        smoke->vector.x = weather_wind_vector;
                        smoke->vector.y = 3.0 * (rand () % 20) / 10.0;
                    }
                }
                solid = hit_solid (Round(ship->x), Round(ship->y));
                sv = addVectors (&ship->vector, &tmpv);
                if (ship->visible)
                    ship_exhaust (ship, tmpv, solid);
                if (ship->afterburn > 1 && solid >= 0)
                    start_burning (ship->x, ship->y);
            }
        } else {                /* Ship is dead */
            sv = ship->vector;
            ship->angle += ship->turn;
            if (ship->angle > M_PI * 2.0)
                ship->angle = 0;
            else if (ship->angle < 0)
                ship->angle = M_PI * 2.0;
        }
        /* Check if someone is operating a forcefield nearby */
        tmplist = ship_list;
        while (tmplist) {
            struct Ship *tmpship=tmplist->data;
            if (tmpship != ship && tmpship->shieldup) {
                f1 = hypot (fabs (tmpship->x - ship->x),
                            fabs (tmpship->y - ship->y));
                if (f1 < 100) {
                    sv.x +=
                        ((tmpship->x -
                          ship->x) / (fabs (tmpship->x - ship->x) +
                                      0.1)) / 1.3;
                    sv.y +=
                        ((tmpship->y -
                          ship->y) / (fabs (tmpship->y - ship->y) +
                                      0.1)) / 1.3;
                }
            } else if (tmpship->antigrav) {
                f1 = hypot (fabs (tmpship->x - ship->x),
                            fabs (tmpship->y - ship->y));
                if (f1 < 200) {
                    sv.y -= gravity.y * 2.0;
                }
            }
            tmplist = tmplist->next;
        }
        /* Gravity */
        if (ship->special != WAntigrav)
            tmpv = addVectors (&sv, &gravity);
        else
            tmpv = sv;
        /* Air viscosity */
        tmpv.x /= 1.01;
        tmpv.y /= 1.01;
        /* Speed limits */
        if (tmpv.x > ship->maxspeed + fabs (ship->thrust * 2.0))
            tmpv.x = ship->maxspeed + fabs (ship->thrust * 2.0);
        if (tmpv.y > ship->maxspeed + fabs (ship->thrust * 2.0))
            tmpv.y = ship->maxspeed + fabs (ship->thrust * 2.0);
        if (tmpv.x < -ship->maxspeed - fabs (ship->thrust * 2.0))
            tmpv.x = -ship->maxspeed - fabs (ship->thrust * 2.0);
        if (tmpv.y < -ship->maxspeed - fabs (ship->thrust * 2.0))
            tmpv.y = -ship->maxspeed - fabs (ship->thrust * 2.0);
        ship->vector = tmpv;
        newx = ship->x - tmpv.x;
        newy = ship->y - tmpv.y;
        /* Hitting the level boundaries when the ship is gray explodes the ship */
        if (((newx < 1 || newy < 1)
             || (newx >= lev_level.width || newy >= lev_level.height))
            && ship->dead == 1) {
            int num;
            level_bounds(&ship->x,&ship->y);
            num = find_player (ship);
            ship->dead = 2;
            ship->visible = 0;
            if (num >= 0) {
                buryplayer (num);
            }
            spawn_clusters (Round(ship->x), Round(ship->y), 32, Cannon);
            spawn_clusters (Round(ship->x), Round(ship->y), 32, FireStarter);
        }
        solid = 0;
        if (newx < 1) {
            newx = 1;
            ship->vector.x=0;
            solid = 1;
        } else if (Round(newx) >= lev_level.width) {
            newx = lev_level.width - 1;
            ship->vector.x=0;
            solid = 1;
        }
        if (newy < 1) {
            newy = 1;
            ship->vector.y=0;
            solid = 1;
        } else if (newy >= lev_level.height-8) {
            newy = lev_level.height - 8;
            ship->vector.y=0;
            solid = 1;
        }
        if (solid) {
            ship->frozen = 0;
            if (ship->darting) {
                ship->darting = NODART;
                ship->maxspeed = MAXSPEED;
            }
            if (ship->dead == 1) {
                finalize_ship(ship);
            }
        }
        /* Terrain collision detection */
        if (lev_level.solid[Round(newx)][Round(newy) + 1] == TER_BASE
            && lev_level.solid[Round(newx)][Round(newy)] != TER_BASE && ship->ghost == 0)
        {
            ship->x = newx;
            ship->y = newy;
        }
        solid = hit_solid (newx, newy);
        if (solid < 0 && ship->ghost == 0) {    /* Underwater ? */
            if (lev_level.solid[Round(ship->x)][Round(ship->y)] == TER_FREE) {
                /* Splash when going in to the water */
                ship_splash (ship);
            }
            if (ship->dead == 0)
                ship->vector.y += GRAVITY * 2.1;
            else
                ship->vector.y += GRAVITY * 0.5;
            if (solid == -2) {
                ship->vector.y = ship->maxspeed;
            } else if (solid == -3) {
                ship->vector.x = -ship->maxspeed;
            } else if (solid == -4) {
                ship->vector.y = -ship->maxspeed;
            } else if (solid == -5) {
                ship->vector.x = ship->maxspeed;
            } else {
                if (ship->vector.x > ship->maxspeed / 2)
                    ship->vector.x = ship->maxspeed / 2;
                if (ship->vector.y > ship->maxspeed / 2)
                    ship->vector.y = ship->maxspeed / 2;
                if (ship->vector.x < -ship->maxspeed / 2)
                    ship->vector.x = -ship->maxspeed / 2;
                if (ship->vector.y < -ship->maxspeed / 2)
                    ship->vector.y = -ship->maxspeed / 2;
            }
            ship->frozen = 0;
        } else if (solid > 0 && ship->ghost == 0) {     /* Hit ground ? */
            if (ship->dead == 1) {
                finalize_ship(ship);
            }
            ship->frozen = 0;
            if (ship->darting) {
                if (ship->darting == SPEARED) {  /* If the ship is speared, it sinks into the terrain a bit */
                    unsigned char tmp_solid;
                    tmp_solid =
                        lev_level.solid[Round(ship->x - ship->vector.x)]
                                       [Round(ship->y-ship->vector.y)];
                    if (tmp_solid != TER_INDESTRUCT && tmp_solid != TER_BASE) {
                        ship->x -= ship->vector.x;
                        ship->y -= ship->vector.y;
                        if (tmp_solid == TER_EXPLOSIVE)
                            spawn_clusters (Round(ship->x), Round(ship->y),
                                    6, Cannon);
                        else if (tmp_solid == TER_EXPLOSIVE2)
                            spawn_clusters (Round(ship->x), Round(ship->y),
                                    6, Grenade);
                    }
                    ship->health -= 0.05; /* Spears hurt even more than darts ! */
                    ship->white_ship = SHIP_WHITE_DUR;
                }
                ship->darting = NODART;
                ship->maxspeed = MAXSPEED;
                ship->health -= 0.1;    /* Darts hurt ! */
                ship->white_ship = SHIP_WHITE_DUR;
                if (ship->health < 0)
                    ship->health = 0;
            }
            if (game_settings.coll_damage && solid != TER_SNOW) {       /* Collision damage */
                f1 = hypot (ship->vector.x,
                            ship->vector.y) / (double) (MAXSPEED);
                if (f1 > 0.5) {
                    ship->health -= 0.01;
                    ship->white_ship = SHIP_WHITE_DUR;
                }
            }
            if (ship->health <= 0 && !ship->dead)
                killship (ship);
            /* Lose all inertia */
            if (solid != TER_SNOW && solid != TER_ICE) {
                ship->vector.x = 0;
                ship->vector.y = 0;
            } else {
                if (ship->vector.y > 0) {       /* Burrow in snow */
                    int snowx,snowy;
                    snowx = Round(newx);
                    snowy = Round(newy);
                    if (solid == TER_SNOW) {
                        putpixel (lev_level.terrain, snowx, snowy, col_black);
                        lev_level.solid[snowx][snowy] = TER_FREE;
                    } else {
                        putpixel (lev_level.terrain, snowx, snowy,
                                  lev_watercol);
                        lev_level.solid[snowx][snowy] = TER_WATER;
                    }
                    add_snowflake (Round(ship->x), Round(ship->y));
                }
                ship->vector.y = 0;
                ship->vector.x /= 2;
            }
            /* Is the player sitting on a base ? */
            if (solid == TER_BASE && ship->dead == 0) {
                ship->onbase = 1;

                /* Counter the antigravity weapon */
                if(ship->special==WAntigrav) ship->vector.y+=gravity.y;

                /* Repair the ship */
                if (ship->health < 1 && !ship->shieldup) {      
                    Particle *spark;
                    ship->health += 0.0012;
                    /* Special effects */
                    spark =
                        make_particle (ship->x + (rand () % 16) - 8,
                                       ship->y + (rand () % 16) - 8, 6);
                    spark->vector.x = (rand () % 4) - 2;
                    spark->vector.y = 3;
                    spark->color[0] = 255;
                    spark->color[1] = 255;
                    spark->color[2] = 255;
                    spark->rd = -100;
                    spark->gd = -100;
                    spark->bd = -15;
                }
                if (ship->health > 1.0)
                    ship->health = 1.0;
                if (!ship->shieldup && !ship->repairing
                    && !ship->fire_special_weapon && ship->visible) {
                    if (ship->energy < 1.0)
                        ship->energy += 0.0014;
                    else if (ship->energy > 1.0)
                        ship->energy = 1.0;
                    if (ship->criticals && rand () % 10 == 0)
                        ship_critical (ship, 1);
                }
                /* Straighten the ship */
                if (lev_level.solid[Round(newx)][Round(newy) + 1] == TER_WATER) {
                    ship->angle = M_PI;
                } else {
                    ship->angle = 0;
                }
            } else {
                ship->onbase = 0;
            }
            newx = ship->x;
            newy = ship->y;
        } else {
            if (hit_solid (Round(ship->x), Round(ship->y)) < 0) {     /* Splash when coming out of the water */
                ship_splash (ship);
            }
            ship->onbase = 0;
        }
        /* Ship collisions */
        if (game_settings.ship_collisions && !ship->ghost && ship->dead == 0)
            tmplist = ship_list;
        while (tmplist) {
            struct Ship *tmpship=tmplist->data;
            if (tmpship == ship || tmpship->dead) {
                tmplist = tmplist->next;
                continue;
            }
            if (fabs (tmpship->x - newx) < 8
                && fabs (tmpship->y - newy) < 8) {
                tmpv = addVectors (&ship->vector, &tmpship->vector);
                tmpship->vector = oppositeVector (&tmpv);
                if (ship->darting) {
                    tmpship->health -= 0.4;
                    tmpship->white_ship = SHIP_WHITE_DUR;
                    if (tmpship->health <= 0)
                        killship (tmpship);
                    ship->darting = NODART;
                    ship->maxspeed = MAXSPEED;
                }
                break;
            }
            tmplist = tmplist->next;
        }
        ship->x = newx;
        ship->y = newy;
        /* Delete destroyed ships */
        if (ship->dead == 2) {
            int p;
            p = find_player (ship);
            if (p >= 0)
                players[p].ship = NULL;
            free(ship);
            if(current==ship_list)
                ship_list=dllist_remove(current);
            else
                dllist_remove(current);
        }
        current = next;
    }
}


static void ship_splash (struct Ship * ship)
{
    Projectile *p;
    double dx, dy;
    double a = 0, r;
    int x, y;
    r = 1.0 + hypot (ship->vector.x, ship->vector.y) / MAXSPEED * 2.0;
    while (a < 2.0 * M_PI) {
        dx = sin (a) * 1.0;
        dy = cos (a) * 1.0;
        x = ship->x;
        y = ship->y;
        p = make_projectile (x, y, ship->vector);
        p->primed = 1;
        p->vector.x = dx * r + ship->vector.x / 2.0;
        p->vector.y = dy * r + ship->vector.y / 2.0;
        p->type = Decor;
        p->color = lev_watercol;
        p->gravity = &gravity;
        add_projectile (p);
        a += 0.2;
    }
}

/* Ship turns gray and starts falling */
void killship (struct Ship * ship)
{
    /* Stop special weapons */
    ship_stop_special(ship);

    /* Kill the ship */
    ship->dead = 1;
    ship->frozen = 0;
    ship->health = 0;
    ship->maxspeed = MAXSPEED;
    ship->visible = 1;
    ship->thrust = 0;
    ship->fire_weapon = 0;
    /* Change the antigravity weapon to something else so the ship can fall */
    if (ship->special == WAntigrav)
        ship->special = WGrenade;

    /* This is borrowed to set which way the ship will start to spin */
    if (ship->angle < M_PI)
        ship->turn = 0.6;
    else
        ship->turn = -0.6;

    /* Sound effect */
    playwave(WAV_CRASH);
}

/* A random critical hit */
void ship_critical (struct Ship * ship, int repair)
{
    int c;
#if CRITICAL_COUNT > 8
#error Add more handlers for critical hits!
#endif
    c = rand () % CRITICAL_COUNT;
    switch (c) {
    case 0: /* Damaged engine */
        c = CRITICAL_ENGINE;
        break;
    case 1: /* Malfunctioning fuel control */
        c = CRITICAL_FUELCONTROL;
        break;
    case 2: /* Left thruster out of order */
        c = CRITICAL_LTHRUSTER;
        break;
    case 3: /* Right thruster out of order */
        c = CRITICAL_RTHRUSTER;
        break;
    case 4: /* Leaking cargo bay */
        c = CRITICAL_CARGO;
        break;
    case 5: /* Standard weapon out of order */
        c = CRITICAL_STDWEAPON;
        ship->fire_weapon = 0;
        break;
    case 6: /* Special weapon out of order */
        c = CRITICAL_SPECIAL;
        ship_stop_special(ship);
        break;
    case 7: /* Temporary power failure */
        c = CRITICAL_POWER;
        ship->no_power = 60;
        break;
    default:
        printf ("Bug! ship_critical(): unhandled critical hit!\n");
        return;
    }
    if ((ship->criticals & c) == 0) {
        int plr = find_player (ship);
        if (repair)
            return;
        if (c == CRITICAL_LTHRUSTER && (ship->criticals & CRITICAL_RTHRUSTER))
            return;           /* We won't leave the ship TOTALLY disabled... */
        if (c == CRITICAL_RTHRUSTER && (ship->criticals & CRITICAL_LTHRUSTER))
            return;
        if(c != CRITICAL_POWER) ship->criticals |= c;
        if (plr >= 0)
            set_player_message (plr, Bigfont, font_color_red, 25,
                                critical2str (c));
    } else if (repair) {
        int plr = find_player (ship);
        ship->criticals &= ~c;
        if (plr >= 0)
            set_player_message (plr, Bigfont, font_color_green, 25,
                                critical2str (c));
    }
}

void claim_ship (struct Ship * ship, int plr)
{
    ship->color = Red + plr;
    ship->ship = ship_gfx[ship->color];
    ship->shield = shield_gfx[plr];
    ship->standard = players[plr].standardWeapon;
    ship->special = players[plr].specialWeapon;
}

char ship_fire_special_weapon (struct Ship * ship)
{
    char fire = 0;
    switch (ship->special) {
    case WZapper:
        fire = 1;
        ship->energy -= 0.0075;
        if (ship->repeat_audio == 0) {
            playwave (WAV_ZAP);
            ship->repeat_audio = 9;
        } else
            ship->repeat_audio--;
        break;
    case WWatergun:
        if (hit_solid (ship->x, ship->y) < 0) {
            ship->energy += 0.02;
            if (ship->energy > 1)
                ship->energy = 1;
        } else
            fire = 1;
        break;
    case WFlame:
        if (hit_solid (ship->x, ship->y) >= 0) {
            fire = 1;
        }
        break;
    default:
        break;
    }
    if (ship->energy <= 0) {
        ship->fire_special_weapon = 0;
        ship->energy = 0;
        fire = 0;
    }
    return fire;
}

static void ship_exhaust (struct Ship * ship, Vector vec, signed char solid)
{
    Particle *part;
#ifdef VWING_STYLE_SMOKE
    Particle *part2, *part3, *part4;
    if (ship->vwing_exhaust <= 2) {
        ship->vwing_exhaust++;
        return;
    }
    ship->vwing_exhaust = 0;
    part =
        make_particle (ship->x + (sin (ship->angle - 0.58) * 5.0),
                       ship->y + (cos (ship->angle - 0.58) * 5.0), 5);
    part->vector.x = 0;
    part->vector.y = 0;
    part2 =
        make_particle (ship->x + (sin (ship->angle + 0.58) * 5.0),
                       ship->y + (cos (ship->angle + 0.58) * 5.0), 5);
    part2->vector = part->vector;
    part3 = make_particle (part->x, part->y, 2);
    part4 = make_particle (part2->x, part2->y, 2);
    part3->vector = ship->vector;
    part4->vector = ship->vector;
#else
    part = make_particle (ship->x, ship->y, 15);
    part->vector = oppositeVector (&vec);
#endif
    if (solid < 0) {
        part->color[0] = 100;
        part->color[1] = 100;
        part->color[2] = 255;
        //part->targ_color[0]=0; part->targ_color[1]=0; part->targ_color[2]=100;
        part->rd = -20;
        part->bd = -20;
        part->gd = -31;
        //calc_color_deltas(part);
        part->vector.x += 0.5 - ((rand () % 100) / 100.0);
        part->vector.y += 0.5 - ((rand () % 100) / 100.0);
    }
#ifdef VWING_STYLE_SMOKE
    else {
        part->color[0] = 255;
        part->color[1] = 255;
        part->color[2] = 255;
        if (ship->visible == 0) {
            part->color[0] = 25;
            part->color[1] = 25;
            part->color[2] = 25;
            part->color[3] = 50;
        }
        part->targ_color[0] = 255;
        part->targ_color[1] = 100;
        part->targ_color[2] = 100;
        part->targ_color[3] = 255;
        calc_color_deltas (part);
        memcpy (part2->color, part->color, 4);
        memcpy (part3->color, part->color, 4);
        memcpy (part4->color, part->color, 4);
        part2->rd = part->rd;
        part2->gd = part->gd;
        part2->bd = part->bd;
        part2->ad = part->ad;
        part3->rd = 0;
        part3->bd = 0;
        part3->gd = 0;
        part3->ad = 0;
        part4->rd = 0;
        part4->bd = 0;
        part4->gd = 0;
        part4->ad = 0;
    }
#endif
#ifdef VWING_STYLE_SMOKE
#endif
}

/* Player special features such as cloaking device */
static void ship_specials (struct Ship * ship) {
    if(ship->special == WRepair && ship->criticals && rand () % 32 == 0) {
            ship_critical (ship, 1);
    }
    if (ship->tagged) {
        ship_tagged (ship);
    }
    /* Remote control animation */
    if (ship->remote_control) {
        if ((int) ship->remote_control > 1) {
            ship->anim++;
            if (ship->anim >= REMOTE_FRAMES)
                ship->anim = 0;
            ship->energy -= 0.004;
            if (ship->energy <= 0)
                ship_fire (ship, SpecialWeapon);
        } else {
            if (ship->anim <= 0)
                ship->anim = REMOTE_FRAMES;
            ship->anim--;
        }
    }
    /* The rest are mutually exclusive (one player can have only one special weapon */
    /* Cloaking device */
    if (ship->visible == 0) {
#ifdef CHEAT_POSSIBLE
        if (!cheat1)
#endif
            ship->energy -= 0.001;
        if (ship->energy <= 0) {
            ship->visible = 1;
            ship->energy = 0;
        }
    }
    /* Ghost mode */
    else if (ship->ghost == 1) {
#ifdef CHEAT_POSSIBLE
        if (!cheat1)
#endif
            ship->energy -= 0.002;
        if (ship->energy <= 0) {
            ship->ghost = 0;
            ghostify (ship, 0);
            ship->energy = 0;
        }
    }
    /* Shield */
    else if (ship->shieldup == 1) {
#ifdef CHEAT_POSSIBLE
        if (!cheat1)
#endif
            ship->energy -= 0.005;
        if (ship->energy <= 0) {
            ship->shieldup = 0;
            ship->energy = 0;
        }
    }
    /* Afterburner */
    else if (ship->afterburn > 1) {
        if (ship->repeat_audio == 0) {
            playwave (WAV_BURN);
            ship->repeat_audio = 15;
        } else
            ship->repeat_audio--;
#ifdef CHEAT_POSSIBLE
        if (!cheat1)
#endif
            ship->energy -= 0.002;
        if (ship->energy <= 0) {
            ship->afterburn = 1;
            ship->maxspeed = MAXSPEED;
            ship->energy = 0;
        }
    } else if (ship->antigrav) {
#ifdef CHEAT_POSSIBLE
        if (!cheat1)
#endif
            ship->energy -= 0.001;
        if (ship->energy <= 0) {
            ship->antigrav = 0;
            ship->energy = 0;
        }
    }
    /* Autorepair */
    else if (ship->repairing) {
#ifdef CHEAT_POSSIBLE
        if (!cheat1)
#endif
            ship->energy -= 0.001;
        ship->health += 0.0008;
        if (ship->health >= 1) {
            ship->health = 1;
            ship->repairing = 0;
        }
        if (ship->energy <= 0) {
            ship->energy = 0;
            ship->repairing = 0;
            ship->energy = 0;
        }
        if (ship->anim == 0) {
            Particle *spark;
            ship->anim = rand () % 15;
            spark =
                make_particle (ship->x + (rand () % 16) - 8,
                               ship->y + (rand () % 16) - 8, 15);
            spark->color[0] = 255;
            spark->color[1] = 255;
            spark->color[2] = 0;
            spark->rd = -7;
            spark->gd = -17;
            spark->bd = 0;
            spark->vector.x = (rand () % 6) - 3;
            spark->vector.y = -2;
        } else
            ship->anim = 0;
    }
}

static void ship_tagged (struct Ship * ship)
{
    Particle *smoke;
    smoke = make_particle (ship->x, ship->y, 255);
    smoke->vector.x = 0;
    smoke->vector.y = 0;
    switch (ship->color) {
    case Red:
        smoke->color[0] = 255;
        smoke->color[1] = 0;
        smoke->color[2] = 0;
        break;
    case Blue:
        smoke->color[0] = 0;
        smoke->color[1] = 0;
        smoke->color[2] = 255;
        break;
    case Green:
        smoke->color[0] = 0;
        smoke->color[1] = 255;
        smoke->color[2] = 0;
        break;
    case Yellow:
        smoke->color[0] = 255;
        smoke->color[1] = 255;
        smoke->color[2] = 0;
        break;
    default:
        break;
    }
#ifdef HAVE_LIBSDL_GFX
    smoke->rd = 0;
    smoke->bd = 0;
    smoke->gd = 0;
    smoke->ad = -1;
#else
    smoke->targ_color[0] = 0;
    smoke->targ_color[1] = 0;
    smoke->targ_color[2] = 0;
    calc_color_deltas (smoke);
#endif
}

/* Enable/disable ghost ship effect */
void ghostify (struct Ship * ship, char status)
{
    if (status)
        ship->ship = ghost_gfx[ship->color - 1];
    else
        ship->ship = ship_gfx[ship->color];
}

/* Check if there is a ship in the specified coordinates and bump
 * it one pixel upwards if there is. This is used to keep ships
 * from getting stuck in regenerating bases */
void bump_ship(int x,int y) {
    struct dllist *shplst=ship_list;
    while(shplst) {
        struct Ship *ship=shplst->data;

        if(Round(ship->x) == x && Round(ship->y-1) == y) {
            ship->y--;
            break;
        }
        shplst=shplst->next;
    }
}

/* Cloaking device engage/disengage effect */
static void cloaking_device (struct Ship * ship, char status)
{
    SDL_Surface *surf;
    Particle *part;
    Uint32 *src;
    int x, y, p;
    p = Roundplus ((35 - ship->angle / (2.0 * M_PI / 35.0)));
    if (p > 35)
        p = 35;
    surf = ship->ship[p];
    src = surf->pixels;
    for (y = -surf->h / 2; y < surf->h / 2; y++)
        for (x = -surf->w / 2; x < surf->w / 2; x++) {
            if (((*src & surf->format->Amask) >> surf->format->Ashift) > 30) {
                if (status) {
                    part = make_particle (ship->x + x, ship->y + y, 8);
                    part->color[0] =
                        (*src & surf->format->Rmask) >> surf->format->Rshift;
                    part->color[1] =
                        (*src & surf->format->Gmask) >> surf->format->Gshift;
                    part->color[2] =
                        (*src & surf->format->Bmask) >> surf->format->Bshift;
                    part->color[3] =
                        (*src & surf->format->Amask) >> surf->format->Ashift;
#ifdef HAVE_LIBSDL_GFX
                    part->rd = 0;
                    part->gd = 0;
                    part->bd = 0;
                    part->ad = -part->color[3] >> 3;
#else
                    part->rd = -part->color[0] >> 3;
                    part->gd = -part->color[1] >> 3;
                    part->bd = -part->color[2] >> 3;
#endif

                    part->vector.x =
                        ship->vector.x + 1.0 - ((rand () % 20) / 10.0);
                    part->vector.y =
                        ship->vector.y + 1.0 - ((rand () % 20) / 10.0);
                } else {
                    part = make_particle (ship->x + x, ship->y + y, 3);
                    memset (part->color, 0, 4);
                    part->targ_color[0] =
                        (*src & surf->format->Rmask) >> surf->format->Rshift;
                    part->targ_color[1] =
                        (*src & surf->format->Gmask) >> surf->format->Gshift;
                    part->targ_color[2] =
                        (*src & surf->format->Bmask) >> surf->format->Bshift;
                    part->targ_color[3] =
                        (*src & surf->format->Amask) >> surf->format->Ashift;
                    calc_color_deltas (part);
                    part->vector = ship->vector;
                }
            }
            src++;
        }
}

void ship_fire (struct Ship * ship, WeaponClass weapon) {
    Projectile *p;
    Vector v;
    AudioSample sfx;
    char playsound = 1;
    Vector ps;
    /* First of all, check if we need any projectiles at all */
    if (weapon == SpecialWeapon) {
        if (ship->special_cooloff)
            return;
        if (ship_fire_special_noproj (ship))
            return;
        if (ship->special == WWatergun)
            return;
    } else if (ship->cooloff) {
        return;
    }
    /* Create a standard projectile */
    sfx = WAV_NORMALWEAP;
    v = makeVector (sin (ship->angle) * 9.0, cos (ship->angle) * 9.0);
    p = make_projectile (ship->x-v.x/4.0, ship->y-v.y/4.0, v);
    p->owner = ship;
    ps.x = ship->vector.x;
    ps.y = ship->vector.y;
    if (weapon == NormalWeapon) {       /* Standard weapon */
        ship_fire_standard_weapon (ship, p, v, ps);
    } else if (weapon == SpecialWeapon) {       /* Special weapon */
        sfx = WAV_SPECIALWEAP;
        if (ship->energy <= 0.1) {
            free (p);
            return;
        }
        p->vector.x = v.x + ps.x;
        p->vector.y = v.y + ps.y;
        switch (ship->special) {
        case WGrenade:
            p->type = Grenade;
            ship->energy -= 0.09;
            p->color = col_grenade;
            ship->special_cooloff = 9;
            break;
        case WMegaBomb:
            p->type = MegaBomb;
            p->vector.x = ship->vector.x * 0.5;
            p->vector.y = 0;
            p->gravity = &strong_gravity;
            p->wind_affects = 0;
            p->maxspeed = 12;
            ship->energy -= 0.25;
            ship->special_cooloff = 5;
            break;
        case WEmber:
            p->type = Ember;
            p->vector.x = ship->vector.x * 0.4;
            p->vector.y = 0;
            p->var1 = 2;
            p->life = EMBER_LIFE;
            p->gravity = &really_weak_gravity;
            ship->energy -= 0.15;
            ship->special_cooloff = 10;
            break;
        case WMissile:
            p->type = Missile;
            p->color = col_gray;
            p->angle = ship->angle + M_PI;
            p->gravity = NULL;
            p->ownerteam = find_player(ship);
            if(p->ownerteam>=0)
                p->ownerteam=player_teams[p->ownerteam];
            else
                p->ownerteam=0;
            ship->energy -= 0.18;
            ship->special_cooloff = 11;
            sfx = WAV_MISSILE;
            break;
        case WSpeargun:
            p->vector.x *= 1.2;
            p->vector.y *= 1.2;
            p->type = Spear;
            p->color = col_gray;
            p->angle = ship->angle;
            ship->energy -= 0.08;
            ship->special_cooloff = 6;
            break;
        case WMine:
            if (hit_solid (ship->x, ship->y) < 0) {
                free (p);
                return;
            }
            p->type = Mine;
            p->vector.x = 0;
            p->vector.y = 0;
            p->owner = NULL;
            p->primed = 19;
            p->color = col_gray;
            p->gravity = NULL;
            p->wind_affects = 0;
            ship->energy -= 0.06;
            ship->special_cooloff = 9;
            break;
        case WMagMine:
            if (hit_solid (ship->x, ship->y) < 0) {
                free (p);
                return;
            }
            p->type = MagMine;
            p->vector.x = sin (ship->angle) * -2.0;
            p->vector.y = cos (ship->angle) * -2.0;
            p->color = map_rgba(149,158,255,255);
            p->gravity = NULL;
            p->wind_affects = 0;
            ship->energy -= 0.06;
            p->owner = NULL;
            p->primed = 20;
            ship->special_cooloff = 7;
            break;
        case WDivide:
            if (hit_solid (ship->x, ship->y) < 0) {
                free (p);
                return;
            }
            p->type = DividingMine;
            p->vector.x = sin (ship->angle) * -2.0;
            p->vector.y = cos (ship->angle) * -2.0;
            p->color = col_green;
            p->gravity = NULL;
            p->wind_affects = 0;
            p->owner = NULL;
            p->primed = 20;
            p->var1 = DIVIDINGMINE_INTERVAL;
            ship->special_cooloff = 10;
            ship->energy -= 0.35;
            break;
        case WClaybomb:
            p->type = Claybomb;
            p->color = col_clay;
            ship->energy -= 0.125;
            ship->special_cooloff = 5;
            break;
        case WSnowball:
            p->type = Snowball;
            p->color = col_snow;
            ship->energy -= 0.09;
            ship->special_cooloff = 7;
            break;
        case WPlastique:
            p->type = Plastique;
            p->color = col_gray;
            p->vector.x = -p->vector.x;
            p->vector.y = -p->vector.y;
            p->life = 5;
            p->gravity = NULL;
            p->wind_affects = 0;
            ship->energy -= 0.06;
            ship->special_cooloff = 5;
            break;
        case WLandmine:
            p->type = Landmine;
            p->color = 0;
            p->vector.x = -p->vector.x;
            p->vector.y = -p->vector.y;
            p->life = 5;
            p->gravity = NULL;
            p->wind_affects = 0;
            p->angle = ship->angle;
            ship->energy -= 0.11;
            ship->special_cooloff = 5;
            break;
        case WGravGun:
            p->type = GravityWell;
            p->color = col_gray;
            p->gravity = NULL;
            p->wind_affects = 0;
            p->primed = 25;
            p->vector =
                makeVector (sin (ship->angle) * -1.2,
                            cos (ship->angle) * -1.2);
            ship->energy -= 0.14;
            ship->special_cooloff = 15;
            p->life = 1000;
            break;
        case WGravMine:
            p->type = GravityWell;
            p->color = col_gray;
            p->gravity = NULL;
            p->wind_affects = 0;
            p->vector.x = 0;
            p->vector.y = 0;
            p->life = 500;
            p->primed = 15;
            ship->energy -= 0.11;
            ship->special_cooloff = 25;
            break;
        case WShotgun:
            {
                double f;
                Projectile *proj;
                for (f = ship->angle - 0.25; f < ship->angle + 0.25;
                     f += 0.08) {
                    proj =
                        make_projectile (p->x, p->y,
                                         makeVector (sin (f) * 1.5,
                                                     cos (f) * 1.5));
                    proj->owner = p->owner;
                    add_projectile (proj);
                }
            }
            ship->vector.x -= sin (ship->angle) * 5.0;  /* Recoil */
            ship->vector.y -= cos (ship->angle) * 5.0;
            ship->energy -= 0.11;
            ship->special_cooloff = 15;
            break;
        case WRocket:
            p->type = Rocket;
            p->color = col_gray;
            p->angle = ship->angle + M_PI;
            p->vector.x /= 3.0;
            p->vector.y /= 3.0;
            p->gravity = NULL;
            p->primed = 15;
            p->owner = NULL;
            ship->energy -= 0.18;
            ship->special_cooloff = 10;
            sfx = WAV_MISSILE;
            break;
        case WMirv:
            p->type = Mirv;
            p->color = col_gray;
            p->angle = ship->angle + M_PI;
            p->vector.x /= 2.2;
            p->vector.y /= 2.2;
            p->gravity = NULL;
            p->life = MIRV_LIFE;
            p->var1 = 3;
            ship->energy -= 0.28;
            ship->special_cooloff = 5;
            sfx = WAV_MISSILE;
            break;
        case WEnergy:
            {
                Particle *part1, *part2;
                p->type = Energy;
                p->color = col_yellow;
                p->angle = ship->angle;
                p->gravity = NULL;
                p->wind_affects = 0;
                part1 = make_particle (ship->x, ship->y, 20);
                part1->vector =
                    makeVector (sin (ship->angle + M_PI_2) * 2+ship->vector.x,
                                cos (ship->angle + M_PI_2) * 2+ship->vector.y);
                part1->color[0] = 255;
                part1->color[1] = 255;
                part1->color[2] = 0;
                part1->targ_color[0] = 100;
                part1->targ_color[1] = 100;
                part1->targ_color[2] = 0;
                calc_color_deltas (part1);
                part2 = make_particle (ship->x, ship->y, 20);
                part2->vector =
                    makeVector (sin (ship->angle - M_PI_2) * 2+ship->vector.x,
                                cos (ship->angle - M_PI_2) * 2+ship->vector.y);
                memcpy (part2->color, part1->color, 3);
                part2->rd = part1->rd;
                part2->gd = part1->gd;
                part2->bd = part1->bd;
                sfx = WAV_LASER;
            }
            p->vector =
                makeVector (sin (ship->angle) * 9.0, cos (ship->angle) * 9.0);

            ship->special_cooloff = 4;
            ship->energy -= 0.01;
            break;
        case WBoomerang:
            p->type = Boomerang;
            p->color = col_yellow;
            p->var1 = 0;
            p->angle = ship->angle - M_PI;
            p->primed = 5;
            ship->special_cooloff = 6;
            ship->energy -= 0.06;
            break;
        case WTag:
            p->type = Tag;
            p->color = col_red;
            p->life = 600;
            p->var1 = 0;
            ship->special_cooloff = 6;
            ship->energy -= 0.05;
            break;
        case WMush:
            p->type = Mush;
            p->color = col_yellow;
            p->var1 = 0;
            ship->special_cooloff = 16;
            ship->energy -= 0.09;
            break;
        case WAcid:
            p->type = Acid;
            p->color = col_green;
            ship->special_cooloff = 10;
            ship->energy -= 0.1;
            break;
        case WEMP:{
                double a;
                p->type = MagWave;
                p->color = col_blue;
                p->life = 13;
                p->wind_affects = 0;
                p->gravity = NULL;
                p->vector.x = ship->vector.x;
                p->vector.y = ship->vector.y + 6.0;
                ship->special_cooloff = 30;
                ship->energy -= 0.3;
                ship->no_power = 60;
                for (a = 0.1; a < 2 * M_PI; a += 0.1) {
                    Projectile *newp;
                    newp = malloc (sizeof (Projectile));
                    memcpy (newp, p, sizeof (Projectile));
                    newp->vector.x = sin (a) * 4.5 + ship->vector.x;
                    newp->vector.y = cos (a) * 4.5 + ship->vector.y;
                    add_projectile (newp);
                }
            }
            break;
        default:
            break;
        }
    } else if (weapon == SpecialWeapon2) {      /* We got ourselves a /really/ special weapon ! */
        switch (ship->special) {
        case WZapper:{
                double r;
                struct Ship *i;
                p->type = Zap;
                p->wind_affects = 0;
                p->vector.x = 0;
                p->vector.y = 0;
                p->life = 3;
                i = find_nearest_ship (p->x, p->y, ship, &r);
                if (r < 45) {
                    p->x = i->x;
                    p->y = i->y;
                } else {
                    r = (rand () % 628) / 100.0;
                    p->x += Round (cos (r) * 25);
                    p->y += Round (sin (r) * 25);
                }
                playsound = 0;
            }
            break;
        case WWatergun:{
                double ra;
                ra = 0.3 - (rand () % 6) / 10.0;
                p->type = Waterjet;
                v = makeVector (sin (ship->angle + ra),
                                cos (ship->angle + ra));
                p->vector.x = v.x * 4.5 + ps.x;
                p->vector.y = v.y * 4.5 + ps.y;
                p->life = 40;
                p->color = lev_watercol;
                ship->energy -= 0.01;
            }
            playsound = 0;
            break;
        case WFlame:{
                double ra;
                ra = 0.3 - (rand () % 4) / 10.0;
                p->type = Napalm;
                v = makeVector (sin (ship->angle + ra),
                                cos (ship->angle + ra));
                p->vector.x = v.x * 4.5 + ps.x;
                p->vector.y = v.y * 4.5 + ps.y;
                p->life = 15;
                p->color = col_white;
                ship->energy -= 0.006;
            }
            playsound = 0;
            break;
        default:
            break;
        }
    }
    if (ship->energy < 0)
        ship->energy = 0;
    add_projectile (p);
    if (playsound)
        playwave (sfx);
#ifdef CHEAT_POSSIBLE
    if (cheat1)
        ship->energy = 1.0;
#endif
}

/* Fire a special weapon that doesn't require projectiles */
static char ship_fire_special_noproj (struct Ship * ship)
{
    switch (ship->special) {
    case WCloak:
        if (ship->visible == 0) {
            ship->visible = -3;
            cloaking_device (ship, 0);
        } else if (ship->energy > 0) {
            ship->visible = 0;
            cloaking_device (ship, 1);
        }
        return 1;
    case WShield:
        if (ship->shieldup == 1)
            ship->shieldup = 0;
        else if (ship->energy > 0)
            ship->shieldup = 1;
        return 1;
    case WAntigrav:
        if (ship->antigrav)
            ship->antigrav = 0;
        else if (ship->energy > 0)
            ship->antigrav = 1;
        return 1;
    case WGhostShip:
        if (ship->ghost == 1) {
            ship->ghost = 0;
            ghostify (ship, 0);
        } else if (ship->energy > 0) {
            ship->ghost = 1;
            ghostify (ship, 1);
        }
        return 1;
    case WAfterBurner:
        ship->repeat_audio = 0;
        if (ship->afterburn > 1) {
            ship->afterburn = 1;
            ship->maxspeed = MAXSPEED;
        } else if (ship->energy > 0) {
            ship->afterburn = 2;
            ship->maxspeed = MAXSPEED * 2;
            spawn_clusters (ship->x, ship->y, 5, FireStarter);
        }
        return 1;
    case WWarp:
        if (ship->energy >= 0.1) {
            drop_jumppoint (ship->x, ship->y, find_player (ship));
            ship->energy -= 0.1;
            if (ship->energy < 0)
                ship->energy = 0;
            ship->special_cooloff = 10;
        }
        return 1;
    case WDart:
        if (ship->energy >= 0.1 && ship->darting == NODART) {
            ship->darting = DARTING;
            ship->turn = 0;
            ship->energy -= 0.1;
            ship->maxspeed = 10;
            ship->special_cooloff = 20;
            ship->fire_special_weapon = 0;
            ship->fire_weapon = 0;
            if (ship->energy < 0)
                ship->energy = 0;
            playwave (WAV_DART);
        }
        return 1;
    case WLandmine:
        if (detonate_remote (ship))
            return 1;
        break;
    case WRepair:
        if (ship->repairing == 0 && ship->energy > 0 && ship->health < 1)
            ship->repairing = 1;
        else if (ship->repairing == 1)
            ship->repairing = 0;
        ship->anim = 0;
        return 1;
    case WInfantry:
        if (ship->energy >= 0.05) {
            add_critter (make_critter
                         (Infantry, ship->x, ship->y, find_player (ship)));
            ship->energy -= 0.05;
            ship->special_cooloff = 5;
            if (ship->energy < 0)
                ship->energy = 0;
        }
        return 1;
    case WHelicopter:
        if (ship->energy >= 0.25
            && lev_level.solid[Round(ship->x)][Round(ship->y)] == TER_FREE) {
            add_critter (make_critter
                         (Helicopter, ship->x, ship->y, find_player (ship)));
            ship->energy -= 0.25;
            ship->special_cooloff = 15;
            if (ship->energy < 0)
                ship->energy = 0;
        }
        return 1;
    case WRemote:
        if (ship->remote_control) {     /* Deactive remote control */
            ship->remote_control->remote_control = NULL;
            ship->remote_control->thrust = 0;
            ship->remote_control->turn = 0;
            ship->remote_control->fire_weapon = 0;
            ship_stop_special(ship->remote_control);
            ship->remote_control = NULL;
        } else if (ship->energy > 0) {  /* Active remote control */
            struct Ship *targ;
            double d;
            targ = find_nearest_ship (ship->x, ship->y, ship, &d);
            if (targ && d < 150) {
                targ->thrust = 0;
                targ->turn = 0;
                targ->fire_weapon = 0;
                ship->thrust = 0;
                ship->turn = 0;
                ship->fire_weapon = 0;
                ship_stop_special(targ);
                ship->remote_control = targ;
                targ->remote_control = (struct Ship *) 1; /* To indicate that the ship is being remote controlled */
            }
        }
        return 1;
    default:
        return 0;
    }
    return 0;
}

/* Fire the normal weapon */
static void ship_fire_standard_weapon (struct Ship * ship, Projectile * p, Vector v,
                                       Vector ps)
{
    switch (ship->standard) {
    case SShot:
        /* Make the bullet velocity be dependent on the speed of the ship */
        p->vector.x = v.x + ps.x;
        p->vector.y = v.y + ps.y;
        ship->cooloff = 3;
        break;
    case S3ShotWide:
        {
            Projectile *proj;
            Vector vec;

            /* Calculate a vector for the new projectile, making it at an angle */
            vec =
                makeVector (sin (ship->angle - 0.10) * 10,
                            cos (ship->angle - 0.10) * 10);
            proj = make_projectile (p->x, p->y, vec);
            proj->owner = p->owner;
            /* Make the bullet velocity be dependent on the speed of the ship */
            proj->vector.x = vec.x + ps.x;
            proj->vector.y = vec.y + ps.y;
            add_projectile (proj);

            /* Calculate a vector for the new projectile, making it at an angle */
            vec =
                makeVector (sin (ship->angle + 0.10) * 10,
                            cos (ship->angle + 0.10) * 10);
            /* Make the bullet velocity be dependent on the speed of the ship */
            proj = make_projectile (p->x, p->y, vec);
            proj->owner = p->owner;
            proj->vector.x = vec.x + ps.x;
            proj->vector.y = vec.y + ps.y;
            add_projectile (proj);

            /* Make the bullet velocity be dependent on the speed of the ship */
            p->vector.x = v.x + ps.x;
            p->vector.y = v.y + ps.y;
            ship->cooloff = 10;
            break;
        }
    case S4Way:
        {
            Projectile *proj;
            Vector vec;
            p->vector.x = v.x + ps.x;
            p->vector.y = v.y + ps.y;

            proj = make_projectile (p->x, p->y, p->vector);
            proj->vector.x = -v.x + ps.x;
            proj->vector.y = -v.y + ps.y;
            proj->owner = p->owner;
            add_projectile (proj);

            vec =
                makeVector (sin (ship->angle + M_PI_2) * 10.0,
                            cos (ship->angle + M_PI_2) * 10.0);
            proj = make_projectile (p->x, p->y, vec);
            proj->vector.x = vec.x + ps.x;
            proj->vector.y = vec.y + ps.y;
            proj->owner = p->owner;
            add_projectile (proj);

            proj = make_projectile (p->x, p->y, vec);
            proj->vector.x = -vec.x + ps.x;
            proj->vector.y = -vec.y + ps.y;
            proj->owner = p->owner;
            add_projectile (proj);

            ship->cooloff = 7;
        }
        break;

    case S3ShotTight:
        {
            Projectile *proj;
            Vector vec;

            /* Make a displacement vector */
            vec =
                makeVector (sin (ship->angle - 0.50) * 1.5,
                            cos (ship->angle - 0.50) * 1.5);
            /* Move the projectile's start (x,y) based on the displacement vector */
            proj =
                make_projectile (p->x + (4 * vec.x), p->y + (4 * vec.y), v);
            proj->owner = p->owner;
            /* Make the bullet velocity be dependent on the speed of the ship */
            proj->vector.x = v.x + ps.x;
            proj->vector.y = v.y + ps.y;
            add_projectile (proj);

            /* Make a displacement vector */
            vec =
                makeVector (sin (ship->angle + 0.50) * 1.5,
                            cos (ship->angle + 0.50) * 1.5);
            /* Move the projectile's start (x,y) based on the displacement vector */
            proj =
                make_projectile (p->x + (4 * vec.x), p->y + (4 * vec.y), v);
            proj->owner = p->owner;
            /* Make the bullet velocity be dependent on the speed of the ship */
            proj->vector.x = v.x + ps.x;
            proj->vector.y = v.y + ps.y;
            add_projectile (proj);

            /* Make the bullet velocity be dependent on the speed of the ship */
            p->vector.x = v.x + ship->vector.x;
            p->vector.y = v.y + ship->vector.y;
            ship->cooloff = 11;
            break;
        }

    case SSweep:
        {
            Vector vec;

            /* Calculate the new sweep angle and generate a vector for it */
            if ((ship->sweep_angle >= -0.40) && (ship->sweeping_down)) {
                ship->sweep_angle -= 0.1;
                vec =
                    makeVector (sin (ship->angle + ship->sweep_angle) * 10.0,
                                cos (ship->angle + ship->sweep_angle) * 10.0);
                if (ship->sweep_angle <= -0.40)
                    ship->sweeping_down = 0;
            } else if (ship->sweep_angle <= 0.40) {
                ship->sweep_angle += 0.1;
                vec =
                    makeVector (sin (ship->angle + ship->sweep_angle) * 10.0,
                                cos (ship->angle + ship->sweep_angle) * 10.0);
                if (ship->sweep_angle >= 0.40)
                    ship->sweeping_down = 1;
            }

            /* Make the bullet velocity be dependent on the speed of the ship */
            /* and change its direction to that of the newly calculated sweep vector */
            p->vector.x = vec.x + ps.x;
            p->vector.y = vec.y + ps.y;
            ship->cooloff = 3;
            break;
        }
    }
}

/* Find the nearest enemy player (in a ship) */
int find_nearest_player (int myX, int myY, int not_this, double *dist) {
    int p, plr = -1;
    double distance = 9999999, d;
    for (p = 0; p < 4; p++) {
        if (not_this >= 0)
            if (player_teams[p] == player_teams[not_this])
                continue;
        if (players[p].ship==NULL || players[p].ship->dead) continue;
        d = hypot (abs (players[p].ship->x - myX),
                   abs (players[p].ship->y - myY));
        if (d < distance) {
            plr = p;
            distance = d;
        }
    }
    if (dist)
        *dist = distance;
    return plr;
}

/* Note. This is a bit different that find_nearest_player       */
/* First of all, this returns a pointer to the ship directly    */
/* Second, not_this is a pointer to a ship, not a team number */
struct Ship *find_nearest_ship (int myX, int myY, struct Ship * not_this, double *dist)
{
    struct dllist *current = ship_list;
    struct Ship *nearest = NULL;
    double distance = 9999999, d;
    while (current) {
        struct Ship *ship=current->data;
        if (ship->dead || ship == not_this) {
            current = current->next;
            continue;
        }
        d = hypot (abs (ship->x - myX),
                   abs (ship->y - myY));
        if (d < distance) {
            nearest = ship;
            distance = d;
        }
        current = current->next;
    }
    if (dist)
        *dist = distance;
    return nearest;
}

struct Ship *hit_ship(double x, double y, struct Ship *ignore, double radius) {
    struct dllist *current = ship_list;
    while (current) {
        struct Ship *ship=current->data;
        if (ship->dead == 0 && ship!=ignore) {
            if(hypot(ship->x-x,ship->y-y)<=radius)
                    return ship;
        }
        current = current->next;
    }
    return NULL;
}

signed char ship_damage (struct Ship * ship, Projectile * proj)
{
    double damage;
    signed char rval = 1;
    int critical = 100;
    if (ship->shieldup)
        return 1;
    switch (proj->type) {
    case Missile:
        damage = 0.02;
        critical = 5;
        break;
    case Grenade:
    case Mine:
    case MagMine:
        damage = 0;
        critical = 15;
        break;
    case Snowball:
        ship->frozen = 1;
        ship->thrust = 0;
        ship->turn = 0;
        ship->fire_weapon = 0;
        ship->fire_special_weapon = 0;
        ship->afterburn = 1;
        ship->visible = 1;
        ship->ghost = 0;
        return -1;
        break;
    case Handgun:
        damage = 0.02;
        break;
    case Spear:
        ship->vector = proj->vector;
        ship->maxspeed = 10;
        ship->darting = SPEARED;
        ship->fire_weapon = 0;
        ship->fire_special_weapon = 0;
        ship->turn=0;
        damage = 0.01;
        rval = -1;
        critical = 4;
    case Zap:
        damage = 0.0075;
        rval = -1;
        break;
    case Energy:
        damage = 0.02;
        rval = -1;
        break;
    case Tag:
        damage = 0.001;
        ship->tagged = proj->life;
        rval = -1;
        critical = 200;
        break;
    case Waterjet:
        ship->vector.x += proj->vector.x;
        ship->vector.y += proj->vector.y;
        ship->angle += 0.7;
        damage = 0.0;
        critical = 500;
        return -1;
    case Napalm:
        damage = 0.0005;
        rval = -1;
        critical = 1000;
        break;
    case Acid:
        if (proj->var1) {
            damage = 0.002;
            break;
        }
        proj->var1 = 1;
        proj->life = 150;
        critical = 1000;
        return 0;
    case Boomerang:
        damage = 0.03;
        if (proj->owner == ship) {
            damage = 0;
#ifdef CHEAT_POSSIBLE
            if (cheat1 == 0)
#endif
                ship->energy += 0.06;
            return -2;
        }
        break;
    case MagWave:
        damage = 0.0;
        critical = 2;
        rval = -1;
        break;
    default:
        damage = 0.03;
    }
    ship->health -= damage;
    if (damage)
        ship->white_ship = SHIP_WHITE_DUR;
    if (proj->type != Energy && proj->type != Napalm && proj->type != Acid
        && proj->type != MagWave) {
        ship->vector.x += proj->vector.x / 2;
        ship->vector.y += proj->vector.y / 2;
    }
    if (game_settings.criticals || proj->type == MagWave) {
        if (rand () % critical == 0)
            ship_critical (ship, 0);
    }
    if (ship->health <= 0)
        killship (ship);
    return rval;
}

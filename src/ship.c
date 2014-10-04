/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
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

#include "console.h"
#include "player.h"
#include "level.h"
#include "list.h"
#include "ldat.h"
#include "fs.h"
#include "decor.h"
#include "animation.h"
#include "particle.h"
#include "audio.h"
#include "special.h"
#include "critter.h"
#include "levelfile.h"
#include "ship.h"
#include "weapon.h"

#define SHIP_POSES      36
#define SHIP_WHITE_DUR	(0.13*GAME_SPEED)   /* After receiving damage, for how long the ship appears white */
#define THRUST          (80.0/GAME_SPEED)
#define DAMAGE_TRESHOLD 3.0 /* Treshold velocity for collision damage */


/* Exported globals */
struct dllist *ship_list;

/* Internally used globals */
static SDL_Surface *ship_gfx[7][SHIP_POSES]; /* 0=grey, 1-4=coloured, 5 = white, 6 =  frozen */
static SDL_Surface *ghost_gfx[4][SHIP_POSES];
static SDL_Surface *shield_gfx[4];      /* 0-3 coloured */
static SDL_Surface **remocon_gfx;
static int remocon_frames;

/* Internally used functions */
static void ship_fire_standard_weapon (struct Ship * ship);
static void ship_specials (struct Ship * ship);

/* Load ship related datafiles */
void init_ships (LDAT *playerfile) {
    SDL_Surface *tmpsurface;
    int r, p;
    /* Load ship graphics */
    for (p = 0; p < SHIP_POSES; p++) {
        tmpsurface = load_image_ldat (playerfile, 0, T_ALPHA,"VWING",p);
        for (r = 0; r < 7; r++) {
            ship_gfx[r][p] = copy_surface(tmpsurface);
            switch (r) {
            case Grey:
                recolor (ship_gfx[r][p], 0.6, 0.6, 0.6, 1);
                break;
            case Frozen:
                recolor (ship_gfx[r][p], 0, 1, 1, 1);
                break;
            case White:
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
        SDL_FreeSurface(tmpsurface);
    }
    /* Create ghost ship graphics */
    for (r = 0; r < 4; r++) {
        for (p = 0; p < SHIP_POSES; p++) {
            ghost_gfx[r][p] = copy_surface (ship_gfx[r + 1][p]);
            recolor (ghost_gfx[r][p], 1.0, 1.0, 1.0, 0.5);
        }
    }
    /* Load Shield graphics */
    tmpsurface = load_image_ldat (playerfile, 0, T_ALPHA, "SHIELD", 0);
    for (r = 0; r < 4; r++) {
        shield_gfx[r] = copy_surface (tmpsurface);
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
            fputs("Unhandled shield color in init_ships()\n",stderr);
            exit(1);
        }
    }
    SDL_FreeSurface(tmpsurface);
    /* Load Remote Control graphics */
    remocon_gfx =
        load_image_array (playerfile, 0, T_ALPHA, "XMIT", &remocon_frames);
}

/* Remove ships */
void clear_ships (void)
{
    dllist_free(ship_list,free);
    ship_list=NULL;
}

/* Prepare for a new level */
void reinit_ships (struct LevelSettings * settings)
{
    struct dllist *objects=NULL;
    int standard,special;
    PlayerColor color;
    struct Ship *newship;
    if (settings)
        objects = settings->objects;
    while (objects) {
        struct LSB_Object *object = objects->data;
        if (object->type == OBJ_SHIP) {
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
                standard = 0;
                special = 0;
                break;
            }
            if (color != Grey) {
                standard = players[color - 1].standardWeapon;
                special = players[color - 1].specialWeapon;
            } else {
                standard = 0;
                special = 0;
            }
            newship = create_ship (color, standard, special);
            newship->physics.x = object->x;
            newship->physics.y = object->y;
        }
        objects = objects->next;
    }
}

/* Create a new ship. It is automatically added to the ship list */
struct Ship *create_ship (PlayerColor color, int weapon, int special)
{
    Vector nulv = {0,0};
    struct Ship *newship = malloc (sizeof (struct Ship));
    if(!newship) {
        perror("create_ship");
        return NULL;
    }
    memset (newship, 0, sizeof (struct Ship));
    newship->ship = ship_gfx[color];
    newship->shield = shield_gfx[color - Red];
    init_physobj(&newship->physics,0,0,nulv);
    newship->physics.sharpness = BOUNCY;
    newship->physics.mass = 8.0;
    newship->physics.radius = 6.0;
    newship->angle = M_PI_2;

    newship->health = 1.0;
    newship->energy = 1.0;
    newship->visible = 1;
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
        if (ship->visible==1) {
            for (plr = 0; plr < 4; plr++) {
                rect.w = ship_gfx[Grey][0]->w;
                rect.h = ship_gfx[Grey][0]->h;
                if (players[plr].state==ALIVE||players[plr].state==DEAD) {
                    SDL_Surface *surf;
                    rect.x =
                        viewport_rects[plr].x + Round(ship->physics.x) -
                        cam_rects[plr].x - 8;
                    rect.y =
                        viewport_rects[plr].y + Round(ship->physics.y) -
                        cam_rects[plr].y - 8;
                    if (rect.x < viewport_rects[plr].x - 16
                        || rect.y < viewport_rects[plr].y - 16
                        || rect.x > viewport_rects[plr].x + cam_rects[plr].w
                        || rect.y > viewport_rects[plr].y + cam_rects[plr].h)
                        continue;
                    rect2 =
                        cliprect (rect.x, rect.y, rect.w, rect.h,
                                  viewport_rects[plr].x, viewport_rects[plr].y,
                                  viewport_rects[plr].x + cam_rects[plr].w,
                                  viewport_rects[plr].y + cam_rects[plr].h);
                    if (rect.x < viewport_rects[plr].x)
                        rect.x = viewport_rects[plr].x;
                    if (rect.y < viewport_rects[plr].y)
                        rect.y = viewport_rects[plr].y;
                    if (players[plr].ship == ship && radars_visible)
                        draw_radar (rect, plr);
                    pose = Round(ship->angle/(2*M_PI)*SHIP_POSES);
                    if (pose > 35)
                        pose = 35;
                    if (ship->state!=INTACT)
                        surf = ship_gfx[Grey][pose];
                    else if (ship->frozen)
                        surf = ship_gfx[Frozen][pose];
                    else if (ship->white_ship)
                        surf = ship_gfx[White][pose];
                    else
                        surf = ship->ship[pose];
                    SDL_BlitSurface (surf, &rect2, screen, &rect);
                    if (ship->shieldup) {
                        SDL_Rect sr, tr;
                        tr.x =
                            viewport_rects[plr].x + Round(ship->physics.x) -
                            cam_rects[plr].x
                            - 16;
                        tr.y =
                            viewport_rects[plr].y + Round(ship->physics.y) -
                            cam_rects[plr].y
                            - 16;
                        tr.w = ship->shield->w;
                        tr.h = ship->shield->h;
                        sr = cliprect (tr.x, tr.y, tr.w, tr.h,
                                       viewport_rects[plr].x, viewport_rects[plr].y,
                                       viewport_rects[plr].x + cam_rects[plr].w,
                                       viewport_rects[plr].y + cam_rects[plr].h);
                        if (tr.x < viewport_rects[plr].x)
                            tr.x = viewport_rects[plr].x;
                        if (tr.y < viewport_rects[plr].y)
                            tr.y = viewport_rects[plr].y;
                        SDL_BlitSurface (ship->shield, &sr, screen, &tr);
                    }
                    if (ship->remote_control) {
                        SDL_Rect sr, tr;
                        tr.x =
                            viewport_rects[plr].x + Round(ship->physics.x) -
                            cam_rects[plr].x - 16;
                        tr.y =
                            viewport_rects[plr].y + Round(ship->physics.y) -
                            cam_rects[plr].y - 16;
                        tr.w = remocon_gfx[ship->anim]->w;
                        tr.h = remocon_gfx[ship->anim]->h;
                        sr = cliprect (tr.x, tr.y, tr.w, tr.h,
                                       viewport_rects[plr].x, viewport_rects[plr].y,
                                       viewport_rects[plr].x + cam_rects[plr].w,
                                       viewport_rects[plr].y + cam_rects[plr].h);
                        if (tr.x < viewport_rects[plr].x)
                            tr.x = viewport_rects[plr].x;
                        if (tr.y < viewport_rects[plr].y)
                            tr.y = viewport_rects[plr].y;
                        SDL_BlitSurface (remocon_gfx[ship->anim], &sr, screen,
                                         &tr);
                    }
                    if (ship->darting) {
                        int cx, cy;
                        float dx, dy;
                        cx = rect.x + ship_gfx[Grey][0]->w / 2;
                        cy = rect.y + ship_gfx[Grey][0]->h / 2;
                        if (ship->darting == DARTING) {
                            dx = cos (ship->angle);
                            dy = sin (ship->angle);
                            draw_line (screen, cx + dx * 5, cy - dy * 5,
                                       cx + dx * 10, cy - dy * 10, col_gray);
                        } else {
                            double h=hypot(ship->physics.vel.x,ship->physics.vel.y);
                            dx = ship->physics.vel.x/h;
                            dy = ship->physics.vel.y/h;
                            draw_line (screen, cx + dx * 6, cy - dy * 6,
                                       cx - dx * 6, cy + dy * 6, col_gray);
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
    /* Find the player who controls this ship */
    int num = find_player (ship);

    /* Set ship state to DESTROYED. It will be deleted at the end of
     * the animation loop in animate_ship()
     */
    ship->state = DESTROYED;
    ship->visible = 0;

    /* Mark the player as dead if pilot has not been ejected */
    if (num >= 0)
        kill_player (num);

    /* Make sure no pilots have roped this ship */
#if 0
    for(num=0;num<4;num++) {
        if(players[num].state == ALIVE && players[num].pilot.rope_ship==ship) {
            pilot_detach_rope(&players[num].pilot);
        }
    }
#endif

    /* Explosion and sound effect */
    spawn_clusters (ship->physics.x, ship->physics.y,5.6, 32, make_bullet);
    spawn_clusters (ship->physics.x, ship->physics.y,5.6, 16, make_firestarter);
    playwave(WAV_EXPLOSION2);
}

/** Stop special weapons **/
void ship_stop_special(struct Ship *ship) {
    if (ship->remote_control) {
        if ((int) ship->remote_control > 1) {
            remote_control(ship,0);
        } else {
            struct dllist *tmp2 = ship_list;
            while (tmp2) {
                if (((struct Ship*)tmp2->data)->remote_control == ship) {
                    remote_control(tmp2->data,0);
                    break;
                }
                tmp2 = tmp2->next;
            }
        }
    }
    ship->fire_special_weapon = 0;
    ship->antigrav = 0;
    if(ship->shieldup) {
        remove_ga(ship->shieldup);
        ship->shieldup = NULL;
    }
    ship->afterburn = 0;
    ship->repairing = 0;
    ship->visible = 1;
    if (ship->physics.solidity==IMMATERIAL)
        ghostify (ship, 0);
}

/* Find nearest ship and activate remote control if found */
void remote_control(struct Ship *ship,int activate) {
    if(activate) {
        struct Ship *targ;
        double d;
        targ = find_nearest_ship (ship->physics.x, ship->physics.y, ship, &d);
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
    } else {
        ship->remote_control->remote_control = NULL;
        ship->remote_control->thrust = 0;
        ship->remote_control->turn = 0;
        ship->remote_control->fire_weapon = 0;
        ship_stop_special(ship->remote_control);
        ship->remote_control = NULL;
    }
}

/* Ship turns gray and starts tumbling */
static void kill_ship (struct Ship * ship)
{
    /* Stop weapons */
    ship->fire_weapon = 0;
    ship_stop_special(ship);

    /* Kill the ship */
    ship->state = BROKEN;
    ship->frozen = 0;
    ship->health = 0;
    ship->thrust = 0;
    ship->physics.thrust.x = 0;
    ship->physics.thrust.y = 0;
    /* Ship turns heavier so it sinks in water */
    ship->physics.mass *= 6.0;

    /* Set the ship spinning */
    if (ship->angle < M_PI)
        ship->turn = -0.6;
    else
        ship->turn = 0.6;

    /* Sound effect */
    playwave(WAV_CRASH);
}

/* Calculate ship thrust vector */
static void set_ship_thrust(struct Ship *ship) {
    double t;
    if(ship->darting) {
        ship->physics.thrust = get_constant_vel(ship->physics.vel,
                ship->physics.radius,ship->physics.mass);
        return;
    } else if(ship->afterburn) {
        t = 1.1;
    } else if(ship->thrust || ((ship->criticals & CRITICAL_FUELCONTROL)
                && rand()%6==0))
    {
        t = 0.5;
    } else {
        ship->physics.thrust.x = 0;
        ship->physics.thrust.y = 0;
        return;
    }
    ship->physics.thrust.x = cos (ship->angle) * t;
    ship->physics.thrust.y = -sin (ship->angle) * t;
}

/* Ship exhaust gas effect */
static void ship_exhaust (struct Ship * ship)
{
    struct Particle *part;
    part = make_particle (ship->physics.x, ship->physics.y, 15);
    part->vector = oppositeVector (ship->physics.thrust);
    if (ship->physics.underwater) {
        part->color[0] = 100;
        part->color[1] = 100;
        part->color[2] = 255;
        part->rd = -20;
        part->bd = -20;
        part->gd = -31;
        /*calc_color_deltas(part,0,0,0,255);*/
        part->vector.x += 0.5 - ((rand () % 100) / 100.0);
        part->vector.y += 0.5 - ((rand () % 100) / 100.0);
    }
}

/* Damage a ship */
void damage_ship(struct Ship *ship, float damage,float critical) {
    ship->health -= damage;
    if(ship->health<0) ship->health=0;
    ship->white_ship = SHIP_WHITE_DUR;

    if(game_settings.criticals && critical>0) {
        if(rand() < (RAND_MAX * critical))
            ship_critical(ship, 0);
    }
}

/* Recharge energy and repair */
static void service_ship(struct Ship *ship) {
    if (ship->health < 1 && !ship->shieldup) {
        struct Particle *spark;
        ship->health += 0.0012;
        /* Special effects */
        spark =
            make_particle (ship->physics.x + (rand () % 16) - 8,
               ship->physics.y + (rand () % 16) - 8, 6);
        spark->vector.x = (rand () % 4) - 2;
        spark->vector.y = -3;
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
    if (ship->physics.underwater)
        ship->angle = 1.5*M_PI;
    else
        ship->angle = M_PI_2;
}

/** Ship animation **/
void animate_ships (void) {
    struct dllist *current = ship_list;
    struct Ship *ship;
    /* Loop through all ships */
    while (current) {
        struct dllist *next=current->next;
        ship = current->data;

        /* Turn the ship */
        ship->angle += ship->turn;
        if (ship->angle > 2 * M_PI)
            ship->angle = 0;
        if (ship->angle < 0)
            ship->angle = 2 * M_PI;

        if (ship->state==INTACT) {
            /* Counters */
            if (ship->cooloff)
                ship->cooloff--;
            if (ship->special_cooloff)
                ship->special_cooloff--;
            if (ship->eject_cooloff)
                ship->eject_cooloff--;
            if (ship->white_ship > 0)
                ship->white_ship--;
            if (ship->no_power > 0)
                ship->no_power--;
            if (ship->visible > 1)
                ship->visible--;
            if ((ship->criticals & CRITICAL_CARGO) && ship->energy > 0)
                ship->energy -= 0.03/GAME_SPEED;
            if(ship->no_power) {
                ship->physics.thrust.x = 0;
                ship->physics.thrust.y = 0;
            } else {
                set_ship_thrust(ship);

                /* Fire normal weapon */
                if (ship->fire_weapon && ship->cooloff == 0)
                    ship_fire_standard_weapon (ship);

                /* Fire special weapons */
                if (ship->fire_special_weapon && ship->special_cooloff==0)
                    ship_fire_special(ship);

                if(ship->physics.thrust.x!=0 || ship->physics.thrust.y!=0) {
                    /* Exhaust effects */
                    if(ship->visible)
                        ship_exhaust (ship);

                    /* Critical engine core */
                    if (ship->thrust && (ship->criticals & CRITICAL_ENGINE)) {
                        int s;
                        ship->health -= 0.03/GAME_SPEED;
                        if (ship->health <= 0.0) {
                            kill_ship (ship);
                            spawn_clusters (ship->physics.x, ship->physics.y,
                                    5.6, 6, make_napalm);
                        }
                        for (s = 0; s < 4; s++) {
                            struct Particle *smoke;
                            smoke =
                                make_particle (ship->physics.x + 8 - rand ()%16,
                                               ship->physics.y + 8 - rand ()%16, 15);
                            smoke->vector.x = weather_wind_vector;
                            smoke->vector.y = -3.0 * (rand () % 20) / 10.0;
                        }
                    }
                } else if (ship->afterburn && ship->physics.underwater == 0) {
                    start_burning (ship->physics.x, ship->physics.y);
                }
            }
            /* Player specials like cloaking device and such */
            ship_specials (ship);

        }

        /* Do physics simulation */
#if 0
        animate_object(&ship->physics,game_settings.ship_collisions>0,&ship_list);
#else
        animate_object(&ship->physics,0,0); /* TODO ship to ship collisions
                                               disabled until object impacts
                                               work properly */
#endif

        /* Ground collisions */
        if(ship->physics.hitground) {
            if(ship->state==BROKEN) {
                /* Ship is dead, explode it */
                finalize_ship(ship);
            } else {
                /* Recharge on base */
                if(ship->physics.hitground == TER_BASE)
                    service_ship(ship);

                /* Collision damage */
                if(game_settings.coll_damage &&
                        ship->physics.hitground != TER_SNOW)
                {
                    double vel = hypot(ship->physics.hitvel.x,
                            ship->physics.hitvel.y);
                    if(vel>DAMAGE_TRESHOLD) {
                        damage_ship(ship,0.01,0.01);
                    }
                }

                /* Touching ground breaks status ailments */
                ship->frozen = 0;
                if(ship->darting) {
                    ship->darting = NODART;
                    ship->physics.sharpness = BOUNCY;
                    damage_ship(ship,0.1,0.0);
                }
            }
        } else if(ship->physics.underwater) {
            ship->frozen=0;
        }

        /* Ship collisions (with Dart) */
        if(ship->physics.hitobj && ship->darting) {
            struct Ship *opponent = dllist_find(ship_list,ship->physics.hitobj)->data;
            damage_ship(opponent,0.4,0.1);
            ship->darting = NODART;
            ship->physics.sharpness = BOUNCY;
        }

        /* Kill ship if health goes below 0 */
        if (ship->health <= 0.0 && ship->state==INTACT)
            kill_ship (ship);

        /* Delete destroyed ships */
        if (ship->state == DESTROYED) {
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

/* Critical hit names */
const char *critical2str (int critical)
{
    switch (critical) {
    case CRITICAL_ENGINE:
        return "Critical engine core";
    case CRITICAL_FUELCONTROL:
        return "Critical fuel control";
    case CRITICAL_LTHRUSTER:
        return "Critical left thruster";
    case CRITICAL_RTHRUSTER:
        return "Critical right thruster";
    case CRITICAL_CARGO:
        return "Critical cargo hold";
    case CRITICAL_STDWEAPON:
        return "Critical mainweapon";
    case CRITICAL_SPECIAL:
        return "Critical special weapon";
    case CRITICAL_POWER:
        return "Temporary power failure";
    default:
        return "Unhandled critical";
    }
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
        ship_stop_special(ship);
        ship->no_power = 60;
        break;
    default:
        fputs("Bug! ship_critical(): unhandled critical hit!\n",stderr);
        return;
    }
    if ((ship->criticals & c) == 0) {
        int plr = find_player (ship);
        if (repair)
            return;
        if (c == CRITICAL_LTHRUSTER && (ship->criticals & CRITICAL_RTHRUSTER))
            return; /* We won't leave the ship TOTALLY disabled... */
        if (c == CRITICAL_RTHRUSTER && (ship->criticals & CRITICAL_LTHRUSTER))
            return;
        if(c != CRITICAL_POWER) ship->criticals |= c;
        if (plr >= 0)
            set_player_message (plr, Bigfont, font_color_red,
                    c==CRITICAL_POWER?ship->no_power:25, critical2str (c));
    } else if (repair) {
        int plr = find_player (ship);
        ship->criticals &= ~c;
        if (plr >= 0)
            set_player_message (plr, Bigfont, font_color_green, 25,
                                critical2str (c));
    }
}

/* Player claims a ship */
void claim_ship (struct Ship * ship, int plr)
{
    ship->color = Red + plr;
    ship->ship = ship_gfx[ship->color];
    ship->shield = shield_gfx[plr];
    ship->standard = players[plr].standardWeapon;
    ship->special = players[plr].specialWeapon;
}

/* Special weapons that affect the state of the ship */
static void ship_specials (struct Ship * ship) {
    /* Remote control animation */
    if (ship->remote_control) {
        if ((int) ship->remote_control > 1) {
            ship->anim++;
            if (ship->anim >= remocon_frames)
                ship->anim = 0;
            ship->energy -= 0.004;
            if (ship->energy <= 0)
                remote_control(ship,0);
        } else {
            if (ship->anim <= 0)
                ship->anim = remocon_frames;
            ship->anim--;
        }
    }
    /* The rest are mutually exclusive (one player can have only one special weapon */
    if(special_weapon[ship->special].id == WEAP_AUTOREPAIR &&
            ship->criticals && rand () % 32 == 0)
    {
            ship_critical (ship, 1);
    }
    /* Cloaking device */
    if (ship->visible == 0) {
            ship->energy -= 0.001;
        if (ship->energy <= 0) {
            ship->visible = 1;
            ship->energy = 0;
        }
    }
    /* Ghost mode */
    else if (ship->physics.solidity == IMMATERIAL) {
            ship->energy -= 0.002;
        if (ship->energy <= 0) {
            ghostify (ship, 0);
            ship->energy = 0;
        }
    }
    /* Shield */
    else if (ship->shieldup) {
            //ship->energy -= 0.005;
        if (ship->energy <= 0) {
            ship_stop_special(ship);
            ship->energy = 0;
        }
    }
    /* Afterburner */
    else if (ship->afterburn) {
        if (ship->repeat_audio == 0) {
            playwave (WAV_BURN);
            ship->repeat_audio = 15;
        } else
            ship->repeat_audio--;
            ship->energy -= 0.002;
        if (ship->energy <= 0) {
            ship->afterburn = 0;
            ship->energy = 0;
        }
    }
    /* Gravity polarizer active mode */
    else if (ship->antigrav) {
            ship->energy -= 0.0005;
            /* Counter gravity and lift */
            ship->physics.vel.y = ship->physics.vel.y - GRAVITY +
                (AIR_rho * ship->physics.radius * GRAVITY) / ship->physics.mass;
        if (ship->energy <= 0) {
            ship->antigrav = 0;
            ship->energy = 0;
        }
    }
    /* Autorepair */
    else if (ship->repairing) {
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
            struct Particle *spark;
            ship->anim = rand () % 15;
            spark =
                make_particle (ship->physics.x + (rand () % 16) - 8,
                               ship->physics.y + (rand () % 16) - 8, 15);
            spark->color[0] = 255;
            spark->color[1] = 255;
            spark->color[2] = 0;
            spark->rd = -7;
            spark->gd = -17;
            spark->bd = 0;
            spark->vector.x = (rand () % 6) - 3;
            spark->vector.y = 2;
        } else
            ship->anim = 0;
    }
}

/* Enable/disable ghost ship effect */
void ghostify (struct Ship * ship, int activate)
{
    if (activate)
        ship->ship = ghost_gfx[ship->color - 1];
    else
        ship->ship = ship_gfx[ship->color];

    ship->physics.solidity = activate?IMMATERIAL:SOLID;
}

/* Check if there is a ship in the specified coordinates and bump
 * it one pixel upwards if there is. This is used to keep ships
 * from getting stuck in regenerating bases */
void bump_ship(int x,int y) {
    struct dllist *shplst=ship_list;
    while(shplst) {
        struct Ship *ship=shplst->data;

        if(Round(ship->physics.x) == x && Round(ship->physics.y) == y) {
            ship->physics.y--;
            break;
        }
        shplst=shplst->next;
    }
}

/* Cloaking device engage/disengage effect */
void cloaking_device (struct Ship * ship, int activate)
{
    SDL_Surface *surf;
    struct Particle *part;
    Uint32 *src;
    int x, y, p;
    p= Round(ship->angle/(2*M_PI)*SHIP_POSES);
    if (p > 35)
        p = 35;
    surf = ship->ship[p];
    src = surf->pixels;
    for (y = -surf->h / 2; y < surf->h / 2; y++)
        for (x = -surf->w / 2; x < surf->w / 2; x++) {
            if (((*src & surf->format->Amask) >> surf->format->Ashift) > 30) {
                if (activate) {
                    part = make_particle (ship->physics.x + x, ship->physics.y + y, 8);
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
                        ship->physics.vel.x + 1.0 - ((rand () % 20) / 10.0);
                    part->vector.y =
                        ship->physics.vel.y + 1.0 - ((rand () % 20) / 10.0);
                } else {
                    Uint8 tr,tg,tb,ta;
                    part = make_particle (ship->physics.x + x, ship->physics.y + y, 3);
                    part->color[0]=0; part->color[1]=0;
                    part->color[2]=0; part->color[3]=0;
                    tr = (*src & surf->format->Rmask) >> surf->format->Rshift;
                    tg = (*src & surf->format->Gmask) >> surf->format->Gshift;
                    tb = (*src & surf->format->Bmask) >> surf->format->Bshift;
                    ta = (*src & surf->format->Amask) >> surf->format->Ashift;
                    calc_color_deltas (part,tr,tg,tb,ta);
                    part->vector = ship->physics.vel;
                }
            }
            src++;
        }
}

/* Fire the primary weapon */
static void ship_fire_standard_weapon (struct Ship * ship)
{
    playwave(WAV_NORMALWEAP);
    normal_weapon[ship->standard].fire(ship);
    ship->cooloff = normal_weapon[ship->standard].cooloff;
}

/* Fire special weapon */
void ship_fire_special(struct Ship *ship) {
    Vector mvel;
    double fx,fy;

    if((special_weapon[ship->special].not_waterproof&&ship->physics.underwater)
            || ship->energy < special_weapon[ship->special].energy)
        return;

    playwave(special_weapon[ship->special].sfx);

    fx = ship->physics.x + cos(ship->angle) * 5.0;
    fy = ship->physics.y + -sin(ship->angle) * 5.0;

    mvel = addVectors(get_muzzle_vel(ship->angle),ship->physics.vel);

    if(special_weapon[ship->special].make_bullet) {
        add_projectile(special_weapon[ship->special].make_bullet(fx,fy,mvel));
    } else {
        special_weapon[ship->special].fire(ship,fx,fy,mvel);
    }

    ship->energy -= special_weapon[ship->special].energy;
    if (ship->energy < 0)
        ship->energy = 0;
    ship->special_cooloff = special_weapon[ship->special].cooloff;

}

/* Find the nearest enemy player in a ship that is visible */
int find_nearest_enemy (double myX, double myY, int myplr, double *dist) {
    int p, plr = -1;
    double distance = 9999999, d;
    for (p = 0; p < 4; p++) {
        if (same_team(p,myplr)) continue;
        if (players[p].ship==NULL || players[p].ship->state!=INTACT
                || players[p].ship->visible!=1) continue;
        d = hypot (players[p].ship->physics.x - myX,
                   players[p].ship->physics.y - myY);
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
struct Ship *find_nearest_ship (double myX, double myY, struct Ship * not_this, double *dist)
{
    struct dllist *current = ship_list;
    struct Ship *nearest = NULL;
    double distance = 9999999, d;
    while (current) {
        struct Ship *ship=current->data;
        if (ship->state!=INTACT || ship == not_this) {
            current = current->next;
            continue;
        }
        d = hypot (ship->physics.x - myX, ship->physics.y - myY);
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

/* Freeze a ship */
void freeze_ship(struct Ship *ship) {
    ship_stop_special(ship);
    ship->frozen = 1;
    ship->thrust = 0;
    ship->turn = 0;
    ship->fire_weapon = 0;
}

/* Impale a ship */
void spear_ship(struct Ship *ship,struct Projectile *spear) {
    ship->physics.vel = spear->physics.vel;
    ship->darting = SPEARED;
    ship->physics.sharpness = BLUNT;
    ship->fire_weapon = 0;
    ship->fire_special_weapon = 0;
    ship->turn=0;
    ship->physics.vel.x += cos(spear->angle) * 30.0;
    ship->physics.vel.y += sin(spear->angle) * 30.0;

}


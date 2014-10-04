/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : special.c
 * Description : Level special (eg. turrets and jump-gates)
 *               handling and animation
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
#include <math.h>

#include "SDL.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "list.h"
#include "fs.h"
#include "player.h"
#include "level.h"
#include "console.h"
#include "weapon.h"
#include "special.h"
#include "vector.h"
#include "ship.h"
#include "audio.h"

#define WARP_FRAMES     21
#define WARP_LOOP       10
#define TURRET_FRAMES   15

/* Internally used globals */
static struct dllist *specials;

static SDL_Surface *fx_jumpgate[1];
static SDL_Surface *fx_turret[3][TURRET_FRAMES];        /* Normal turrets, upside down turrets and SAM-Sites */
static SDL_Surface *fx_jumppoint_entry[WARP_FRAMES];
static SDL_Surface *fx_jumppoint_exit[WARP_FRAMES];

/* Internally used function */
static int hitsolid_rect (int x, int y, int w, int h)
{
    int tx, ty, tx2, ty2;
    tx2 = x + w;
    if (tx2 > lev_level.width)
        return 1;
    ty2 = y + h;
    if (ty2 > lev_level.height)
        return 1;
    for (tx = x; tx < tx2; tx++)
        for (ty = y; ty < ty2; ty++)
            if (lev_level.solid[tx][ty] != TER_FREE
                && lev_level.solid[tx][ty] != TER_WATER)
                return 1;
    return 0;
}

static int find_turret_y (int x, int y)
{
    int pos;
    char a1, a2;
    a1 = lev_level.solid[x][y];
    a2 = a1;
    for (pos = 0; pos < 300; pos++) {
        if (y + pos < lev_level.height - 30) {
            if (lev_level.solid[x][y + pos] == TER_GROUND && a1 == TER_FREE)
                return y + pos;
            a1 = lev_level.solid[x][y + pos];
        }
        if (y - pos > 30) {
            if (lev_level.solid[x][y - pos] == TER_GROUND && a2 == TER_FREE)
                return y - pos;
            a2 = lev_level.solid[x][y - pos];
        }
    }
    return -1;
}

/*** Load level special images ***/
void init_specials (LDAT *specialfile) {
    int r;
    specials = NULL;
    for (r = 0; r < WARP_FRAMES; r++) {
        fx_jumppoint_entry[r] = load_image_ldat (specialfile, 0, T_ALPHA,"WARP",r);
        fx_jumppoint_exit[r] = load_image_ldat (specialfile, 0, T_ALPHA,"WARPE",r);
    }
    for (r = 0; r < TURRET_FRAMES; r++) {
        fx_turret[0][r] = load_image_ldat (specialfile, 0, T_ALPHA, "TURRET", r);
        fx_turret[1][r] = load_image_ldat (specialfile, 0, T_ALPHA, "SAMSITE", r);
        fx_turret[2][r] = flip_surface (fx_turret[0][r]);
    }
    fx_jumpgate[0] = load_image_ldat (specialfile, 0, T_ALPHA, "JUMPGATE", 0);
}

/*** Clear all level specials at the end of the level ***/
void clear_specials (void) {
    dllist_free(specials,free);
    specials=NULL;
}

/*** Load level specials at the start of the level ***/
void prepare_specials (struct LevelSettings * settings) {
    SpecialObj *gate1, *gate2, *turret, *obj;
    struct dllist *list, *list2, *objects=NULL;
    int r, loops, tmpi;
    /* Initialize jump gates */
    for (r = 0; r < level_settings.jumpgates; r++) {
        gate1 = malloc (sizeof (SpecialObj));
        gate1->age = -1;
        gate1->frame = 0;
        gate1->type = JumpGate;
        gate1->owner = -1;
        gate1->timer = 0;
        gate1->var1 = 0;
        loops = 0;
        do {
            gate1->x = rand () % lev_level.width;
            gate1->y = rand () % lev_level.height;
            loops++;
            if (loops > 1000)
                break;
        } while (hitsolid_rect (gate1->x, gate1->y, 32, 32));
        if (loops < 1000) {
            gate2 = malloc (sizeof (SpecialObj));
            gate2->age = -1;
            gate2->frame = 0;
            gate2->type = JumpGate;
            gate2->owner = -1;
            gate2->timer = 0;
            gate2->var1 = 0;
            loops = 0;
            do {
                gate2->x = rand () % lev_level.width;
                gate2->y = rand () % lev_level.height;
                loops++;
                if (loops > 1000)
                    break;
            } while (hitsolid_rect (gate2->x, gate2->y, 32, 32)
                     || hypot (gate2->x - gate1->x,
                               gate2->y - gate1->y) < 200);
            if (loops < 10000) {
                gate1->link = gate2;
                gate2->link = gate1;
                gate1->health = -1;
                gate2->health = -1;
                addspecial (gate2);
                addspecial (gate1);
            }
        }
    }
    /* Initialize turrets */
    for (r = 0; r < level_settings.turrets; r++) {
        turret = malloc (sizeof (SpecialObj));
        loops = 0;
        while (1) {
            do {
                turret->x = 30 + rand () % (lev_level.width - 60);
                turret->y = 30 + rand () % (lev_level.height - 60);
                loops++;
                if (loops > 5000)
                    break;
            } while (lev_level.solid[turret->x][turret->y] != TER_FREE);
            if (loops > 5000)
                break;
            tmpi = find_turret_y (turret->x, turret->y);
            if (tmpi != -1) {
                turret->y = tmpi;
                break;
            }
        }
        if (loops < 5000) {
            for (tmpi = turret->y + 1; tmpi < turret->y + 7; tmpi++)
                if (lev_level.solid[turret->x][tmpi] != TER_FREE
                    && lev_level.solid[turret->x][tmpi] != TER_TUNNEL)
                    break;
            if (tmpi < turret->y + 6) {
                turret->y -= 7;
                turret->var1 = rand () % 3;     /* Normal turret, Grenade launcher,SAM site */
            } else {            /* Turret is upside down */
                turret->var1 = 3 + (rand () & 0x01);
            }
            turret->type = Turret;
            turret->owner = -1;
            turret->age = -1;
            turret->frame = 1 + rand () % (TURRET_FRAMES - 2);
            turret->timer = 0;
            turret->link = NULL;
            turret->health = 1;
            turret->var2 = rand () & 0x01;
            addspecial (turret);
        }
    }
    /* Initialize manually placed specials */
    if (settings)
        objects = settings->objects;
    while (objects) {
        struct LSB_Object *object = objects->data;
        if (object->type >= FIRST_SPECIAL && object->type <= LAST_SPECIAL) {
            obj = malloc (sizeof (SpecialObj));
            if (object->type == OBJ_TURRET)
                obj->type = Turret;
            else if (object->type == OBJ_JUMPGATE)
                obj->type = JumpGate;
            else fprintf(stderr,"Warning: unhandled special %d\n",obj->type);
            obj->x = object->x;
            obj->y = object->y;
            obj->owner = -1;
            obj->age = -1;
            obj->timer = 0;
            obj->link = NULL;
            obj->health = 1;
            if (obj->type == Turret) {
                obj->var1 = object->value;
                obj->var2 = rand () & 0x01;
                obj->frame = 1 + rand () % (TURRET_FRAMES - 2);
            } else {
                obj->var1 = object->id; /* We borrow this */
                obj->link = (SpecialObj *) (int) object->link;
                obj->frame = 0;
            }
            if (object->ceiling_attach == 0) {
                if (obj->type == Turret) {
                    obj->y -= fx_turret[0][0]->h;
                    obj->var1 = object->value;
                } else {
                    obj->y -= fx_jumpgate[0]->h;
                }
            } else if (obj->type == Turret)
                obj->var1 = 3 + object->value;
            addspecial (obj);
        }
        objects = objects->next;
    }
    /* Pair up the jumpgates */
    list = specials;
    while (list) {
        SpecialObj *special=list->data;
        if (special->type == JumpGate)
            if ((int) special->link <= 255) {
                list2 = list->next;
                while (list2) {
                    SpecialObj *special2=list2->data;
                    if (special2->type == JumpGate
                        && (int)special2->link <= 255)
                        if (special2->var1 == (int) special->link) {
                            special->var1 = 0;
                            special->link = special2;
                            special2->var1 = 0;
                            special2->link = special;
                            break;
                        }
                    list2 = list2->next;
                }
                if (list2 == NULL) {
                    fprintf(stderr,
                         "Warning! Pairless jumpgate with id %d and link %d (removed)\n",
                         special->var1, (int) special->link);
                    list2 = list->next;
                    free(list->data);
                    if(list==specials)
                        specials=dllist_remove(list);
                    else
                        dllist_remove(list);
                    list = list2;
                    continue;
                }
            }
        list = list->next;
    }
}

/*** Add a new level special ***/
void addspecial (SpecialObj *special) {
    if(specials)
        dllist_append(specials,special);
    else
        specials=dllist_append(specials,special);
}

/*** Drop a jump point. If it has a pair, the points will be opened ***/
void drop_jumppoint (int x, int y, char player) {
    struct dllist *list = specials;
    SpecialObj *jump, *exit = NULL;
    jump = malloc (sizeof (SpecialObj));
    jump->x = x - 16;
    jump->y = y - 16;
    jump->owner = player;
    jump->frame = 0;
    jump->timer = (game_settings.jumplife == 0) ? 0 : WARP_LOOP / 2;
    jump->health = -1;
    while (list) {
        SpecialObj *obj=list->data;
        if (obj->type == WarpExit && obj->owner == player)
            if (obj->link == NULL)
                exit = obj;
        list = list->next;
    }
    if (exit == NULL) {
        jump->type = WarpExit;
        jump->link = NULL;
        jump->age = -1;
    } else {
        jump->type = WarpEntry;
        jump->link = exit;
        exit->link = jump;
        jump->age =
            (game_settings.jumplife ==
             0) ? JPSHORTLIFE : (game_settings.jumplife ==
                                 1) ? JPMEDIUMLIFE : JPLONGLIFE;
        exit->age = jump->age;
#if HAVE_LIBSDL_MIXER
        playwave (WAV_JUMP);
#endif
    }
    addspecial (jump);
}

static inline void draw_special (SpecialObj * special)
{
    SDL_Surface **sprite = NULL;
    SDL_Rect rect, rect2;
    signed char showme = -1;
    int p;
    switch (special->type) {
    case Unknown:
        sprite = NULL;
        break;
    case WarpEntry:
        sprite = fx_jumppoint_entry;
        if (special->frame == 0)
            showme = special->owner;
        break;
    case WarpExit:
        sprite = fx_jumppoint_exit;
        if (special->frame == 0)
            showme = special->owner;
        break;
    case JumpGate:
        sprite = fx_jumpgate;
        break;
    case Turret:
        sprite =
            fx_turret[(special->var1 == 2) + (special->var1 > 2 ? 2 : 0)];
        break;
    }
    for (p = 0; p < 4; p++) {
        if ((players[p].state==ALIVE||players[p].state==DEAD) && (showme == p || showme == -1)) {
            rect.w = sprite[0]->w;
            rect.h = sprite[0]->h;
            rect.x = special->x - cam_rects[p].x + lev_rects[p].x;
            rect.y = special->y - cam_rects[p].y + lev_rects[p].y;
            if ((rect.x > lev_rects[p].x - rect.w
                 && rect.x < lev_rects[p].x + cam_rects[p].w)
                && (rect.y > lev_rects[p].y - rect.h
                    && rect.y < lev_rects[p].y + cam_rects[p].h)) {
                rect2 =
                    cliprect (rect.x, rect.y, rect.w, rect.h, lev_rects[p].x,
                              lev_rects[p].y, lev_rects[p].x + cam_rects[p].w,
                              lev_rects[p].y + cam_rects[p].h);
                if (rect.x < lev_rects[p].x)
                    rect.x = lev_rects[p].x;
                if (rect.y < lev_rects[p].y)
                    rect.y = lev_rects[p].y;
                SDL_BlitSurface (sprite[special->frame], &rect2, screen,
                                 &rect);
            }
        }
    }
}

static void ship_hit_special(struct dllist *list) {
    SpecialObj *obj=list->data;
    struct dllist *ships;
    int w;
    w = 32;
    ships = ship_list;
    while (ships) {
        struct Ship *ship=ships->data;
        if (ship->x >= obj->x && ship->x <= obj->x + w)
            if (ship->y >= obj->y
                && ship->y <= obj->y + w) {
                switch (obj->type) {
                case WarpEntry:
                case WarpExit:
                    if (obj->link && obj->timer == 0) {
                        ship->x = obj->link->x + w / 2;
                        ship->y = obj->link->y + w / 2;
                        obj->timer = 25;
                        obj->link->timer = 25;
                    }
                    break;
                case JumpGate:
                    if (obj->timer == 0) {
                        obj->timer =
                            (game_settings.jumplife ==
                             0) ? JPSHORTLIFE : (game_settings.jumplife ==
                                                 1) ? JPMEDIUMLIFE :
                            JPLONGLIFE;
                        obj->link->timer = obj->timer;
                        drop_jumppoint (obj->x + 16,
                                        obj->y + 16,
                                        find_player (ship) + 10);
                        drop_jumppoint (obj->link->x + 16,
                                        obj->link->y + 16,
                                        find_player (ship) + 10);
                    }
                    break;
                case Turret:
                    if (ship->darting) {
                        spawn_clusters (obj->x, obj->y,
                                        10, Cannon);
                        add_explosion (obj->x, obj->y,
                                       Noproj);
                        free (obj);
                        if(list==specials)
                            specials=dllist_remove(list);
                        else
                            dllist_remove(list);
                        return;
                    }
                    break;
                default:
                    break;
                }
            }
        ships = ships->next;
    }
}

/*** Turret firing ***/
static void turret_shoot(SpecialObj *turret) {
    double dist,range, angle;
    int plr,targx,targy,targframe;
    Projectile *p;

    if (turret->var1 == 2)
        range = 190.0;      /* SAM site */
    else
        range = 140.0;      /* Regular turret */
    plr = find_nearest_player(turret->x, turret->y, -1, &dist);
    /* Check if a ship is in range */
    if (plr >= 0 && dist<range &&
            (players[plr].ship->visible||players[plr].ship->tagged)) {
        targx = players[plr].ship->x;
        targy = players[plr].ship->y;
    } else {    /* Check if a pilot is in range */
        if(turret->var1==2) return;
        plr = find_nearest_pilot(turret->x, turret->y, -1, &dist);
        if(plr>=0 && dist<range) {
            targx = players[plr].pilot.x;
            targy = players[plr].pilot.y;
        } else {
            return; /* Nothing in range */
        }
    }
    /* Aim turret */
    angle = atan2 (targx - turret->x, targy - turret->y);
    if (((angle < -M_PI_2 || angle > M_PI_2) && turret->var1 < 2)
                || turret->var1 == 2
                || ((angle > -M_PI_2 && angle < M_PI_2) && turret->var1 > 2)) {
        if (turret->var1 > 2) {      /* Upside down */
            targframe=Round( (angle+M_PI_2) / M_PI * (TURRET_FRAMES-1) );
        } else {    /* Normal */
            double a2;
            if (angle > -M_PI && angle < -M_PI_2) {
                a2=-angle-M_PI_2;
            } else if(angle > M_PI_2 && angle<M_PI) {
                a2=M_PI+M_PI_2-angle;
            } else if(fabs(angle-M_PI)<0.01) {  /* Special case: straight up */
                a2=M_PI_2;
            } else { /* Missile turret can target ships from below as well */
                a2=angle+M_PI_2;
            }
            targframe=Round( a2 / M_PI * (TURRET_FRAMES-1) );
        }
        if(targframe>=TURRET_FRAMES) targframe=TURRET_FRAMES-1;
        else if(targframe<0) targframe=0;

        if(turret->frame<targframe) {
            turret->frame++;
        } else if(turret->frame>targframe) {
            turret->frame--;
        } else {
            /* Fire! */
            p = make_projectile (turret->x +
                                 sin (angle) * fx_turret[0][0]->w,
                                 turret->y +
                                 cos (angle) * fx_turret[0][0]->h,
                                 makeVector (-sin (angle), -cos (angle)));
            if (turret->var1 > 2)
                p->y += fx_turret[0][0]->h + 3;
            if (turret->var1 == 1) {
                p->type = Grenade;
                p->color = col_grenade;
                turret->timer = 25;
            } else if (turret->var1 == 2) {
                p->type = Missile;
                p->color = col_gray;
                turret->timer = 45;
                p->angle = angle;
            } else
                turret->timer = 25;
#if HAVE_LIBSDL_MIXER
            if (p->type == Missile)
                playwave_3d (WAV_MISSILE, p->x, p->y);
            else
                playwave_3d (WAV_NORMALWEAP, p->x, p->y);
#endif
            add_projectile (p);
        }
    }
}

/*** Do level special animations ***/
void animate_specials (void) {
    struct dllist *list = specials,*next;
    while (list) {
        SpecialObj *obj=list->data;
        /* Animations */
        switch (obj->type) {
            case WarpExit:
            case WarpEntry:
                obj->frame++;
                if (obj->age < 0)
                    obj->frame = 0;
                else if (obj->frame >= WARP_FRAMES)
                    obj->frame = WARP_LOOP;
                break;
            case Turret:
                if (obj->timer != 0)
                    break;
                if (obj->var2 < 5) {
                    obj->var2++;
                    if (obj->var2 == 4) {
                        obj->var2 = 0;
                        obj->frame++;
                        if (obj->frame >= TURRET_FRAMES - 1) {
                            obj->var2 = 5;
                            obj->frame=TURRET_FRAMES-1;
                        }
                    }
                } else if (obj->var2 >= 5 && obj->var2 <= 10) {
                    obj->var2++;
                    if (obj->var2 == 10) {
                        obj->var2 = 5;
                        obj->frame--;
                        if (obj->frame <= 0) {
                            obj->var2 = 0;
                            obj->frame=0;
                        }
                    }
                }
                break;
            default:
                break;
        }
        /* Turrets shoot ! */
        if (obj->type == Turret && obj->timer == 0) {
            turret_shoot(obj);
        }
        /* Draw the special */
        draw_special (obj);
        /* Decrement age if positive */
        if (obj->age > 0)
            obj->age--;
        if (obj->timer > 0)
            obj->timer--;
        /* Check if the object has expired.
         * If age is <0, the object has no time limit */
        next = list->next;
        if (obj->age == 0) {
            free (obj);
            if(list==specials)
                specials=dllist_remove(list);
            else
                dllist_remove(list);
        } else { /* Check for player collisions */
            ship_hit_special (list);
        }
        list = next;
    }
}

/*** Check if a projectile collides with level special ***/
int projectile_hit_special (Projectile * proj) {
    struct dllist *list = specials;
    int w, h;
    while (list) {
        SpecialObj *obj=list->data;
        w = 16;
        h = 16;
        if (obj->type != JumpGate) {   /* Currently, jumpgate is the only special that doesn't collide with projectiles */
            if (proj->x >= obj->x && proj->y >= obj->y) {
                if (proj->x <= obj->x + w
                    && proj->y <= obj->y + h) {
                    switch (obj->type) {
                    case WarpEntry:
                    case WarpExit:
                        if (proj->type != MegaBomb && proj->type != Rocket) {
                            list = list->next;
                            continue;
                        }
                        if (obj->link && obj->timer == 0) {
                            proj->x = obj->link->x + w / 2;
                            proj->y = obj->link->y + h / 2;
                            obj->timer = 10;
                            obj->link->timer = 10;
                        }
                        return 0;
                        break;
                    default:
                        obj->health -= 0.2;
                        if (obj->health <= 0) {
                            spawn_clusters (obj->x,
                                            obj->y, 10, Cannon);
                            spawn_clusters (obj->x,
                                            obj->y, 16,
                                            FireStarter);
                            add_explosion (obj->x, obj->y,
                                           Noproj);
                            playwave_3d(WAV_EXPLOSION2,obj->x,obj->y);
                            free (obj);
                            if(list==specials)
                                specials=dllist_remove(list);
                            else
                                dllist_remove(list);
                        }
                        break;
                    }
                    return 1;
                }
            }
        }
        list = list->next;
    }
    return 0;
}


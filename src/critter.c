/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : critter.c
 * Description : Critters that walk,swim or fly around the level. Most are passive, but some attack (soldiers and helicopters)
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

#include "console.h"
#include "particle.h"
#include "critter.h"
#include "player.h"
#include "level.h"
#include "game.h"
#include "list.h"
#include "ship.h"
#include "fs.h"

#include "audio.h"

#define BAT_FRAMES          3
#define BIRD_FRAMES         4
#define COW_FRAMES          6
#define FISH_FRAMES         2
#define INFANTRY_FRAMES	    1
#define HELICOPTER_FRAMES   2
#define CRITTER_ANGRY	35  /* How long does a critter stay angry */

typedef enum { FlyingCritter, GroundCritter, WaterCritter } CritterType;

struct Critter {
    int x, y;
    int x2, y2;
    int frame;
    int wait;
    SDL_Rect rect;
    Vector vector;
    double angle;
    int timer;
    int owner; /* So attacking critters know who not to shoot */
    int health;
    struct Ship *carried;
    int explode;
    int angry;
    CritterType type;
    ObjectType species;
};

static struct dllist *critters;

/* Number of limited critters */
static int soldier_count[4];
static int helicopter_count[4];

/* Critter images */
static SDL_Surface **critter_gfx[6];
static SDL_Surface *bat_attack;

/* Splatter gravity */
static Vector splt_gravity;
static Vector splt_gravity2;

/* Bat attack ! */
/* We dont bother with a linked list. */
/* MAX_CRITTERS is fine, since each bat can teasy only one ship at a time */
struct Bat_Attack {
    SDL_Rect src;
    SDL_Rect targ;
    int end;
    struct Critter *me;
} crit_bat_attack[MAX_CRITTERS];

/* Internally used function definitions */
static void recalc_target (struct Critter * critter);

/* The generic movement logic for ground critters */
static void ground_critter_movement (struct Critter * critter);
/* The generic attack logic for ground critters */
static void ground_critter_attack (struct Critter * crittter);
/* The generic movement logic for flying and swimming critters */
static void air_critter_movement (struct Critter * critter);
/* Special movement logic for helicopters */
static void helicopter_movement (struct Critter * critter, int recalc);
/* Special movement logic for bats */
static void bat_movement (struct Critter * critter, int hitwall); 
/* The attack critter for flying critters (helicopter) */
static void air_critter_attack (struct Critter * critter);     

static inline void draw_critter (struct Critter * critter);
/* Kill a critter, (splatter, remove from list) */
static struct dllist *kill_critter (struct dllist *critter); 

/* Get critter name */
static const char *critter2str(ObjectType critter) {
    switch(critter) {
        case OBJ_BIRD:
            return "bird";
        case OBJ_COW:
            return "cow";
        case OBJ_FISH:
            return "fish";
        case OBJ_BAT:
            return "bat";
        case OBJ_SOLDIER:
            return "soldier";
        case OBJ_HELICOPTER:
            return "helicopter";
        default:
            return "<not a critter>";
    }
}

/* Load critter data */
void init_critters (LDAT *datafile) {
    critters = NULL;
    /* Load critter gfx. Note ! The order in which they are loaded counts ! The order must be same as the enumerations in critter.h ! */
    /* Load bird frames */
    critter_gfx[0] =
        load_image_array (datafile, 0, T_ALPHA, "COW", 0, COW_FRAMES - 1);
    critter_gfx[1] =
        load_image_array (datafile, 0, T_ALPHA, "FISH", 0, FISH_FRAMES - 1);
    critter_gfx[2] =
        load_image_array (datafile, 0, T_ALPHA, "BIRD", 0, BIRD_FRAMES - 1);
    critter_gfx[3] =
        load_image_array (datafile, 0, T_ALPHA, "BAT", 0, BAT_FRAMES - 1);
    critter_gfx[4] =
        load_image_array (datafile, 0, T_COLORKEY, "INFANTRY", 0,
                INFANTRY_FRAMES - 1);
    critter_gfx[5] =
        load_image_array (datafile, 0, T_COLORKEY, "HELICOPTER", 0,
                          HELICOPTER_FRAMES - 1);
    bat_attack = load_image_ldat (datafile, 1, T_ALPHA, "BAT_ATTACK", 0);
    splt_gravity = makeVector (0, -WEAP_GRAVITY);
    splt_gravity2 = makeVector (0, -WEAP_GRAVITY / 4.0);
}

/* Clear critters from memory */
void clear_critters (void)
{
    dllist_free(critters,free);
    critters=NULL;
}

/* Add random number of critters of specified species in random locations */
static void add_random_critters(ObjectType species,int max) {
    if(max>0) {
        int r,count;
        count = rand()%max;
        for(r=0;r<count;r++) {
            struct Critter *critter = make_critter(species,-1,-1,-1);
            if(critter)
                add_critter(critter);
        }
    }
}

/* Add random and manually placed critters */
void prepare_critters (struct LevelSettings * settings)
{
    struct Critter *crit = NULL;
    struct dllist *objects = NULL;
    int r;
    if (level_settings.critters == 0)
        return;

    /* Add random critters (if any) */
    add_random_critters(OBJ_BIRD, level_settings.birds);
    add_random_critters(OBJ_COW, level_settings.cows);
    add_random_critters(OBJ_FISH, level_settings.fish);
    add_random_critters(OBJ_BAT, level_settings.bats);

    /* Reset critter counters */
    for (r = 0; r < 4; r++) {
        soldier_count[r] = 0;
        helicopter_count[r] = 0;
    }
    for (r = 0; r < level_settings.bats; r++) {
        crit_bat_attack[r].src.y = 0;
        crit_bat_attack[r].src.w = screen->w/2;
        crit_bat_attack[r].src.h = screen->h/2;
        crit_bat_attack[r].end = 0;
    }

    /* Add manually placed critters */
    if (settings)
        objects = settings->objects;
    while (objects) {
        struct LSB_Object *object = objects->data;
        if (object->type >= FIRST_CRITTER && object->type <= LAST_CRITTER) {
            crit = make_critter (object->type, object->x, object->y, -1);
            if (object->ceiling_attach)
                object->y -= critter_gfx[object->type][0]->h;
            if (object->value && object->type == OBJ_BAT) {
                crit->carried = (struct Ship *) 1;
                crit->frame = BAT_FRAMES - 1;
            }
            add_critter (crit);
        }
        objects = objects->next;
    }
}

struct Critter *make_critter (ObjectType species, int x, int y, int owner)
{
    struct Critter *newcritter = NULL;
    int loop, loop2, r, breakloop;
    unsigned char medium = 0;
    newcritter = malloc (sizeof (struct Critter));
    newcritter->health = 1;
    newcritter->explode = 0;
    newcritter->rect.x = 0;
    newcritter->rect.y = 0;
    newcritter->rect.w = critter_gfx[species-FIRST_CRITTER][0]->w;    /* All critter sprites should stay the same size */
    newcritter->rect.h = critter_gfx[species-FIRST_CRITTER][0]->h;
    switch (species) {
    case OBJ_HELICOPTER:
        newcritter->health = 3;
        newcritter->angle = 0;
        newcritter->rect.w /= 4;
        newcritter->rect.x = newcritter->rect.w * owner;
    case OBJ_BIRD:
        newcritter->type = FlyingCritter;
        medium = TER_FREE;
        break;
    case OBJ_BAT:
        newcritter->type = FlyingCritter;
        medium = TER_FREE;
        newcritter->health = 2;
        break;
    case OBJ_SOLDIER:
        newcritter->health = 1;
        newcritter->rect.w /= 4;
        newcritter->rect.x = newcritter->rect.w * owner;
    case OBJ_COW:
        newcritter->type = GroundCritter;
        break;
    case OBJ_FISH:
        newcritter->type = WaterCritter;
        medium = TER_WATER;
        break;
    default:
        fprintf(stderr,"Unhandled critter type %d\n",species);
        return NULL;
    }
    /* Find a place for the critter */
    if (x < 0 || y < 0) {
        loop = 0;
        if (newcritter->type == GroundCritter) {
            do {
                if (loop++ > 1000) {
                    fprintf(stderr,"Couldn't find a place for a %s after 1000 tries.\n", critter2str(species));
                    free (newcritter);
                    return NULL;
                }
                newcritter->x = 20 + rand () % (lev_level.width - 40);
                newcritter->y = 20 + rand () % (lev_level.height - 40);
                if(is_walkable(newcritter->x,newcritter->y)==0 &&
                        is_water(newcritter->x,newcritter->y)==0) {
                    do {
                        newcritter->y++;
                    } while(newcritter->y<=lev_level.height-20 &&
                            is_walkable(newcritter->x,newcritter->y)==0 &&
                            is_water(newcritter->x,newcritter->y)==0);
                } else {
                    newcritter->y=lev_level.height;
                }
            } while (newcritter->y>lev_level.height-20 ||
                    is_walkable(newcritter->x,newcritter->y)==0 ||
                    is_water(newcritter->x,newcritter->y));
            newcritter->y -= critter_gfx[species][0]->h+1;
        } else {
            loop2 = 0;
            breakloop = 0;
            while (1) {
                do {
                    newcritter->x = 20 + rand () % (lev_level.width - 40);
                    newcritter->y = 20 + rand () % (lev_level.height - 40);
                    loop++;
                    if (loop > 1000) {
                        printf("Couldn't find a place for a %s after 1000 tries.\n",
                            critter2str(species));
                        free (newcritter);
                        return NULL;
                    }
                } while (lev_level.solid[newcritter->x][newcritter->y] !=
                         medium);
                if (species == OBJ_BAT) {
                    /* bats are all asleep when the level begins */
                    for (r = 0; r < 100; r++) {
                        if (newcritter->y - r < 20)
                            break;
                        if (hit_solid
                            (newcritter->x + newcritter->rect.w / 2,
                             newcritter->y - r) > 0) {
                            newcritter->y -= r;
                            newcritter->carried = (struct Ship *) 1;
                            newcritter->frame = BAT_FRAMES - 1;
                            breakloop = 1;
                            break;
                        }
                    }
                    if (breakloop)
                        break;
                    loop2++;
                    if (loop2 > 20) {
                        printf("Couldn't find a place for a %s after 20000 tries.\n",
                                critter2str(species));
                        free (newcritter);
                        return NULL;
                    }
                } else {
                    break;
                }
            }
        }
    } else {
        newcritter->x = x;
        newcritter->y = y;
    }
    if (newcritter->type == GroundCritter) {
        newcritter->x2 = 0;
    } else {
        newcritter->x2 = newcritter->x;
        newcritter->y2 = newcritter->y;
        /*recalc_target(newcritter);*/
    }
    newcritter->vector = makeVector (0, 0);
    newcritter->species = species;
    newcritter->frame = 0;
    newcritter->wait = 4;
    newcritter->timer = 10;
    newcritter->owner = owner;
    newcritter->carried = NULL;
    newcritter->angry = 0;
    return newcritter;
}

/* Add a critter to the list */
void add_critter (struct Critter * newcritter) {
    if (newcritter->species == OBJ_SOLDIER) {
        if (soldier_count[newcritter->owner] > level_settings.soldiers
            && level_settings.soldiers) {
            struct dllist *list = critters;
            while (list) {
                if (((struct Critter*)list->data)->species == OBJ_SOLDIER
                    && ((struct Critter*)list->data)->owner == newcritter->owner) {
                    kill_critter (list);
                    break;
                }
                list = list->next;
            }
        }
        soldier_count[newcritter->owner]++;
    } else if (newcritter->species == OBJ_HELICOPTER) {
        if (helicopter_count[newcritter->owner] > level_settings.helicopters
            && level_settings.helicopters) {
            struct dllist *list = critters;
            while (list) {
                if (((struct Critter*)list->data)->species == OBJ_HELICOPTER
                    && ((struct Critter*)list->data)->owner == newcritter->owner) {
                    kill_critter (list);
                    break;
                }
                list = list->next;
            }
        }
        helicopter_count[newcritter->owner]++;
    }

    if (critters)
        dllist_append(critters,newcritter);
    else
        critters=dllist_append(critters,newcritter);
}

/* Critter gravity effects (caused by a GravityWell) */
void cow_storm (int x, int y)
{
    struct dllist *list = critters;

    while (list) {
        struct Critter *critter=list->data;
        if (abs (critter->x - x) < 100)
            if (abs (critter->y - y) < 100) {
                critter->vector.x -=
                    (critter->x - x) / (abs (critter->x - x) +
                                              0.01) / 1.3;
                critter->vector.y -=
                    (critter->y - y) / (abs (critter->y - y) +
                                              0.01) / 1.3;
            }
        list = list->next;
    }
}

/** Animate critters **/
void animate_critters (void) {
    struct dllist *list = critters;
    struct dllist *ships;
    int maxframes;
    int i1;
    double f1;
    while (list) {
        struct Critter *critter=list->data;
        switch (critter->species) {
        case OBJ_BIRD:
            maxframes = BIRD_FRAMES;
            break;
        case OBJ_BAT:
            maxframes = BAT_FRAMES - !critter->carried;
            break;
        case OBJ_COW:
            maxframes = (critter->x2 > 0) ? COW_FRAMES / 2 : COW_FRAMES;
            break;
        case OBJ_FISH:
            maxframes =
                (critter->x2 - critter->x >
                 0) ? FISH_FRAMES / 2 : FISH_FRAMES;
            break;
        case OBJ_HELICOPTER:
            maxframes =
                (critter->x2 - critter->x >
                 0) ? HELICOPTER_FRAMES / 2 : HELICOPTER_FRAMES;
            break;
        default:
            maxframes = 0;
            break;
        }
        critter->wait--;
        if (critter->angry)
            critter->angry--;
        /* Gravity effects, check if someone is operating a force field nearby */
        ships = ship_list;
        while (ships) {
            struct Ship *ship=ships->data;
            if (ship->shieldup) {
                f1 = hypot (abs (ship->x - critter->x),
                            abs (ship->y - critter->y));
                if (f1 < 100) {
                    critter->vector.x -=
                        ((ship->x - critter->x) / (abs (ship->x -
                                                    critter->x) +
                                               0.1)) / 1.3;
                    critter->vector.y -=
                        ((ship->y - critter->y) / (abs (ship->y -
                                                    critter->y) +
                                               0.1)) / 1.3;
                }
            } else if (ship->antigrav) {
                f1 = hypot (abs (ship->x - critter->x),
                            abs (ship->y - critter->y));
                if (f1 < 200) {
                    critter->vector.y -= 0.6;
                }
            }
            ships = ships->next;
        }
        /* Critters don't go thru walls */
        if (critter->x > lev_level.width - 20)
            critter->x = lev_level.width - 20;
        if (critter->y > lev_level.height - 20)
            critter->y = lev_level.height - 20;
        if (critter->type == FlyingCritter
            && lev_level.solid[critter->x +
                               critter->rect.w / 2][critter->y] !=
            TER_FREE) {
            critter->vector.x = 0;
            critter->vector.y = 0;
            if (critter->species == OBJ_HELICOPTER)
                helicopter_movement (critter, 1); /* Recalculate heading */
            else if (critter->species == OBJ_BAT)
                bat_movement (critter, 1);
        } else if (critter->type == WaterCritter
                   && is_walkable (critter->x, critter->y) > 0) {
            if (critter->vector.x > 5.0 || critter->vector.x > 5.0) {
                list = kill_critter (list);
                continue;
            }
            critter->vector.x = 0;
            critter->vector.y = 0;
        }
        /* Water critter physics */
        else if (critter->type == WaterCritter
                 && lev_level.solid[critter->x][critter->y] ==
                 TER_FREE) {
            critter->vector.y += 0.375;
            if (critter->vector.y > 6)
                critter->vector.y = 6;
            critter->vector.x /= 1.5;
        }
        /* Apply vectors */
        critter->y += Round (critter->vector.y);
        critter->x += Round (critter->vector.x);
        /* Make sure the critter doesn't leave the level boundaries */
        if (critter->x < 0)
            critter->x = 0;
        else if (critter->x >= lev_level.width)
            critter->x = lev_level.width - 1;
        if (critter->y < 0)
            critter->y = 0;
        else if (critter->y >= lev_level.height)
            critter->x = lev_level.height - 1;

        if (critter->type == GroundCritter) {     /* Ground critter physics */
            if (is_water (critter->x, critter->y)) {
                list = kill_critter (list);
                continue;
            }                   /* Ground types dont tolerate water */
            if (is_walkable (critter->x, critter->y + critter->rect.h)) {
                if (abs (critter->vector.y) >= 5) {        /* Critter hit the ground too fast */
                    list = kill_critter (list);
                    continue;
                }
                critter->vector.y = 0;
                critter->vector.x = 0;
            } else {
                critter->vector.y += 0.375;
                critter->vector.x /= 1.5;
                if (critter->vector.y > 6)
                    critter->vector.y = 6;
            }
        }
        /* Critter sprite animation */
        if (critter->wait == 0) {
            critter->wait = 4;
            if (!(critter->species == OBJ_BAT && critter->carried))
                critter->frame++;
            if (critter->frame >= maxframes) {
                if (critter->type == GroundCritter) {
                    if (critter->x2 < 0)
                        critter->frame = maxframes / 2;
                    else
                        critter->frame = 0;
                } else if (critter->type == WaterCritter
                           || critter->species == OBJ_HELICOPTER) {
                    if (critter->x2 < critter->x)
                        critter->frame = maxframes / 2;
                    else
                        critter->frame = 0;
                } else
                    critter->frame = 0;
            }
            /* Critter timer (used for attacking and such) */
            if (critter->timer > 0)
                critter->timer--;
            /* - Movement and attack logic starts here - */
            if (critter->type == GroundCritter) { /* Ground critter movement logic */
                if (critter->species == OBJ_SOLDIER
                    && critter->timer == 0) {
                    ground_critter_attack (critter);
                }
                ground_critter_movement (critter);
            } else {            /* Air & Water critter movement */
                /* Birds shed feathers when you hit them */
                if (critter->species == OBJ_BIRD) {
                    i1 = find_nearest_player (critter->x,
                                              critter->y,
                                              critter->owner, &f1);
                    if (f1 < 16) {
                        splatter (critter->x, critter->y,
                                  Feather);
                        critter->vector.x =
                            (critter->x - players[i1].ship->x) / 2.0;
                        critter->vector.y =
                            (critter->y - players[i1].ship->y) / 2.0;
                    }
                }
                /* Helicopters attack players  */
                if (critter->species == OBJ_HELICOPTER
                    && critter->timer == 0) {
                    air_critter_attack (critter);
                }
                /* Vectors decay */
                critter->vector.y /= 1.5;
                critter->vector.x /= 1.5;
                /* Movement logic */
                if (critter->species == OBJ_HELICOPTER)
                    helicopter_movement (critter, 0);
                else if (critter->species == OBJ_BAT)
                    bat_movement (critter, 0);
                else
                    air_critter_movement (critter);
            }
        }
        draw_critter (critter);
        list = list->next;
    }
}

/** Ground critter movement logic (this is used by all ground critters) **/
static void ground_critter_movement (struct Critter * critter)
{
    int dx, dy;
    struct Ship *ship;
    if (critter->carried == NULL) {
        if (critter->x2 == 0) {
            do {
                dx = rand () % 50;
                if ((rand () & 0x01) == 0)
                    dx = 0 - dx;
            } while (dx == 0);
            critter->x2 = dx;
        }
        if (critter->x2 < 0) {
            critter->x2++;
            critter->x--;
            if (critter->x < 10) {
                critter->x++;
                critter->x2 = 10;
            }
            dy = find_nearest_terrain (critter->x, critter->y,
                                       critter->rect.h);
            if (dy == -1) {
                critter->x++;
                critter->x2 = 0;
            } else
                critter->y = dy;
        } else {
            critter->x2--;
            critter->x++;
            if (critter->x > lev_level.width - 10) {
                critter->x--;
                critter->x2 = -10;
            }
            dy = find_nearest_terrain (critter->x, critter->y,
                                       critter->rect.h);
            if (dy == -1) {
                critter->x--;
                critter->x2 = 0;
            } else
                critter->y = dy;
        }
    }
    /* Ground critters can be picked up */
    if (critter->carried)
        ship = hit_ship (critter->x, critter->y, NULL, 25);
    else
        ship =
            hit_ship (critter->x, critter->y, NULL,
                      (critter->species == OBJ_SOLDIER) ? 5 : 13);
    if (ship) {
        if (critter->carried
            || (critter->carried == NULL && ship->carrying == 0
                && ship->visible)) {
            critter->x = ship->x - critter->rect.w / 2;
            critter->y = ship->y - critter->rect.h;
            critter->carried = ship;
            critter->vector.y = 0;
            ship->carrying = 1;
        }
    } else {
        if (critter->carried) {
            critter->carried->carrying = 0;
            critter->carried = NULL;
        }
    }

}

/*** Critter shooting ***/
static void critter_shoot(struct Critter *critter,int targx,int targy,double inaccuracy) {
    Projectile *p;
    double a;
    int x,y;

    x=critter->x+critter->rect.w/2;
    y=critter->y+critter->rect.h/2;
    a = atan2(targx - x, targy - y)+inaccuracy;

    p = make_projectile(x+sin(a)*3.0,y+cos(a)*3.0,
            makeVector(-sin(a)*1.25, -cos(a)*1.25));
    p->type=Handgun;
    p->primed = 1;
    add_projectile (p);
}

/** Ground critter attack logic (this is used by Infantry) **/
static void ground_critter_attack (struct Critter * critter) {
    struct dllist *cl;
    double f1;
    int i1;
    /* Attack player */
    i1 = find_nearest_player (critter->x, critter->y, critter->owner, &f1);
    if (i1>=0 && f1 < 120 && (players[i1].ship->visible || players[i1].ship->tagged)
        && players[i1].ship->y<critter->y) {
        critter_shoot(critter,players[i1].ship->x,players[i1].ship->y,0);
        critter->timer = 5;
    } else {                    /* Attack helicopters */
        cl = critters;
        while (cl) {
            struct Critter *clc=cl->data;
            if (clc->species == OBJ_HELICOPTER
                && player_teams[clc->owner] !=
                player_teams[critter->owner]
                && clc->y > critter->y) {
                if (critter->y - clc->y < 100
                    && abs (clc->x - critter->x) < 90) {
                    critter_shoot(critter,clc->x,clc->y,(rand()%5)/10.0);
                    critter->timer = 5;
                    break;
                }
            }
            cl = cl->next;
        }
    }
}

/** Air and water critter movement logic (this is used by birds and fish) **/
static void air_critter_movement (struct Critter * critter)
{
    int dx, dy;
    dx = critter->x - critter->x2;
    dy = critter->y - critter->y2;
    if ((dx > -5 && dx < 5) && (dy > -5 && dy < 5))
        recalc_target (critter);
    if (critter->x > critter->x2)
        critter->x -= 2;
    else
        critter->x += 2;
    if (critter->y > critter->y2)
        critter->y -= 2;
    else
        critter->y += 2;
}

/** Bat movement logic **/
static void bat_movement (struct Critter * critter, int hitwall)
{
    int dx, dy, p, b;
    int rx, ry;
    double f;
    Vector v;
    rx = critter->x + critter->rect.w / 2;
    ry = critter->y + critter->rect.h / 2;
    if (critter->carried
        && (lev_level.solid[rx][critter->y] == TER_FREE
            || lev_level.solid[rx][critter->y] == TER_TUNNEL))
        critter->angry = 1;
    if (critter->angry && critter->carried) {   /* Just woke up */
        critter->carried = 0;
        critter->frame = 0;
    }
    p = find_nearest_player (rx, ry, critter->owner, &f);
    if (f < 60) {
        dx = players[p].ship->x - rx;
        dy = players[p].ship->y - ry;
        if (f < 20 && critter->carried == 0 && bat_attack && players[p].ship) {
            /* Bat attack! */
            for (b = 0; b < level_settings.bats; b++) {
                if (crit_bat_attack[b].me == critter
                    && crit_bat_attack[b].end)
                    break;
                if (crit_bat_attack[b].end == 0) {
                    int dx, dy;
                    dx = cam_rects[p].w/2 - rand () % cam_rects[p].w;
                    dy = cam_rects[p].h/2 - rand () % cam_rects[p].h;
                    crit_bat_attack[b].targ.x = lev_rects[p].x + dx;
                    crit_bat_attack[b].targ.y = lev_rects[p].y + dy;
                    crit_bat_attack[b].src.x = 0;
                    crit_bat_attack[b].src.y = 0;
                    crit_bat_attack[b].src.w = bat_attack->w;
                    crit_bat_attack[b].src.h = bat_attack->h;
                    if (dx < 0) {
                        crit_bat_attack[b].src.x -= dx;
                        crit_bat_attack[b].src.w = bat_attack->w + dx;
                        crit_bat_attack[b].targ.x -= dx;
                    } else if(dx+bat_attack->w>cam_rects[p].w) {
                        crit_bat_attack[b].src.w -= dx + bat_attack->w - cam_rects[p].w;
                    }
                    if (dy < 0) {
                        crit_bat_attack[b].src.y = -dy;
                        crit_bat_attack[b].src.h = bat_attack->h + dy;
                        crit_bat_attack[b].targ.y -= dy;
                    } else if(dy+bat_attack->h>cam_rects[p].h) {
                        crit_bat_attack[b].src.h -= dy + bat_attack->h - cam_rects[p].h;
                    }
                    crit_bat_attack[b].me = critter;
                    crit_bat_attack[b].end = 10;
                    break;
                }
            }
        }
    } else {
        dx = critter->x2 - critter->x;
        dy = critter->y2 - critter->y;
    }
    if (hitwall) {
        if (hit_solid (rx, critter->y + 1) || critter->angry)
            hitwall = 2;
        else {
            critter->carried = (struct Ship *) 1;
            critter->frame = BAT_FRAMES - 1;
            critter->vector.x = 0;
            critter->vector.y = 0;
            return;
        }
    }
    if (hitwall == 2 || ((dx > -5 && dx < 5) && (dy > -5 && dy < 5))) {
        do {
            do {
                dx = 120 - rand () % 240;
            } while (critter->x + dx >= lev_level.width
                     || critter->x + dx <= 0);
            do {
                dy = 120 - rand () % 240;
            } while (critter->y + dy >= lev_level.height
                     || critter->y + dy <= 0);
        } while (lev_level.solid[critter->x + dx][critter->y + dy] !=
                 TER_FREE);
        for (p = 0; p < 100; p++) {     /* Bats seek sleeping places */
            if (critter->y + dy - p < 10)
                break;
            if (hit_solid
                (critter->x + (critter->rect.w / 2) + dx,
                 critter->y + dy - p) > 0) {
                dy -= p;
                break;
            }
        }
        critter->x2 = critter->x + dx;
        critter->y2 = critter->y + dy;
    }
    critter->angle = atan2 (dx, dy);
    v.x = sin (critter->angle);
    v.y = cos (critter->angle);
    critter->vector = addVectors (&critter->vector, &v);
}

/** Helicopter movement logic **/
static void helicopter_movement (struct Critter * critter, int recalc)
{
    int dx, dy, p;
    double f;
    Vector v;
    p = find_nearest_player (critter->x, critter->y, critter->owner, &f);
    if (f < 200) {
        if (f > 30) {
            dx = players[p].ship->x - critter->x;
            dy = players[p].ship->y - critter->y;
        } else {
            dx = critter->x - players[p].ship->x;
            dy = critter->y - players[p].ship->y;
        }
    } else {
        dx = critter->x2 - critter->x;
        dy = critter->y2 - critter->y;
    }
    if (recalc || ((dx > -5 && dx < 5) && (dy > -5 && dy < 5))) {
        do {
            do {
                dx = 120 - rand () % 240;
            } while (critter->x + dx >= lev_level.width
                     || critter->x + dx <= 0);
            do {
                dy = 120 - rand () % 240;
            } while (critter->y + dy >= lev_level.height
                     || critter->y + dy <= 0);
        } while (lev_level.solid[critter->x + dx][critter->y + dy] !=
                 TER_FREE);
        critter->x2 = critter->x + dx;
        critter->y2 = critter->y + dy;
    }
    critter->angle = atan2 (dx, dy);
    v.x = sin (critter->angle);
    v.y = cos (critter->angle);
    critter->vector = addVectors (&critter->vector, &v);
}

/** Air critter attack logic (this is used by helicopters) **/
static void air_critter_attack (struct Critter * critter) {
    struct dllist *cl = NULL;
    int i1, p;
    double f1;
    i1 = find_nearest_player (critter->x, critter->y, critter->owner, &f1);
    if (f1 < 160 && (players[i1].ship->visible || players[i1].ship->tagged)
        && players[i1].ship) {
        critter_shoot(critter,players[i1].ship->x, players[i1].ship->y,0);
        critter->timer = 5;
    } else { /* No ships around ? How about infantry ? */
        cl = critters;
        while (cl) {
            struct Critter *clc=cl->data;
            if (clc->species == OBJ_SOLDIER
                && player_teams[clc->owner] !=
                player_teams[critter->owner]) {
                if (clc->y < critter->y) {
                    cl = cl->next;
                    continue;
                }
                if (clc->y - critter->y < 120
                    && abs (clc->x - critter->x) < 90) {
                    critter_shoot(critter,clc->x,clc->y,(rand()%5)/10.0);
                    critter->timer = 5;
                    break;
                }
            }
            cl = cl->next;
        }
        if (cl == NULL) { /* Ok, how about pilots */
            p = find_nearest_pilot(critter->x,critter->y,critter->owner,&f1);
            if(p>=0 && f1<=100.0) {
                critter_shoot(critter,players[p].pilot.x,players[p].pilot.y,(rand()%5)/10.0);
                critter->timer = 6;
            }
        }
    }
}

/** Draw critters **/
static inline void draw_critter (struct Critter * critter)
{
    SDL_Rect rect, rect2;
    SDL_Surface *sprite;
    int px, py, p;
    sprite = critter_gfx[critter->species - FIRST_CRITTER][critter->frame];
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            rect.x = critter->x - cam_rects[p].x + lev_rects[p].x;
            rect.y = critter->y - cam_rects[p].y + lev_rects[p].y;
            if ((rect.x > lev_rects[p].x - critter->rect.w
                 && rect.x < lev_rects[p].x + cam_rects[p].w)
                && (rect.y > lev_rects[p].y - critter->rect.h
                    && rect.y < lev_rects[p].y + cam_rects[p].h)) {
                rect2 =
                    cliprect (rect.x, rect.y, critter->rect.w,
                              critter->rect.h, lev_rects[p].x, lev_rects[p].y,
                              lev_rects[p].x + cam_rects[p].w,
                              lev_rects[p].y + cam_rects[p].h);
                rect2.x += critter->rect.x;
                /*rect2.y+=critter->rect.y; */
                if (rect.x < lev_rects[p].x)
                    rect.x = lev_rects[p].x;
                if (rect.y < lev_rects[p].y)
                    rect.y = lev_rects[p].y;
                if (critter->species == OBJ_SOLDIER) {
                    if (players[p].ship == NULL) {
                        px = players[p].pilot.x;
                        py = players[p].pilot.y;
                    } else {
                        px = players[p].ship->x;
                        py = players[p].ship->y;
                    }
                    if (abs (critter->x - px) < 70
                        && abs (critter->y - py) < 70)
                        SDL_BlitSurface (sprite, &rect2, screen, &rect);
                } else
                    SDL_BlitSurface (sprite, &rect2, screen, &rect);
            }
        }
    }
}

/*** Draw the bat attack ***/
void draw_bat_attack ()
{
    int b;
    if (!bat_attack)
        return;
    for (b = 0; b < level_settings.bats; b++) {
        if (crit_bat_attack[b].end == 0)
            continue;
        crit_bat_attack[b].end--;
        SDL_BlitSurface (bat_attack, &crit_bat_attack[b].src, screen,
                         &crit_bat_attack[b].targ);
    }
}

static void recalc_target (struct Critter * critter)
{
    unsigned char terrain = 0;
    int uh_oh = 0;
    switch (critter->type) {
    case FlyingCritter:
        terrain = TER_FREE;
        break;
    case GroundCritter:
        terrain = TER_GROUND;
        break;
    case WaterCritter:
        terrain = TER_WATER;
        break;
    }
    do {
        do {
            if (uh_oh == 100)
                return;
            critter->x2 = critter->x + ((rand () % 100) - 50);
            critter->y2 = critter->y + ((rand () % 100) - 50);
            uh_oh++;
        } while (critter->x2 < 10 || critter->y2 < 10
                 || critter->x2 > lev_level.width - 10
                 || critter->y2 > lev_level.height - 10);
    } while (lev_level.solid[critter->x2][critter->y2] != terrain);
}

int hit_critter (int x, int y, ProjectileType proj)
{
    struct dllist *list = critters;
    while (list) {
        struct Critter *critter=list->data;
        if (x >= critter->x && y >= critter->y)
            if (x <= critter->x + critter->rect.w
                && y <= critter->y + critter->rect.h) {
                if (proj == Plastique) {
                    critter->explode = 1;
                } else {
                    if (!
                        (critter->species == OBJ_BAT
                         && critter->carried))
                        critter->health--;
                    critter->angry = CRITTER_ANGRY;
                    if (proj == Spear || proj == Acid)
                        critter->health = 0;
                    if (critter->health == 0)
                        kill_critter (list);
                    else if (critter->species != OBJ_HELICOPTER)
                        splatter (critter->x, critter->y,
                                  SomeBlood);
                }
                return 1;
            }
        list = list->next;
    }
    return 0;
}

int find_nearest_terrain (int x, int y, int h)
{
    int r, ry;
    ry = y + h;
    for (r = 0; r < 15; r++) {
        if (ry + r - 1 < lev_level.height && is_walkable (x, ry + r)
            && (lev_level.solid[x][ry + r - 1] == TER_FREE
                || lev_level.solid[x][ry + r - 1] == TER_TUNNEL
                || lev_level.solid[x][ry + r - 1] == TER_WALKWAY))
            return y + r;
        else if (ry - r - 1 > 0 && is_walkable (x, ry - r)
                 && (lev_level.solid[x][ry - r - 1] == TER_FREE
                     || lev_level.solid[x][ry - r - 1] == TER_TUNNEL
                     || lev_level.solid[x][ry - r - 1] == TER_WALKWAY))
            return y - r;
    }
    return -1;
}

#if 0
int find_nearest_terrain(int x,int y,int h) {
	  int r;
	    for(r=0;r<15;r++) {
		        if(is_walkable(x,y+h+r) && lev_level.solid[x][y+h+r-1]==TER_FREE) return y+r;
			    else if(is_walkable(x,y+h-r) && lev_level.solid[x][y+h-r-1]==TER_FREE) return y-r;
			      }
	      return -1;
}
#endif

static struct dllist *kill_critter (struct dllist *list) {
    struct Critter *critter=list->data;
    struct dllist *next=list->next;
    if (critter->species == OBJ_HELICOPTER || critter->explode) {
        spawn_clusters (critter->x + critter->rect.w / 2,
                        critter->y + critter->rect.h / 2,
                        (critter->explode) ? 16 : 6, Cannon);
        if (critter->explode)
            spawn_clusters (critter->x, critter->y, 6,
                            Napalm);
    } else {
        splatter (critter->x +
                  critter_gfx[critter->species][0]->w / 2,
                  critter->y +
                  critter_gfx[critter->species][0]->h / 2, Blood);
        if (critter->species == OBJ_BIRD)
            splatter (critter->x +
                      critter_gfx[critter->species][0]->w / 2,
                      critter->y +
                      critter_gfx[critter->species][0]->h / 2,
                      LotsOfFeathers);
    }
    if (critter->species == OBJ_COW)
        playwave_3d (WAV_CRITTER1, critter->x, critter->y);
    else if (critter->species == OBJ_BIRD)
        playwave_3d (WAV_CRITTER2, critter->x, critter->y);
    else if (critter->species == OBJ_SOLDIER)
        soldier_count[critter->owner]--;
    else if (critter->species == OBJ_HELICOPTER)
        helicopter_count[critter->owner]--;
    if (critter->carried > (struct Ship *) 0x1)
        critter->carried->carrying = 0;

    free (critter);
    if(list==critters)
        critters=dllist_remove(list);
    else
        dllist_remove(list);
    return next;
}

void splatter (int x, int y, SplatterType type)
{
    Projectile *sp;
    double angle, incr, r = 1.0, d;
    if (type == Blood)
        r = 17.0;
    else if (type == SomeBlood)
        r = 6.0;
    else if (type == Feather)
        r = 3.0;
    else if (type == LotsOfFeathers)
        r = 6.0;
    incr = (2.0 * M_PI) / r;    /* How badly it gets splattered */
    angle = 0;
    d = (rand () % 314) / 100.0;
    while (angle < 2.0 * M_PI) {
        r = (rand () % 6) / 10.0;
        sp = make_projectile (x, y,
                              makeVector (sin (angle + r + d),
                                          cos (angle + r + d)));
        sp->vector.x /= 2.5;
        sp->vector.y /= 2.5;
        sp->type = Decor;
        sp->primed = 3;         /* So we seem them at least for a while */
        sp->wind_affects = 1;
        switch (type) {
        case Blood:
        case SomeBlood:
            sp->gravity = &splt_gravity;
            sp->color = col_red;
            break;
        case Feather:
        case LotsOfFeathers:
            sp->gravity = &splt_gravity2;
            sp->color = col_transculent;
            sp->maxspeed = 1.0;
            break;
        }
        add_projectile (sp);
        angle += incr;
    }
}

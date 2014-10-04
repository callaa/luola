/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
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
#include "critter.h"
#include "decor.h"
#include "player.h"
#include "level.h"
#include "game.h"
#include "list.h"
#include "ship.h"
#include "fs.h"

#include "audio.h"

/* List of critters */
struct dllist *critter_list;

/* Number of limited critters */
static int soldier_count[4];
static int helicopter_count[4];

/* Critter images */
static SDL_Surface **cow_gfx;
static SDL_Surface **bird_gfx;
static SDL_Surface **fish_gfx;
static SDL_Surface **bat_gfx;
static SDL_Surface **soldier_gfx;
static SDL_Surface **helicopter_gfx;
static SDL_Surface *bat_attack;
static SDL_Surface *iceblock;

/* Number of frames in critter animations */
static int cow_frames;
static int bird_frames;
static int fish_frames;
static int bat_frames;
static int soldier_frames;
static int helicopter_frames;

/* Bat attack! */
struct BatAttack {
    SDL_Rect src;
    SDL_Rect targ;
    int end;
    struct Critter *me;
} crit_bat_attack[16];

/* Load critter data */
void init_critters (LDAT *datafile) {
    cow_gfx =
        load_image_array (datafile, 0, T_ALPHA, "COW", &cow_frames);
    fish_gfx =
        load_image_array (datafile, 0, T_ALPHA, "FISH", &fish_frames);
    bird_gfx =
        load_image_array (datafile, 0, T_ALPHA, "BIRD", &bird_frames);
    bat_gfx =
        load_image_array (datafile, 0, T_ALPHA, "BAT", &bat_frames);
    soldier_gfx =
        load_image_array (datafile, 0, T_COLORKEY, "INFANTRY", &soldier_frames);
    helicopter_gfx =
        load_image_array (datafile, 0, T_COLORKEY, "HELICOPTER", &helicopter_frames);
    bat_attack = load_image_ldat (datafile, 1, T_ALPHA, "BAT_ATTACK", 0);
    iceblock = load_image_ldat (datafile, 1, T_ALPHA, "ICE", 0);
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

    /* Clear old critters */
    dllist_free(critter_list,free);
    critter_list=NULL;

    /* Stop here if critters are disabled */
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
    for (r = 0; r < sizeof(crit_bat_attack)/sizeof(struct BatAttack); r++) {
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
            if(crit) {
#if 0
                if (object->ceiling_attach)
                    critter->physics.y -= critter->gfx_rect.h;
#endif
                add_critter (crit);
            }
        }
        objects = objects->next;
    }
}

/* Splash of blood */
static void splatter(struct Critter *critter) {
    add_splash(critter->physics.x, critter->physics.y,5.0,16,
            critter->physics.vel,make_blood);
}

/* Splash of blood and ice */
static void shatter(struct Critter *critter) {
    add_splash(critter->physics.x, critter->physics.y,5.0,8,
            critter->physics.vel,make_blood);
    add_splash(critter->physics.x, critter->physics.y,5.0,8,
            critter->physics.vel,make_snowflake);
}

/* Timer function: change ground critter walking direction */
static void gc_dosomething(struct Critter *critter) {
    /* Decisions, 0 stay still, 1 walk left, 2 walk right */
    int decision = rand()%3;
    switch(decision) {
        case 0: critter->walker.walking = 0; break;
        case 1:
                if(critter->walker.walking>0)
                    critter->cornered+=1;
                critter->walker.walking = -1;
                break;
        case 2:
                if(critter->walker.walking<0)
                    critter->cornered+=1;
                critter->walker.walking = 1;
                break;
    }
    /* Time until next decision */
    critter->timer = 1+rand()%60;
}

/* Ground critter timer function: flee from any nearby enemy player */
static void gc_flee(struct Critter *critter) {
    if(critter->ff>0) {
        double distance;
        int enemy = find_nearest_enemy(critter->walker.physics.x,
                critter->walker.physics.y, critter->owner, &distance);
        if(distance<200.0) {
            critter->walker.walkspeed = 3;
            if(players[enemy].ship->physics.x < critter->walker.physics.x) {
                if(critter->walker.walking<0)
                    critter->cornered+=1;
                critter->walker.walking = 1;
            } else {
                if(critter->walker.walking>0)
                    critter->cornered+=1;
                critter->walker.walking = -1;
            }
        } else {
            critter->walker.walkspeed = 1;
        }
        critter->ff--;
        critter->timer = 1;
    } else {
        critter->walker.walkspeed = 1;
        critter->timerfunc = gc_dosomething;
        critter->timer = 2;
    }

}

/* A bird dies in a splash of blood and feathers */
static void bird_die(struct Critter *critter) {
    add_splash(critter->physics.x, critter->physics.y,5.0, 16,
            critter->physics.vel,make_blood);
    add_splash(critter->physics.x, critter->physics.y,3.0, 24,
            critter->physics.vel,make_feather);
    playwave_3d (WAV_CRITTER2, critter->physics.x, critter->physics.y);
}

/* Helicopters explode */
static void helicopter_die(struct Critter *critter) {
    spawn_clusters (critter->physics.x + critter->gfx_rect.w / 2,
                        critter->physics.y + critter->gfx_rect.h / 2,
                        5.6, 6, make_bullet);
}

/* Ground critter dies and alerts others nearby */
static void gc_die(struct Critter *critter) {
    struct dllist *lst=critter_list;
    splatter(critter);
    while(lst) {
        struct Critter *c=lst->data;
        if(c!=critter && c->type==GROUNDCRITTER &&
                fabs(c->physics.x-critter->physics.x)<250 &&
                fabs(c->physics.y-critter->physics.y)<250)
        {
            if(c->physics.x<critter->physics.x)
                c->walker.walking = -1;
            else
                c->walker.walking = 1;
            c->ff = 15 + rand()%30;
            c->timerfunc = gc_flee;
            c->timer = 0;
        }
        lst=lst->next;
    }
}

/* Air/water critter timer function: search a new target to go to */
static void ac_searchtarget(struct Critter *critter) {
    unsigned int loops=0;
    int newx,newy,tmp;
    int oldx = Round(critter->physics.x);
    int oldy = Round(critter->physics.y);

    /* Search for a new target that doesn't go thru solid terrain */
    do {
        newx = oldx + 200-rand()%400;
        newy = oldy + 200-rand()%400;
    } while(((critter->type==AIRCRITTER?is_water(newx,newy):is_free(newx,newy))
            || hit_solid_line(oldx,oldy,newx,newy,&tmp,&tmp)
            != (critter->type==AIRCRITTER?TER_FREE:TER_WATER))
            && loops++<100);

    critter->flyer.targx = newx;
    critter->flyer.targy = newy;
    
    critter->timer = 2*GAME_SPEED + rand()%(5*GAME_SPEED);
}

/* Shoot */
static int critter_shoot(struct Critter *critter, double angle) {
    double x = critter->physics.x + cos(angle)* critter->physics.radius;
    double y = critter->physics.y + sin(angle)* critter->physics.radius;
    if(is_solid(Round(x),Round(y))) return 0;
    Vector mvel = {cos(angle) * 10, sin(angle)*10};
    add_projectile(make_bullet(x,y,addVectors(critter->physics.vel,mvel)));
    return 1;
};

/* Find the nearest enemy critter */
/* Critters are identified by their graphics */
static struct Critter *find_enemy_critter(float x,float y,int owner, double *distance,SDL_Surface **gfx) {
    struct dllist *ptr = critter_list;
    struct Critter *nearest = NULL;
    double dist = 999999;
    while(ptr) {
        struct Critter *e = ptr->data;
        if(e->gfx == gfx && same_team(e->owner,owner)==0) {
            double d=hypot(e->physics.x-x,e->physics.y-y);
            if(d<dist) {
                dist=d;
                nearest=e;
            }
        }
        ptr=ptr->next;
    }
    if(distance)
        *distance = dist;
    return nearest;
}

/* Search for a target to shoot at */
/* Set airborne to zero to limit targets to those that can be easily */
/* shot at from ground */
static int find_target(float x, float y, int owner, float *targx, float *targy,
        double *dist, int airborne) {
    double distance;
    int eplr;
    struct Critter *ec;
    /* First priority, enemy ships */
    eplr = find_nearest_enemy(x,y, owner, &distance);
    if(distance < 120.0) {
        *targx = players[eplr].ship->physics.x;
        *targy = players[eplr].ship->physics.y;
        if(dist) *dist = distance;
        return 1;
    }
    /* Second priority, enemy helicopters */
    ec = find_enemy_critter(x,y, owner ,&distance,helicopter_gfx);
    if(distance < 120.0) {
        *targx = ec->physics.x;
        *targy = ec->physics.y;
        if(dist) *dist = distance;
        return 1;
    }
    if(airborne) {
        /* Third priority, airborne only, enemy soldiers */
        ec = find_enemy_critter(x,y, owner,&distance,soldier_gfx);
        if(distance < 120.0) {
            *targx = ec->physics.x;
            *targy = ec->physics.y;
            if(dist) *dist = distance;
            return 1;
        }
        /* Fourth priority, airborne only, enemy pilots */
        eplr = find_nearest_pilot(x,y,owner,&distance);
        if(distance < 120.0) {
            *targx = players[eplr].pilot.walker.physics.x;
            *targy = players[eplr].pilot.walker.physics.y;
            if(dist) *dist = distance;
            return 1;
        }
    }
    return 0;
}

/* Search for enemies and shoot */
static void soldier_animate(struct Critter *soldier) {
    if(soldier->cooloff>0)
        soldier->cooloff--;
    else if(soldier->ff==0 || soldier->cornered>60.0) {
        float targx,targy;
        if(find_target(soldier->physics.x, soldier->physics.y,
                    soldier->owner, &targx, &targy, NULL, 0))
        {
            if(critter_shoot(soldier,atan2(targy-soldier->physics.y,targx-soldier->physics.x))) {
                soldier->walker.walking = 0;
                if(soldier->cornered>70.0) /* Shoot like mad when cornered */
                    soldier->cooloff = 0.15*GAME_SPEED;
                else
                    soldier->cooloff = 0.7*GAME_SPEED;
            }
        }
    }
}

/* Search for enemies and shoot */
static void helicopter_animate(struct Critter *hc) {
    if(hc->cooloff>0)
        hc->cooloff--;
    else if(hc->ff==0) {
        double distance;
        float targx,targy;
        if(find_target(hc->physics.x,hc->physics.y, hc->owner, &targx, &targy,
                    &distance, 1))
        {
            if(distance>60.0) {
                hc->flyer.targx = targx;
                hc->flyer.targy = targy;
            }
            if(distance<150.0) {
                if(critter_shoot(hc,atan2(targy-hc->physics.y,targx-hc->physics.x))) {
                    hc->cooloff = 0.7*GAME_SPEED;
                }
            }
        }
    }
}

/* Bats seek out a place to perch */
static void bat_seekground(struct Critter *bat) {
    double a = (rand()%3141)/1000.0;
    int targx = Round(bat->physics.x) + cos(a)*150;
    int targy = Round(bat->physics.y) - sin(a)*150;

    hit_solid_line(Round(bat->physics.x),Round(bat->physics.y),
            targx,targy,&targx,&targy);
    bat->flyer.targx = targx;
    bat->flyer.targy = targy;
}

/* Bat attack. Set position on player viewport where bat image */
/* will be drawn in draw_bat_attack() */
static void bat_harass_player(struct Critter *bat, int plr) {
    int b=0;
    for(;b<sizeof(crit_bat_attack)/sizeof(struct BatAttack);b++) {
        if(crit_bat_attack[b].me == bat && crit_bat_attack[b].end)
            break;
        if (crit_bat_attack[b].end == 0) {
            int dx, dy;
            dx = cam_rects[plr].w/2 - rand () % cam_rects[plr].w;
            dy = cam_rects[plr].h/2 - rand () % cam_rects[plr].h;
            crit_bat_attack[b].targ.x = viewport_rects[plr].x + dx;
            crit_bat_attack[b].targ.y = viewport_rects[plr].y + dy;
            crit_bat_attack[b].src.x = 0;
            crit_bat_attack[b].src.y = 0;
            crit_bat_attack[b].src.w = bat_attack->w;
            crit_bat_attack[b].src.h = bat_attack->h;
            if (dx < 0) {
                crit_bat_attack[b].src.x -= dx;
                crit_bat_attack[b].src.w = bat_attack->w + dx;
                crit_bat_attack[b].targ.x -= dx;
            } else if(dx+bat_attack->w>cam_rects[plr].w) {
                crit_bat_attack[b].src.w -= dx + bat_attack->w - cam_rects[plr].w;
            }
            if (dy < 0) {
                crit_bat_attack[b].src.y = -dy;
                crit_bat_attack[b].src.h = bat_attack->h + dy;
                crit_bat_attack[b].targ.y -= dy;
            } else if(dy+bat_attack->h>cam_rects[plr].h) {
                crit_bat_attack[b].src.h -= dy + bat_attack->h - cam_rects[plr].h;
            }
            crit_bat_attack[b].me = bat;
            crit_bat_attack[b].end = 10;
            break;
        }
    }
}

/* Bats stay mainly still unless disturbed */
static void bat_animate(struct Critter *bat) {
    if(is_breathable(Round(bat->physics.x),Round(bat->physics.y)-1)) {
        /* Seek out nearby ships or ground */
        double d;
        struct Ship *targ = find_nearest_ship(bat->physics.x,bat->physics.y,NULL,&d);
        if(d<200.0) {
            bat->flyer.targx = targ->physics.x;
            bat->flyer.targy = targ->physics.y;
            if(d<20.0) {
                int p=0;
                for(;p<4;p++) {
                    if(players[p].ship == targ) {
                        bat_harass_player(bat,p);
                        break;
                    }
                }
            }
        } else if(bat->timer<0) {
            bat->timer = 2*GAME_SPEED + rand()%(2*GAME_SPEED);
        }
    }
}

/* Make a basic critter skeleton */
static struct Critter *make_base(SDL_Surface **gfx) {
    struct Critter *c = malloc(sizeof(struct Critter));
    if(!c) {
        perror(__func__);
        return NULL;
    }

    c->gfx = gfx;
    c->gfx_rect.x = 0;
    c->gfx_rect.y = 0;
    c->gfx_rect.w = gfx[0]->w;
    c->gfx_rect.h = gfx[0]->h;
    c->frame = 0;
    c->bidir = 1;

    c->health = 0.01;
    c->owner = -1;
    c->ship = 1;
    c->frozen = 0;

    c->timer=-1;
    c->ff = 0;
    c->cooloff = 0;
    c->cornered = 0;

    c->animate = NULL;
    
    return c;
}

/* Create a basic ground critter */
static struct Critter *make_base_gc(SDL_Surface **gfx,float x,float y) {
    struct Critter *c = make_base(gfx);
    c->type = GROUNDCRITTER;
    init_walker(&c->walker);
    c->physics.x = x;
    c->physics.y = y;
    c->physics.radius = 6;
    c->physics.mass = 12;
    c->walker.walking = rand()%2?-1:1;
    c->walker.walkspeed = 1;
    c->walker.slope = 5;
    c->ship = 0;

    c->timer = 1;
    c->timerfunc = gc_dosomething;
    c->die = gc_die;
    return c;
}

/* Make a cow */
static struct Critter *make_cow(float x,float y) {
    struct Critter *cow = make_base_gc(cow_gfx,x,y);
    cow->frames = cow_frames;
    return cow;
}

/* Make a soldier */
static struct Critter *make_soldier(float x,float y,int owner) {
    struct Critter *soldier = make_base_gc(soldier_gfx,x,y);
    soldier->frames = soldier_frames;
    soldier->walker.slope=10;
    soldier->gfx_rect.w/=4;
    soldier->gfx_rect.x = soldier->gfx_rect.w*owner;
    soldier->animate = soldier_animate;
    soldier->owner=owner;

    return soldier;
}

/* Create a basic air critter */
static struct Critter *make_base_ac(SDL_Surface **gfx,float x,float y) {
    struct Critter *c = make_base(gfx);
    init_flyer(&c->flyer,AIRBORNE);
    c->type = AIRCRITTER;
    c->physics.x = x;
    c->physics.y = y;

    c->timer = 0;
    c->timerfunc = ac_searchtarget;
    return c;
}

/* Create a bird */
static struct Critter *make_bird(float x,float y) {
    struct Critter *bird = make_base_ac(bird_gfx,x,y);
    bird->frames = bird_frames;
    bird->bidir = 0;
    bird->die = bird_die;
    return bird;
}

/* Create a bat */
static struct Critter *make_bat(float x,float y) {
    struct Critter *bat = make_base_ac(bat_gfx,x,y);
    bat->health = 0.05;
    bat->frames = bat_frames;
    bat->bidir = 0;
    bat->flyer.bat = 1;
    bat->ship = 0;
    bat->timerfunc = bat_seekground;
    bat->animate = bat_animate;
    bat->die = splatter;
    return bat;
}

/* Create a helicopter */
static struct Critter *make_helicopter(float x,float y,int owner) {
    struct Critter *hc = make_base_ac(helicopter_gfx,x,y);
    hc->frames = helicopter_frames;
    hc->health = 0.2;
    hc->bidir = 1;
    hc->gfx_rect.w/=4;
    hc->gfx_rect.x = hc->gfx_rect.w*owner;
    hc->animate = helicopter_animate;
    hc->owner = owner;
    hc->ship = 0;
    hc->die = helicopter_die;
    return hc;
}

/* Create a basic water critter */
static struct Critter *make_base_wc(SDL_Surface **gfx,float x,float y) {
    struct Critter *c = make_base(gfx);
    init_flyer(&c->flyer,UNDERWATER);
    c->flyer.speed = 0.5;
    c->type = WATERCRITTER;
    c->physics.x = x;
    c->physics.y = y;

    c->timer = 0;
    c->timerfunc = ac_searchtarget;
    return c;
}

/* Create a fish */
static struct Critter *make_fish(float x,float y) {
    struct Critter *fish = make_base_wc(fish_gfx,x,y);
    fish->frames = fish_frames;
    fish->bidir = 1;
    fish->die = splatter;
    return fish;
}

/* Find a random starting position. If ground=-+1, coordinates
 * will be at the interface of walkable terrain and the medium.
 * x and y will be set to the discovered coordinates.
 * Returns nonzero if no suitable coordinates were found.
 */
static int random_coords(Uint8 medium, int ground,float *xcoord,float *ycoord) {
    unsigned int loops=0;
    while(loops++<1000) {
        int x = rand()%lev_level.width;
        int y = rand()%lev_level.height;
        if(lev_level.solid[x][y]==medium) {
            if(ground) {
                int r=0;
                for(r=0;r<200;r++,y+=ground) {
                    if(y<0 || y>=lev_level.height ||
                            lev_level.solid[x][y]!=medium) break;
                }
                if(is_walkable(x,y)==0) continue;
            }
            *xcoord = x;
            *ycoord = y;
            return 0;
        }
    }
    return 1;
}

/* Make a critter */
struct Critter *make_critter (ObjectType species, float x, float y, int owner)
{
    struct Critter *newcritter = NULL;
    switch(species) {
        case OBJ_COW: newcritter = make_cow(x,y); break;
        case OBJ_SOLDIER: newcritter = make_soldier(x,y,owner); break;
        case OBJ_BIRD: newcritter = make_bird(x,y); break;
        case OBJ_BAT: newcritter = make_bat(x,y); break;
        case OBJ_HELICOPTER: newcritter = make_helicopter(x,y,owner); break;
        case OBJ_FISH: newcritter = make_fish(x,y); break;
        default:
           fprintf(stderr,"make_critter(%d,%.1f,%.1f,%d): Unhandled object type.\n",species, x, y, owner);
    }
    if(newcritter==NULL) return NULL;


    /* Find a place for the critter if not specified explicitly */
    if (x < 0 || y < 0) {
        int medium,ground;
        if(newcritter->type==WATERCRITTER) {
            medium = TER_WATER;
            ground = 0;
        } else {
            medium = TER_FREE;
            if(newcritter->type==GROUNDCRITTER)
                ground=1;
            else if(species==OBJ_BAT)
                ground=-1;
            else
                ground=0;
        }
        if(random_coords(medium,ground,&newcritter->physics.x,
                    &newcritter->physics.y)) {
            fprintf(stderr,"Couldn't find a place for a %s.\n", obj2str(species));
            free (newcritter);
            return NULL;
        }
    }
    return newcritter;
}

/* Kill a critter */
static struct dllist *kill_critter (struct dllist *list) {
    struct dllist *next=list->next;
    struct Critter *c = list->data;

    c->die(c);

    if(c->gfx == soldier_gfx && c->owner>=0)
        soldier_count[c->owner]--;
    else if(c->gfx == helicopter_gfx && c->owner>=0)
        helicopter_count[c->owner]--;

    free (list->data);

    if(list==critter_list)
        critter_list=dllist_remove(list);
    else
        dllist_remove(list);
    return next;
}

/* Add a critter to the list */
void add_critter (struct Critter * newcritter) {
    int kill=0;
    /* Enforce hostile critter limits */
    if(newcritter->gfx == soldier_gfx && game_settings.soldiers>0 &&
            newcritter->owner>=0)
    {
        if(++soldier_count[newcritter->owner] >= game_settings.soldiers)
            kill=1;
    } else if(newcritter->gfx == helicopter_gfx && game_settings.helicopters>0
            && newcritter->owner>=0)
    {
        if(++helicopter_count[newcritter->owner] >= game_settings.helicopters)
            kill=1;
    }
    if(kill) {
        struct dllist *ptr = critter_list;
        while(ptr) {
            struct Critter *c = ptr->data;
            if(c->gfx == newcritter->gfx && c->owner == newcritter->owner) {
                kill_critter(ptr);
                break;
            }
            ptr=ptr->next;
        }
    }

    /* Add critter to the list */
    if (critter_list)
        dllist_append(critter_list,newcritter);
    else
        critter_list=dllist_append(critter_list,newcritter);
}

/* Projectile hits a critter */
void hit_critter(struct Critter *critter, struct Projectile *p) {
    critter->health -= p->damage;
    /* special case, snowball freezes critters */
    if(p->color == col_snow && p->damage==0 && p->critical==0) {
        critter->timer=-1;
        critter->animate = NULL;
        critter->timerfunc = NULL;
        critter->die = shatter;
        critter->type = INERTCRITTER;
        critter->frozen = 1;
    }
}

/* Draw a critter on all active viewports */
static void draw_critter (struct Critter * critter)
{
    int p;
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            SDL_Rect rect, rect2;
            rect.x = critter->physics.x - critter->gfx_rect.w/2;
            rect.y = critter->physics.y;
            if(critter->type==GROUNDCRITTER)
                rect.y -= critter->gfx_rect.h;
            else
                rect.y -= critter->gfx_rect.h/2;

            rect.x = rect.x - cam_rects[p].x + viewport_rects[p].x;
            rect.y = rect.y - cam_rects[p].y + viewport_rects[p].y;
            if ((rect.x > viewport_rects[p].x - critter->gfx_rect.w
                 && rect.x < viewport_rects[p].x + cam_rects[p].w)
                && (rect.y > viewport_rects[p].y - critter->gfx_rect.h
                    && rect.y < viewport_rects[p].y + cam_rects[p].h)) {
                rect2 =
                    cliprect (rect.x, rect.y, critter->gfx_rect.w,
                            critter->gfx_rect.h, viewport_rects[p].x,
                            viewport_rects[p].y,
                            viewport_rects[p].x + cam_rects[p].w,
                            viewport_rects[p].y + cam_rects[p].h);
                rect2.x += critter->gfx_rect.x;
                rect2.y += critter->gfx_rect.y;
                if (rect.x < viewport_rects[p].x)
                    rect.x = viewport_rects[p].x;
                if (rect.y < viewport_rects[p].y)
                    rect.y = viewport_rects[p].y;
                SDL_BlitSurface (critter->gfx[critter->frame],
                        &rect2, screen, &rect);
                if(critter->frozen && iceblock) {
                    rect.x = rect.x + critter->gfx_rect.w/2 - iceblock->w/2;
                    rect.y = rect.y + critter->gfx_rect.h/2 - iceblock->h/2;
                    SDL_BlitSurface(iceblock, NULL, screen, &rect);
                }
#if 0
                /* Debugging aid: display critter target */
                if(critter->type!=GROUNDCRITTER) {
                    int x = critter->flyer.targx-cam_rects[p].x;
                    int y = critter->flyer.targy-cam_rects[p].y;
                    if(x>0 && x<cam_rects[p].w && y>0 && y<cam_rects[p].h)
                        putpixel(screen,x+viewport_rects[p].x,y+viewport_rects[p].y,col_red);
                        putpixel(screen,x+2+viewport_rects[p].x,y+viewport_rects[p].y,col_red);
                        putpixel(screen,x-2+viewport_rects[p].x,y+viewport_rects[p].y,col_red);
                        putpixel(screen,x+viewport_rects[p].x,y+2+viewport_rects[p].y,col_red);
                        putpixel(screen,x+viewport_rects[p].x,y-2+viewport_rects[p].y,col_red);
                }
#endif
            }
        }
    }
}

/* Some generic ground critter animation */
static void animate_groundcritter(struct Critter *critter) {
    animate_walker(&critter->walker,critter->ship?1:0,&ship_list);
    if(critter->walker.walking) {
        int x = Round(critter->walker.physics.x);
        int y = Round(critter->walker.physics.y);
        x += critter->walker.walkspeed * critter->walker.walking;
        if(find_foothold(x,y, critter->walker.slope)<0) {
            /* Turn around if can't find foothold */
            critter->walker.walking *= -1;
            critter->cornered += 1;
        }
        /* Update animation frame */
        critter->frame++;
        if(critter->walker.walking>0) {
            if(critter->frame>=critter->frames/2)
                critter->frame=0;
        } else {
            if(critter->frame>=critter->frames)
                critter->frame=critter->frames/2;
        }
    } else {
        /* Reset to neutral pose */
        if(critter->frame>=critter->frames/2)
            critter->frame=critter->frames/2;
        else
            critter->frame=0;
    }
}

/* Some generic air critter animation */
static void animate_aircritter(struct Critter *critter) {
    animate_flyer(&critter->flyer,critter->ship?1:0,&ship_list);
    if(critter->physics.hitground && is_walkable(Round(critter->physics.x),Round(critter->physics.y) + (critter->flyer.bat?1:-1))==0) {
        /* Flying critter is perched */
        critter->frame = critter->frames-1;
    } else {
        critter->frame++;
        if(critter->bidir) {
            if(critter->physics.vel.x>0) {
                if(critter->frame>=(critter->frames-1)/2)
                    critter->frame=0;
            } else {
                if(critter->frame>=critter->frames-1)
                    critter->frame=(critter->frames-1)/2;
            }
        } else {
            if(critter->frame>=critter->frames-1)
                critter->frame=0;
        }
    }
}

/* Some generic water critter animation */
static void animate_watercritter(struct Critter *critter) {
    animate_flyer(&critter->flyer,critter->ship?1:0,&ship_list);
    critter->frame++;
    if(critter->bidir) {
        if(critter->physics.vel.x>0) {
            if(critter->frame>=critter->frames/2)
                critter->frame=0;
        } else {
            if(critter->frame>=critter->frames)
                critter->frame=critter->frames/2;
        }
    } else {
        if(critter->frame>=critter->frames)
            critter->frame=0;
    }
}

/* Animate critters */
void animate_critters (void) {
    struct dllist *list = critter_list;
    while (list) {
        struct Critter *critter=list->data;
        switch(critter->type) {
            case INERTCRITTER: animate_object(&critter->physics,1,&ship_list); break;
            case GROUNDCRITTER: animate_groundcritter(critter); break;
            case AIRCRITTER: animate_aircritter(critter); break;
            case WATERCRITTER: animate_watercritter(critter); break;
        }

        /* Timer function */
        if(critter->timer>0) critter->timer--;
        else if(critter->timer==0) {
            critter->timer=-1;
            critter->timerfunc(critter);
        }

        /* Decrease feeling of claustrophobia */
        if(critter->cornered>0)
            critter->cornered -= 1.0/GAME_SPEED;

        /* Special animation if any */
        if(critter->animate)
            critter->animate(critter);

        /* Check if critter has been thrown against ground too hard */
        if(critter->physics.hitground && hypot(critter->physics.hitvel.x,
                    critter->physics.hitvel.y)> 5.0)
        {
            critter->health -= 0.1;
        }

        /* Kill critter if health is below 0 and/or move on to the next one */
        if(critter->health<=0) {
            list = kill_critter(list);
        } else {
            draw_critter (critter);
            list = list->next;
        }
    }
}

/* Draw the bat attack */
void draw_bat_attack ()
{
    int b;
    if (!bat_attack)
        return;
    for (b = 0; b < sizeof(crit_bat_attack)/sizeof(struct BatAttack); b++) {
        if (crit_bat_attack[b].end) {
            crit_bat_attack[b].end--;
            SDL_BlitSurface (bat_attack, &crit_bat_attack[b].src, screen,
                             &crit_bat_attack[b].targ);
        }
    }
}


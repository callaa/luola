/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : special.c
 * Description : Level special objects
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
#include "weapon.h"
#include "special.h"
#include "ship.h"
#include "audio.h"

/* List of special objects */
static struct dllist *special_list;

/* Special object graphics */
static SDL_Surface **jumpgate_gfx;
static int jumpgate_frames;
static SDL_Surface **turret_gfx[2]; /* Normal and missile turrets */
static int turret_frames[2];
static SDL_Surface **jumppoint_gfx[2]; /* Entry and exit points */
static int jumppoint_frames[2];

/* Check if there is any solid terrain inside a rectangle */
static int hitsolid_rect (int x, int y, int w, int h)
{
    int x2 = x+w, y2 = y+h;
    for (; x < x2; x++) {
        for (; y < y2; y++) {
            if (is_solid(x,y))
                return 1;
        }
        y-=h;
    }
    return 0;
}

/* Search for a good place for a turret */
/* If ground!=0, only y+ axis is searched */
static int find_turret_xy (int *tx, int *ty,int ground)
{
    int x[4]={*tx,*tx,*tx,*tx}, y[4]={*ty,*ty,*ty,*ty};
    int dx[4]={0,1,0,-1}, dy[4]={1,0,-1,0};
    int r,loops=0;
    while(++loops<400) {
        for(r=0;r<4;r++) {
            if((ground&&r!=0) || is_water(x[r],y[r])) continue;
            x[r] += dx[r];
            y[r] += dy[r];
            if(is_solid(x[r],y[r])) {
                *tx = x[r];
                *ty = y[r];
                return 1;
            }
        }
    }
    return 0;
}

/* Load level special object images */
void init_specials (LDAT *specialfile) {
    jumppoint_gfx[0] = load_image_array(specialfile, 0, T_ALPHA, "WARP",
            &jumppoint_frames[0]);
    jumppoint_gfx[1] = load_image_array(specialfile, 0, T_ALPHA, "WARPE",
            &jumppoint_frames[1]);

    turret_gfx[0] = load_image_array(specialfile, 0, T_ALPHA, "TURRET",
            &turret_frames[0]);
    turret_gfx[1] = load_image_array(specialfile, 0, T_ALPHA, "SAMSITE",
            &turret_frames[1]);

    jumpgate_gfx = load_image_array (specialfile, 0, T_ALPHA, "JUMPGATE",
            &jumpgate_frames);
}

/* Clear all level specials at the end of the level */
void clear_specials (void) {
    dllist_free(special_list,free);
    special_list=NULL;
}

/* Transport a ship between two jumppoints */
static void jumppoint_hitship(struct SpecialObj *point, struct Ship *ship) {
    if(point->timer==0 && point->link && point->frame>=point->frames/2) {
        point->timer=0.8*GAME_SPEED;
        point->link->timer=point->timer;
        ship->physics.x = point->link->x;
        ship->physics.y = point->link->y;
    }
}

/* Transport a projectile between two jumppoints */
static void jumppoint_hitprojectile(struct SpecialObj *point, struct Projectile *p) {
    if(p->physics.radius>=3.5 && point->timer==0 && point->link &&
            point->frame>=point->frames/2)
    {
        point->timer=0.1*GAME_SPEED;
        point->link->timer=point->timer;
        p->physics.x = point->link->x;
        p->physics.y = point->link->y;
    }
}

/* Jump-point animation */
static void jumppoint_animate(struct SpecialObj *point) {
    point->frame++;
    if(point->frame>=point->frames) /* Loop at the middle */
        point->frame=point->frames/2;
}

/* Create a jump-point */
struct SpecialObj *make_jumppoint(int x,int y,int owner,int exit) {
    struct SpecialObj *point= malloc(sizeof(struct SpecialObj));
    if(!point) {
        perror(__func__);
        return NULL;
    }
    point->gfx = jumppoint_gfx[exit?1:0];
    point->frames = jumppoint_frames[exit?1:0];
    point->frame=0;
    point->x = x;
    point->y = y;
    point->owner = owner;
    point->secret = 0;
    switch(game_settings.jumplife) {
        case JLIFE_SHORT: point->life = point->frames*1.5; break;
        case JLIFE_MEDIUM: point->life = point->frames*3.5; break;
        case JLIFE_LONG: point->life = point->frames*6.5; break;
    }
    point->timer = 0;
    point->link = NULL;
    point->hitship = jumppoint_hitship;
    point->hitprojectile = jumppoint_hitprojectile;
    point->animate = jumppoint_animate;
    point->destroy = NULL;

    return point;
}

/* Open a wormhole between two jumpgates */
static void jumpgate_hitship(struct SpecialObj *gate, struct Ship *ship) {
    if(gate->timer==0 && gate->link) {
        struct SpecialObj *exit = make_jumppoint(gate->link->x,gate->link->y,gate->owner,1);
        struct SpecialObj *entry = make_jumppoint(gate->x,gate->y,gate->owner,0);

        entry->link = exit;
        exit->link = entry;
        if(game_settings.onewayjp) {
            exit->hitship = NULL;
            exit->hitprojectile = NULL;
        }
        gate->timer=exit->life*1.5;
        gate->link->timer=gate->timer;
        add_special(exit);
        add_special(entry);
    }
}

/* Create a jumpgate */
static struct SpecialObj *make_jumpgate(int x,int y) {
    struct SpecialObj *gate = malloc(sizeof(struct SpecialObj));
    if(!gate) {
        perror(__func__);
        return NULL;
    }
    gate->gfx = jumpgate_gfx;
    gate->frames = jumpgate_frames;
    gate->frame=0;
    gate->x = x;
    gate->y = y;
    gate->owner = -1;
    gate->life = -1;
    gate->timer = 0;
    gate->secret = 0;
    gate->link = NULL;
    gate->hitship = jumpgate_hitship;
    gate->hitprojectile = NULL;
    gate->animate = NULL;
    gate->destroy = NULL;

    return gate;
}

/* Add random jumpgates to the level */
static void add_random_gates(int count) {
    int w=jumpgate_gfx[0]->w;
    int h=jumpgate_gfx[0]->h;
    int r;
    for(r=0;r<count;r++) {
        struct SpecialObj *gate[2];
        int g;
        for(g=0;g<2;g++) {
            int x,y,loops=0;
            do {
                x = rand()%lev_level.width;
                y = rand()%lev_level.height;
            } while((hitsolid_rect(x-w/2,y-h/2,w,h) ||
                        (g==1 && hypot(gate[0]->x-x,gate[0]->y-y)<500.0))
                    && ++loops<1000);
            if(loops>=1000) {
                fprintf(stderr,"Warning: Couldn't find place for a jumpgate!\n");
                if(g>0)
                    free(gate[0]);
                return;
            }
            gate[g] = make_jumpgate(x,y);
        }
        gate[0]->link = gate[1];
        gate[1]->link = gate[0];
        add_special(gate[0]);
        add_special(gate[1]);
    }
}

/* Turret fires a projectile */
static void turret_shoot(struct SpecialObj *turret) {
    struct Projectile *bullet;
    double x = turret->x + cos(turret->angle)*6;
    double y = turret->y - sin(turret->angle)*6;
    Vector v = get_muzzle_vel(turret->angle);

    switch(turret->type) {
        case 2:
            bullet = make_missile(x,y,v);
            bullet->angle = 2*M_PI-turret->angle;
            turret->timer = 1.5*GAME_SPEED;
            playwave_3d (WAV_MISSILE, turret->x, turret->y);
            break;
        case 1:
            bullet = make_grenade(x,y,v);
            turret->timer = 0.9 * GAME_SPEED;
            playwave_3d (WAV_NORMALWEAP, turret->x, turret->y);
            break;
        default:
            bullet = make_bullet(x,y,v);
            turret->timer = 0.5 * GAME_SPEED;
            playwave_3d (WAV_NORMALWEAP, turret->x, turret->y);
            break;
    }
    bullet->owner = turret->owner;
    add_projectile(bullet);
}

/* Turret explodes when it is destroyed */
static void turret_explode(struct SpecialObj *turret) {
    spawn_clusters(turret->x,turret->y, 5.6, 8, make_bullet);
    spawn_clusters(turret->x,turret->y, 5.6, 16, make_firestarter);
    add_explosion (turret->x,turret->y);
}

/* Turret animation */
static void turret_animate(struct SpecialObj *turret) {
    double dist;
    float targx=-1,targy=-1;
    int targplr;
    /* Get a target */
    targplr = find_nearest_enemy(turret->x,turret->y,turret->owner, &dist);
    if(dist<160.0) {
        targx = players[targplr].ship->physics.x;
        targy = players[targplr].ship->physics.y;
    } else {
        targplr = find_nearest_pilot(turret->x,turret->y,turret->owner, &dist);
        if(dist<160.0) {
            targx = players[targplr].pilot.walker.physics.x;
            targy = players[targplr].pilot.walker.physics.y;
        }
    }
    /* Aim at target if found */
    if(targx>=0) {
        double a = atan2(turret->y-targy, targx-turret->x);
        double d;
        if(a<0) a = 2*M_PI + a;
        d = shortestRotation(turret->angle,a);
        if(d<-0.2) {
            turret->turn = -0.2;
        } else if(d>0.2) {
            turret->turn = 0.2;
        } else {
            turret->turn = d/2;
            if(turret->timer==0)
                turret_shoot(turret);
        }
    } else {
        /* Restore normal turning speed */
        if(turret->turn<0)
            turret->turn=-0.05;
        else
            turret->turn=0.05;
    }
    /* Rotate turret */
    turret->angle += turret->turn;
    if(is_solid(turret->x+cos(turret->angle)*5,turret->y-sin(turret->angle)*5))
    {
        turret->angle -= turret->turn;
        turret->turn = -turret->turn;
    }
    if(turret->angle>2*M_PI) turret->angle=0;
    else if(turret->angle<0) turret->angle=2*M_PI;
    turret->frame = Round(turret->angle/(2*M_PI)*(turret->frames-1));
}

/* Missile turret animation */
static void mturret_animate(struct SpecialObj *turret) {
    /* Find a target */
    int targplr;
    double dist;
    if(turret->timer==0) {
        targplr = find_nearest_enemy(turret->x,turret->y,turret->owner, &dist);
        if(dist<250.0) {
            turret->angle = (turret->frames-turret->frame)/(double)turret->frames*M_PI_2+M_PI_4;
            turret_shoot(turret);
        }
    }

    /* Animation */
    if(turret->turn>0) {
        if(++turret->frame>=turret->frames) {
            turret->frame=turret->frames-1;
            turret->turn=-1;
        }
    } else {
        if(--turret->frame>turret->frames) {
            turret->frame=0;
            turret->turn=1;
        }
    }
}

/* Projectile hits a turret */
static void turret_hitprojectile(struct SpecialObj *turret, struct Projectile *p) {
    if(p->critter) { /* Collide only with the type of projectiles that
                        can hit critters and pilots */
        turret->health -= p->damage;
        if(turret->health<=0) turret->life=0;
        p->life=0;
        if(p->explode)
            p->explode(p);
    }
}

/* Create a turret */
static struct SpecialObj *make_turret(int x,int y,int type) {
    double a;
    struct SpecialObj *turret = malloc(sizeof(struct SpecialObj));
    if(!turret) {
        perror(__func__);
        return NULL;
    }
    turret->gfx = turret_gfx[type==2];
    turret->frames = turret_frames[type==2];
    turret->type = type;
    turret->frame=0;
    turret->x = x;
    turret->y = y;
    turret->owner = -1;
    turret->life = -1;
    turret->timer = 0;
    turret->secret = 0;
    turret->health = 0.20;
    turret->hitship = NULL;
    turret->hitprojectile = turret_hitprojectile;
    if(type==2)
        turret->animate = mturret_animate;
    else
        turret->animate = turret_animate;
    turret->destroy = turret_explode;
    turret->turn = 0.05;

    /* Find a good starting angle */
    if(type<2)
        for(a=0;a<2*M_PI;a++)
            if(!is_solid(x+cos(a)*5,y-sin(a)*5)) {turret->angle = a; break;}

    return turret;
}

/* Add random turrets to level */
static void add_random_turrets(int count) {
    int r;
    for(r=0;r<count;r++) {
        int x,y,loops=0;
        int type=rand()%3;
        do {
            x = rand()%lev_level.width;
            y = rand()%lev_level.height;
            if(is_free(x,y)) {
                if(find_turret_xy(&x,&y,type==2))
                    break;
            }
        } while(++loops<1000);
        if(loops>=1000) {
            fprintf(stderr,"Warning: Couldn't find place for a turret!\n");
            return;
        }
        add_special(make_turret(x,y,type));
    }
}

/* Place level specials at the start of the level */
void prepare_specials (struct LevelSettings * settings) {
    struct dllist *objects=NULL;

    /* Add random objects */
    add_random_gates(level_settings.jumpgates);
    add_random_turrets(level_settings.turrets);

    /* Add manually placed objects */
    if(settings)
        objects=settings->objects;
    while(objects) {
        struct LSB_Object *objdef = objects->data;
        struct SpecialObj *obj=NULL;
        switch(objdef->type) {
            case OBJ_TURRET:
                obj = make_turret(objdef->x,objdef->y,objdef->value);
                break;
            case OBJ_JUMPGATE:
                obj = make_jumpgate(objdef->x,objdef->y);
                obj->owner = objdef->id; /* Store id here temporarily */
                obj->link = (struct SpecialObj*)objdef->link;
                break;
            default: break;
        }
        add_special(obj);
        objects = objects->next;
    }
    /* Pair up the manually placed jumpgates */
    objects = special_list;
    while(objects) {
        struct SpecialObj *obj = objects->data;
        if(obj->gfx == jumpgate_gfx && (int)obj->link<=255) {
            struct dllist *pair = objects->next;
            while(pair) {
                struct SpecialObj *p = pair->data;
                if(p->gfx == jumpgate_gfx && (int)p->link == obj->owner &&
                        p->owner == (int)obj->link)
                {
                    obj->owner = -1;
                    obj->link = p;
                    p->owner = -1;
                    p->link = obj;
                    break;
                }
                pair = pair->next;
            }
            if(pair==NULL) {
                fprintf(stderr,"No pair (%d) found for jumpgate %d\n",
                        (int)obj->link,obj->owner);
                obj->life=0; /* mark the jumpgate for deletion */
            }
        }
        objects = objects->next;
    }
}

/* Add a new level special */
void add_special (struct SpecialObj *special) {
    if(special) {
        if(special_list)
            dllist_append(special_list,special);
        else
            special_list=dllist_append(special_list,special);
    }
}

/* Drop a jump point. The jump-point will wait for a pair to be dropped */
/* before it will open. */
void drop_jumppoint (int x, int y, int player) {
    /* First search for a exit point to this jumppoint */
    struct dllist *ptr=special_list;
    struct SpecialObj *exit,*point;
    while(ptr) {
        exit = ptr->data;
        if(exit->gfx==jumppoint_gfx[1] && exit->owner == player &&
                exit->link==NULL)
            break;
        ptr=ptr->next;
    }
    if(ptr) { /* If exit point exists, create the wormhole */
        point = make_jumppoint(x,y,player,0);
        point->link = exit;
        exit->link = point;

        exit->life = point->life;
        exit->animate = point->animate;
        exit->secret = 0;
        if(game_settings.onewayjp==0) {
            exit->hitship = point->hitship;
            exit->hitprojectile = point->hitprojectile;
        }
        playwave (WAV_JUMP);
    } else { /* No exit point, create one */
        point = make_jumppoint(x,y,player,1);
        point->animate = NULL;
        point->hitship = NULL;
        point->hitprojectile = NULL;
        point->life=-1;
        point->secret = 1;
    }
    add_special(point);
}

/* Draw a special object on all viewports */
static void draw_special (struct SpecialObj *object)
{
    int p;
    for(p=0;p<4;p++) {
        if (players[p].state==ALIVE||players[p].state==DEAD) {
            SDL_Rect rect = {0,0, object->gfx[0]->w, object->gfx[0]->h};
            SDL_Rect rect2;
            if(object->secret && p!=object->owner) continue;
            rect.x = object->x - rect.w/2 - cam_rects[p].x + viewport_rects[p].x;
            rect.y = object->y - rect.h/2 - cam_rects[p].y + viewport_rects[p].y;
            if ((rect.x > viewport_rects[p].x - rect.w
                 && rect.x < viewport_rects[p].x + cam_rects[p].w)
                && (rect.y > viewport_rects[p].y - rect.h
                    && rect.y < viewport_rects[p].y + cam_rects[p].h)) {
                rect2 =
                    cliprect (rect.x, rect.y, rect.w, rect.h, viewport_rects[p].x,
                              viewport_rects[p].y, viewport_rects[p].x + cam_rects[p].w,
                              viewport_rects[p].y + cam_rects[p].h);
                if (rect.x < viewport_rects[p].x)
                    rect.x = viewport_rects[p].x;
                if (rect.y < viewport_rects[p].y)
                    rect.y = viewport_rects[p].y;
                SDL_BlitSurface (object->gfx[object->frame], &rect2, screen,
                                 &rect);
            }
        }
    }
}

/* Check if a ship hits a special object */
static void ship_hit_special(struct SpecialObj *obj) {
    struct dllist *ptr = ship_list;
    int w2 = obj->gfx[0]->w/2;
    int h2 = obj->gfx[0]->h/2;
    while(ptr) {
        struct Ship *ship=ptr->data;

        if(ship->physics.x>=obj->x-w2 && ship->physics.x<=obj->x+w2 &&
                ship->physics.y>=obj->y-h2 && ship->physics.y<=obj->y+h2)
        {
            obj->hitship(obj,ship);
            break;
        }
        ptr=ptr->next;
    }
}

/* Check if a projectile hits a special object */
static void projectile_hit_special (struct SpecialObj *obj) {
    struct dllist *ptr = projectile_list;
    int w2 = obj->gfx[0]->w/2;
    int h2 = obj->gfx[0]->h/2;
    while(ptr) {
        struct Projectile *p = ptr->data;

        if(p->physics.x>=obj->x-w2 && p->physics.x<=obj->x+w2 &&
                p->physics.y>=obj->y-h2 && p->physics.y<=obj->y+h2)
        {
            obj->hitprojectile(obj,p);
        }
        ptr=ptr->next;
    }
}

/* Animate special objects */
void animate_specials (void) {
    struct dllist *list = special_list,*next;
    while (list) {
        struct SpecialObj *obj=list->data;

        /* Check for collisions */
        if(obj->hitship)
            ship_hit_special (obj);
        if(obj->hitprojectile)
            projectile_hit_special(obj);

        /* Animate */
        if(obj->animate)
            obj->animate(obj);
        if (obj->timer > 0)
            obj->timer--;

        /* Draw the object on all viewports */
        draw_special (obj);

        /* Check if the object has expired.
         * If life < 0, the object has no time limit */
        next = list->next;
        if(obj->life > 0) obj->life--;
        else if (obj->life == 0) {
            if(obj->destroy)
                obj->destroy(obj);
            free (obj);
            if(list==special_list)
                special_list=dllist_remove(list);
            else
                dllist_remove(list);
        }
        list = next;
    }
}


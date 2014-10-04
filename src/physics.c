/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : physics.c
 * Description : Physics engine
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

#include "physics.h"
#include "level.h"
#include "decor.h"  /* For snow and splash effects */

/* List of gravity anomalies */
static struct dllist *gravities;

/* World physics settings */
#define WATER_FLOW 2.0
#define SPLASH_TRESHOLD 4.0 /* Minimum radius before objects splash */
#define SPLASH_SPEED 2.0    /* Minimum velocity before objects splash */

/* Clear away old gravity anomalies */
void reset_physics(void) {
    dllist_free(gravities,free);
    gravities = NULL;
}

/* Initialize a physical object to some sensible state */
void init_physobj(struct Physics *obj,float x,float y,Vector v) {
    obj->x = x;
    obj->y = y;
    obj->vel = v;
    obj->thrust.x = 0;
    obj->thrust.y = 0;

    obj->radius = 1;
    obj->mass = 1;

    obj->semisolid = 0;
    obj->solidity=SOLID;
    obj->sharpness=SHARP;
    obj->obj=1;

    obj->hitground=0;
    obj->hitobj=NULL;
    obj->underwater=0;
}

/* Create a new gravity anomaly and add it to list */
struct GravityAnomaly *new_ga(float x,float y,float radius, float mass) {
    struct GravityAnomaly *ga = malloc(sizeof(struct GravityAnomaly));
    if(!ga) {
        perror("new_ga");
        return NULL;
    }

    ga->type = GA_LOCAL;
    ga->local.x = x; ga->local.y = y;
    ga->radius = radius;
    ga->mass = mass;

    ga->offset = mass/145.0;
    gravities = dllist_prepend(gravities, ga);
    return ga;
}

/* Create a new linked gravity anomaly and add it to list */
struct GravityAnomaly *new_ga_link(float *x,float *y,float radius, float mass) {
    struct GravityAnomaly *ga = new_ga(0,0,radius,mass);
    ga->type = GA_LINK;
    ga->link.x = x;
    ga->link.y = y;
    return ga;
}

/* Remove pointed gravity anomaly */
void remove_ga(struct GravityAnomaly *ga) {
    struct dllist *lst = dllist_find(gravities, ga);
    if(lst) {
        if(lst==gravities)
            gravities = dllist_remove(lst);
        else
            dllist_remove(lst);
    } else {
        fprintf(stderr,"%s(%p): gravity anomaly not found!\n",__func__,ga);
    }
}

/* Impact between two objects */
static void object_impact(struct Physics *obj1, struct Physics *obj2) {
    /* TODO */
    Vector tmpv1,tmpv2;
    tmpv1 = multVector(obj1->vel,1*obj1->mass/obj2->mass);
    tmpv2 = multVector(obj2->vel,1*obj2->mass/obj1->mass);
    obj2->vel = addVectors(obj2->vel,tmpv1);
    obj1->vel = addVectors(obj1->vel,tmpv2);
}

/* Animate a physical object for 1 frame */
void animate_object(struct Physics *object,int lists,struct dllist *objects[]) {
    struct dllist *ga = gravities;
    Vector flow = {0,0};
    double newx,newy;
    int ix,iy;
    double k;
    Vector Fv;
    int hitx,hity;

    /* Next position for object */
    newx = object->x + object->vel.x;
    newy = object->y + object->vel.y;
    ix = Round(newx);
    iy = Round(newy);

    /* Check level boundary collision (happens to all objects) */
    if(ix<0 || ix>=lev_level.width || iy<0 ||iy>=lev_level.height) {
        newx = object->x;
        newy = object->y;
        ix = Round(newx);
        iy = Round(newx);
        object->hitground = TER_INDESTRUCT;
        object->hitvel = object->vel;
        object->vel.x = 0.0;
        object->vel.y = 0.0;
    } else if(object->solidity!=IMMATERIAL) {
        /* Check terrain collision if object is solid */
        int terrain,solid;
        object->underwater = 0;
        object->hitground  = 0;
        if(object->solidity==SOLID) {
            terrain = hit_solid_line(Round(object->x), Round(object->y),
                    ix,iy,&hitx,&hity);
        } else {
            terrain = lev_level.solid[ix][iy];
        }
        if(ter_free(terrain)) solid=0;
        else if(object->semisolid && ter_semisolid(terrain)) solid=0;
        else if(terrain==TER_WATER) solid = -1;
        else if(terrain>=TER_WATERFU && terrain<=TER_WATERFL)
            solid = -2-(terrain-TER_WATERFU);
        else solid = terrain;
        /* Snow and ice are soft, so things can burrow into them */
        if((solid==TER_SNOW || solid==TER_ICE) &&
            object->solidity==SOLID && object->mass *
                hypot(object->hitvel.x,object->hitvel.y) > 4.9) {
            if(solid==TER_SNOW) {
                Vector sv = multVector(oppositeVector(object->hitvel),0.2);
                putpixel(lev_level.terrain,hitx,hity,col_black);
                lev_level.solid[hitx][hity] = TER_FREE;
                add_decor(make_snowflake(hitx + sv.x,hity + sv.y, sv));
            } else {
                putpixel(lev_level.terrain,hitx,hity,lev_watercol);
                lev_level.solid[hitx][hity] = TER_WATER;
            }
        }
        /* Common terrain collisions */
        if(solid>0) {
            object->hitground = terrain;
            object->hitvel = object->vel;
            if(object->solidity==SOLID) {
                /* Sharp objects sink into the terrain */
                if(object->sharpness==SHARP || (object->sharpness==BLUNT
                    && (terrain!=TER_INDESTRUCT&&terrain!=TER_BASE&&
                        terrain!=TER_BASEMAT)))
                {
                    newx = hitx;
                    newy = hity;
                    ix = hitx;
                    iy = hity;
                } else {
                    /* Blunt objects bounce back */
                    newx = object->x;
                    newy = object->y;
                    ix = Round(newx);
                    iy = Round(newx);
                }
                object->vel.x = 0;
                object->vel.y = 0;
            }
        } else {
            if(solid<0) {
                /* Underwater */
                object->underwater = 1;
                if(object->radius>SPLASH_TRESHOLD && is_water(Round(object->x),Round(object->y))==0 && hypot(object->vel.x,object->vel.y) > SPLASH_SPEED)
                {
                    add_splash(object->x,object->y,5.0,32,
                            multVector(object->vel,0.5), make_waterdrop);
                }
                switch(solid) {
                        case -2: flow.y = -WATER_FLOW; break;
                        case -3: flow.x = WATER_FLOW; break;
                        case -4: flow.y = WATER_FLOW; break;
                        case -5: flow.x = -WATER_FLOW; break;
                    }
            } else if(object->radius>SPLASH_TRESHOLD && is_water(Round(object->x),Round(object->y)) && hypot(object->vel.x,object->vel.y) > SPLASH_SPEED)
            {
                add_splash(object->x,object->y,5.0, 32,
                        multVector(object->vel,0.5), make_waterdrop);
            }
        }
    }
    /* Check object collisions */
    /* Note. collides with only one object at a time */
    /* Object collision checks are only done if object hasn't already
     * collided with ground. This is to stop bullets from damaging a ship
     * or killing a pilot while burrowing */
    if(object->obj && object->hitground==0) {
        int l=0;
        object->hitobj = NULL;
        while(l<lists && object->hitobj==NULL) {
            struct dllist *list = objects[l];
            while(list) {
                struct Physics *obj2 = list->data;
                if(obj2 == object) {
                    list=list->next;
                    continue;
                }
                /* First, check bounding box collisions */
                if( fabs(newx - obj2->x) < (object->radius + obj2->radius) &&
                    fabs(newy - obj2->y) < (object->radius + obj2->radius)) {
                    /* Then do a more accurate check */
                    double d = hypot(newx-obj2->x, newy-obj2->y);
                    if(d < (object->radius + obj2->radius)) {
                        object->hitobj = obj2;
                        object_impact(object,obj2);
#if 0
                        /* Exchange energies. Doesn't work properly yet */
                        Vector tmpv1,tmpv2;
                        tmpv1 = multVector(object->vel,1*object->mass/obj2->mass);
                        tmpv2 = multVector(obj2->vel,1*obj2->mass/object->mass);
                        obj2->vel = addVectors(obj2->vel,tmpv1);
                        object->vel = addVectors(object->vel,tmpv2);
#endif
                        break;
                    }
                }
                list=list->next;
            }
            l++;
        }
    }

    /* Relocate object to new coordinates */
    object->x = newx;
    object->y = newy;

    /* Gravity anomalies */
    while(ga) {
        struct GravityAnomaly *g = ga->data;
        double dist;
        double gx,gy;
        if(g->type==GA_LOCAL) {
            gx = g->local.x;
            gy = g->local.y;
        } else {
            gx = *g->link.x;
            gy = *g->link.y;
        }
        dist = hypot(gx - object->x, gy-object->y);
        if(dist>g->radius) {
            double force = g->mass/((dist-g->radius + g->offset)*(dist-g->radius+g->offset));
            object->vel.x -= (object->x - gx)/dist*force;
            object->vel.y -= (object->y - gy)/dist*force;
        }
        ga = ga->next;
    }

    /* Physics */
    object->vel.y += GRAVITY;

    object->vel.y -= ((object->underwater?WATER_rho:AIR_rho)*object->radius
            * GRAVITY) / object->mass;

    object->vel.x += object->thrust.x + flow.x;
    object->vel.y += object->thrust.y + flow.y;

    k = -3 * object->radius * (object->underwater?WATER_k:AIR_k);
    Fv = multVector(object->vel,k/GAME_SPEED);

    object->vel = addVectors(object->vel, Fv);
}

/* Get the mass required for an object of given radius to float in air */
/* Forces affecting an object are gravity and lift */
/* Lift must be great enough to counter gravity, but not too great, so
 * the object will stay still. */
double get_floating_mass(double radius) {
    return AIR_rho * radius;
}

/* Get the acceleration that will quarantee constant velocity in air */
Vector get_constant_vel(Vector v, double radius,double mass) {
    Vector a = {0, -GRAVITY};
    double k = (3 * radius * AIR_k)/GAME_SPEED;
    a.y += (AIR_rho*radius*GRAVITY)/mass;

    a.x += v.x * k;
    a.y += v.y * k;

    return a;
}

/* Rotate a vector */
void rotateVector(Vector *vec, double th) {
    double c = cos(th);
    double s = sin(th);
    double tx = c * vec->x - s * vec->y;
    vec->y = s * vec->x + c * vec->y;
    vec->x = tx;
}

/* Get shortest rotation angle for src to match target */
double shortestRotation(double src, double target) {
    double d = target - src;

    if (d > M_PI)
        d = -2*M_PI + d;
    else if(d< -M_PI)
        d = 2*M_PI + d;

    return d;
}


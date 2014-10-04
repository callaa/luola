/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : physics.h
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

#ifndef PHYSICS_H
#define PHYSICS_H

#include "list.h"

#define GAME_SPEED       (1000/30)   /* Speed of the game. (1000/FPS) */
#define GRAVITY (8.0/GAME_SPEED)
#define WATER_k 0.25
#define AIR_k   0.06
#define WATER_rho 1.8
#define AIR_rho   0.5

/* A mathematical vector */
typedef struct {
    double x;
    double y;
} Vector;

/* Physical object */
struct Physics {
    float x,y;
    float mass,radius;
    Vector vel;
    Vector thrust;
    /* Options */
    int semisolid;  /* Can go thru semisolid terrain */
    enum {IMMATERIAL,WRAITH,SOLID} solidity; /* Does terrain affect the object? Wraiths go thru terrain like air, but report collisions */
    enum {BOUNCY,BLUNT,SHARP} sharpness; /* Does the object sink into ground when it hits it. Blunt objects sink only in soft terrain. */
    int obj;       /* Do other objects affect the object */

    /* Flags set by animate_object */
    int hitground;  /* Object touched ground */
    Vector hitvel;  /* Velocity when ground was hit */
    int underwater; /* Object is under water */
    struct Physics *hitobj;    /* The other object this one touched */
};

/* A gravity anomaly */
struct GravityAnomaly {
    enum {GA_LOCAL,GA_LINK} type;
    union {
        struct {
            float x,y;
        } local;
        struct {
            float *x,*y;
        } link;
    };
    float radius;
    float mass;

    float offset;
};

/* Clear away old gravity anomalies */
extern void reset_physics(void);

/* Create a new physical object with default values. */
extern void init_physobj(struct Physics *obj,float x,float y,Vector v);

/* Create a new gravity anomaly and add it to list */
extern struct GravityAnomaly *new_ga(float x,float y, float radius,float mass);

/* Create a new linked gravity anomaly and add it to list */
extern struct GravityAnomaly *new_ga_link(float *x,float *y, float radius,float mass);

/* Remove pointed gravity anomaly */
extern void remove_ga(struct GravityAnomaly *ga);

/* Animate an object for a single frame (GAME_SPEED) */
/* objects is a list of other physical objects that are checked for collisions */
extern void animate_object(struct Physics *object,int lists,struct dllist *objects[]);

/* Get the mass required for an object of given radius to float in air */
extern double get_floating_mass(double radius);

/* Get the acceleration that will quarantee constant velocity in air */
extern Vector get_constant_vel(Vector v, double radius,double mass);

/* Make a new vector */
static inline Vector makeVector (double x, double y) {
    Vector newVec = {x,y};
    return newVec;
}

/* Return an opposite vector */
static inline Vector oppositeVector (const Vector vec) {
    Vector opp = {-vec.x, -vec.y};
    return opp;
}

/* Add two vectors together */
static inline Vector addVectors (Vector v1,Vector v2)
{
    Vector result = {v1.x+v2.x, v1.y+v2.y};
    return result;
}

static inline Vector multVector(Vector v,double m) {
    Vector result = {v.x * m, v.y * m};
    return result;
}

/* Rotate a vector */
extern void rotateVector(Vector *vec, double th);

/* Get shortest rotation angle for src to match target */
extern double shortestRotation(double src, double target);

#endif


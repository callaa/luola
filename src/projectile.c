/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2006 Calle Laakkonen
 *
 * File        : projectile.c
 * Description : Projectile animation and handling
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

#include "defines.h" /* Round */
#include "projectile.h"
#include "console.h"
#include "player.h"
#include "level.h"
#include "game.h"
#include "ship.h"
#include "pilot.h"
#include "critter.h"
#include "fs.h"

/* How soon an explosion sends out shrapnel */
#define EXPLOSION_CLUSTER_SPEED 5

struct Explosion {
    int x,y,frame,terrain;
};

struct dllist *projectile_list,*last_projectile;
static struct dllist *explosions;
static SDL_Surface **explosion_gfx;
static int explosion_frames;

/* Load projectile related datafiles */
extern void init_projectiles(LDAT *explosionfile) {
    explosion_gfx = load_image_array(explosionfile,0,T_ALPHA,"EXPL",&explosion_frames);
}

/* Clear all projectiles */
void clear_projectiles(void) {
    dllist_free(projectile_list,free);
    projectile_list=NULL;
    last_projectile = NULL;
    dllist_free(explosions,free);
    explosions=NULL;
}

/* Add a new projectile */
void add_projectile(struct Projectile *p) {
    last_projectile=dllist_append(last_projectile,p);
    if(!projectile_list) projectile_list=last_projectile;
}

/* Add an explosion animation and make a hole */
void add_explosion(int x,int y) {
    if(x<0 || y<0 || x>=lev_level.width || y>=lev_level.height)
        return;
    playwave_3d (WAV_EXPLOSION, x, y);
    if(game_settings.explosions || is_explosive(x,y)) {
        struct Explosion *e;
        e = malloc(sizeof(struct Explosion));
        e->x = x-explosion_gfx[0]->w/2;
        e->y = y-explosion_gfx[0]->h/2;;
        e->frame = 0;
        e->terrain = lev_level.solid[x][y];

        explosions = dllist_prepend(explosions,e);
    }

    make_hole(x,y);
}

/* Remove a projectile. Returns the next one in the list */
static struct dllist *remove_projectile(struct dllist *lst) {
    struct dllist *next=lst->next;

    if(((struct Projectile*)lst->data)->destroy)
        ((struct Projectile*)lst->data)->destroy(lst->data);

    free(lst->data);
    if(lst==last_projectile) last_projectile=last_projectile->prev;
    if(lst==projectile_list)
        projectile_list=dllist_remove(lst);
    else
        dllist_remove(lst);
    return next;
}

/* Projectile drawing framework */
static void projectile_draw(struct Projectile *p) {
    if(p->draw) {
        int ix = Round(p->physics.x);
        int iy = Round(p->physics.y);
        int plr;

        for(plr=0;plr<4;plr++) {
            if(players[plr].state==ALIVE || players[plr].state==DEAD) {
                SDL_Rect rect = {viewport_rects[plr].x,viewport_rects[plr].y,
                    cam_rects[plr].w,cam_rects[plr].h};
                int sx = ix - cam_rects[plr].x;
                int sy = iy - cam_rects[plr].y;
                if(sx>=0 && sx<rect.w && sy>=0 && sy < rect.h) {
                    if(p->cloak) {
                        float plrx = players[plr].ship?players[plr].ship->physics.x:players[plr].pilot.walker.physics.x;
                        float plry = players[plr].ship?players[plr].ship->physics.y:players[plr].pilot.walker.physics.y;
                        if(fabs(plrx-p->physics.x) > 60 ||
                                fabs(plry-p->physics.y) > 60)
                            continue;
                    }
                    p->draw(p,ix - cam_rects[plr].x,
                            iy - cam_rects[plr].y,rect);
                }
            }
        }
    }
}

/* Set time until projectile becomes active */
void set_fuse(struct Projectile *p, int ticks) {
    p->physics.obj = 0;
    p->prime = ticks;
}

/* Draw and animate explosions */
static void animate_explosions(void) {
    struct dllist *ptr = explosions;
    while(ptr) {
        struct dllist *next = ptr->next;
        struct Explosion *e = ptr->data;
        int p;

        e->frame++;
        if (e->frame == EXPLOSION_CLUSTER_SPEED) {
            if(e->terrain == TER_EXPLOSIVE)
                spawn_clusters(e->x, e->y, 5.6, 6, make_bullet);
            else if(e->terrain == TER_EXPLOSIVE2)
                spawn_clusters(e->x, e->y, 5.6, 3, make_grenade);
        }
        if (e->frame == explosion_frames) {   /* Animation is over */
            free (e);
            if(ptr==explosions)
                explosions=dllist_remove(ptr);
            else
                dllist_remove(ptr);
        } else {
            SDL_Surface *surf = explosion_gfx[e->frame];
            for(p=0;p<4;p++) {
                if(players[p].state == ALIVE || players[p].state == DEAD) {
                    SDL_Rect rect;
                    rect.x = viewport_rects[p].x + e->x - cam_rects[p].x;
                    rect.y = viewport_rects[p].y + e->y - cam_rects[p].y;
                    if((rect.x > viewport_rects[p].x &&
                            rect.x < viewport_rects[p].x + cam_rects[p].w - surf->w)
                            && (rect.y > viewport_rects[p].y
                            && rect.y < viewport_rects[p].y + cam_rects[p].h - surf->h))
                        SDL_BlitSurface (surf, NULL, screen, &rect);

                }
            }
        }
        ptr = next;
    }
}

/* Animate all listed projectiles */
void animate_projectiles(void) {
    struct dllist *ptr = projectile_list;
    struct dllist *clist[4];

    clist[0] = ship_list;
    while(ptr) {
        int ccount=1;
        struct Projectile *p = ptr->data;

        /* Counters */
        if(p->life > 0) p->life--;
        if(p->timer> 0) p->timer--;
        else if(p->timer==0) {
            p->timer=-1;
            p->timerfunc(p);
        }
        if(p->prime> 0) p->prime--;
        else if(p->prime==0) {
            p->prime=-1;
            p->physics.obj = 1;
        }

        /* Run special movement code if any */
        if(p->move)
            p->move(p);

        /* Do physics simulation. */
        if(p->otherobj) {
            clist[ccount++] = projectile_list;
        }
        if(p->critter) {
            clist[ccount++] = pilot_list;
            clist[ccount++] = critter_list;
        }

        animate_object(&p->physics,ccount,clist);

        /* Terrain collisions */
        if(p->physics.hitground || (p->hydrophobic && p->physics.underwater)) {
            if(p->explode)
                p->explode(p);
            if(p->physics.solidity!=WRAITH) /* Wraith type objects must decide when to die themselves */
                p->life=0;
        }
        /* Object collisions */
        if(p->physics.hitobj) {
            struct dllist *ptr;
            if((ptr = dllist_find(ship_list,p->physics.hitobj))) {
                struct Ship *hs = ptr->data;
                /* Collided with a ship */
                if(p->hitship && p->hitship(p,hs))
                    p->life = 0;
            } else if((ptr = dllist_find(pilot_list,p->physics.hitobj))) {
                /* Not a ship, a pilot then */
                if(p->explode)
                    p->explode(p);
                p->life=0;
                kill_pilot(ptr->data);
            } else if((ptr = dllist_find(critter_list,p->physics.hitobj))) {
                /* Not a pilot, a critter then */
                hit_critter(ptr->data,p);
                if(p->explode)
                    p->explode(p);
                p->life=0;
            } else if((ptr = dllist_find(projectile_list,p->physics.hitobj))) {
                struct Projectile *hp = ptr->data;
                if(hp->life) {
                    /* Not a critter, an other projectile then */
                    if(p->explode)
                        p->explode(p);
                    p->life=0;
                    if(hp->explode)
                        hp->explode(p);
                    hp->life=0;
                }
            }
        }

        /* Draw the projectile */
        projectile_draw(p);

        /* Delete expired projectile and move on */
        if(p->life==0)
            ptr = remove_projectile(ptr);
        else
            ptr=ptr->next;
    }

    animate_explosions();
}


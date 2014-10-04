/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : pilot.c
 * Description : Pilot code
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
#include "level.h"
#include "particle.h"
#include "player.h"
#include "animation.h"
#include "critter.h"
#include "ship.h"
#include "ldat.h"
#include "decor.h"
#include "fs.h"

#define LETHAL_VELOCITY 4.0  /* How fast is too fast */
#define PILOT_TOOFAST   10   /* For how long can a pilot fall too fast without dieing when hitting ground */
#define MAX_ROPE_LEN	120  /* Maximium length of the pilots rope in pixels */
#define ROPE_SPEED      5    /* How fast does the rope unwind */
#define PILOT_STD_RADIUS 4.1 /* Normal radius for pilot */
#define PILOT_PAR_RADIUS 8.0 /* Parachuting radius for pilot */

/* List of active pilots */
struct dllist *pilot_list;

/* Internally used globals */
static SDL_Surface *pilot_sprite[4][3];        /* Normal,Normal2, Parachute */

/* Load pilot related datafiles */
void init_pilots (LDAT *playerfile) {
    int r, p;
    SDL_Surface *tmp;
    SDL_Rect rect, targrect;
    /* Load pilot sprites */
    for (r = 0; r < 3; r++) {   /* Pilots have 3 frames */
        rect.y = 0;
        targrect.x = 0;
        targrect.y = 0;
        tmp = load_image_ldat (playerfile, 0, T_COLORKEY, "PILOT", r);
        for (p = 0; p < 4; p++) { /* There can be 4 pilots in the game */
            rect.w = tmp->w / 4;
            rect.h = tmp->h;
            pilot_sprite[p][r] = make_surface(tmp,rect.w,rect.h);
            rect.x = rect.w * p;
            pixelcopy ((Uint32 *) tmp->pixels + rect.x,
                       (Uint32 *) pilot_sprite[p][r]->pixels, rect.w, rect.h,
                       tmp->pitch / tmp->format->BytesPerPixel,
                       pilot_sprite[p][r]->pitch /
                       tmp->format->BytesPerPixel);
            SDL_SetColorKey (pilot_sprite[p][r],
                             SDL_SRCCOLORKEY | SDL_RLEACCEL,
                             tmp->format->colorkey);
        }
        SDL_FreeSurface (tmp);
    }
}

/* Initialize all pilots */
void reinit_pilots(void) {
    int plr,r;
    dllist_free(pilot_list,NULL);
    pilot_list = 0;

    for(plr=0;plr<4;plr++) {
        memset (&players[plr].pilot, 0, sizeof (Pilot));

        init_walker(&players[plr].pilot.walker);
        players[plr].pilot.walker.physics.radius = PILOT_STD_RADIUS;

        for (r = 0; r < 3; r++)
            players[plr].pilot.sprite[r] = pilot_sprite[plr][r];
        players[plr].pilot.attack_vector = makeVector(1,0);
    }
}

/* Deploy the parachute */
static void deploy_parachute(struct Pilot *pilot) {
    pilot->parachuting = 1;
    pilot->walker.jumpmode = GLIDE;
    pilot->walker.physics.radius = PILOT_PAR_RADIUS;
}

/* Remove the parachute */
static void undeploy_parachute(struct Pilot *pilot) {
    pilot->parachuting = 0;
    pilot->walker.jumpmode = JUMP;
    pilot->walker.physics.radius = PILOT_STD_RADIUS;
}

/* Eject a pilot */
void eject_pilot(int plr) {
    if (players[plr].ship) {
        players[plr].ship->fire_weapon = 0;
        ship_stop_special(players[plr].ship);
        players[plr].ship->thrust = 0;
        players[plr].ship->physics.thrust = makeVector(0,0);
        if (players[plr].ship->state == INTACT)
            players[plr].ship->turn = 0;
        players[plr].pilot.walker.physics.x = players[plr].ship->physics.x;
        players[plr].pilot.walker.physics.y = players[plr].ship->physics.y;
        players[plr].ship->eject_cooloff = 70;
        players[plr].ship = NULL;
    }
    memset (&players[plr].controller, 0, sizeof (GameController));
    players[plr].weapon_select = 0;
    players[plr].pilot.walker.physics.vel = makeVector (0, -3);
#if 0
    players[plr].pilot.rope = 0;
#endif
    players[plr].pilot.walker.walking = 0;
    players[plr].pilot.lock = 0;
    undeploy_parachute(&players[plr].pilot);

    pilot_list = dllist_prepend(pilot_list,&players[plr].pilot);
}

/* Remove a pilot from the level */
void remove_pilot(struct Pilot *pilot) {
    struct dllist *lst = dllist_find(pilot_list,pilot);

    pilot_detach_rope(pilot);

    if(lst==pilot_list)
        pilot_list = dllist_remove(lst);
    else
        dllist_remove(lst);
}

/* Kill a pilot */
void kill_pilot(struct Pilot *pilot) {
    int p;
    for(p=0;p<4;p++)
        if(&players[p].pilot == pilot) break;

    kill_player(p);

    add_splash(pilot->walker.physics.x, pilot->walker.physics.y,5.0, 16,
            pilot->walker.physics.vel,make_blood);

    remove_pilot(pilot);
}

/* Make sweatdrops appear around pilot's head */
static void make_sweatdrops(const struct Pilot *pilot) {
    struct Particle *sweatdrop;
    int r;
    for (r = 0; r < 6; r++) {
        sweatdrop =
            make_particle (pilot->walker.physics.x + (8 - rand () % 16),
                           pilot->walker.physics.y-pilot->sprite[0]->h *(2.0/3) - rand () % 12, 2);
        sweatdrop->color[0] = 220;
        sweatdrop->color[1] = 220;
        sweatdrop->color[2] = 255;
#ifdef HAVE_LIBSDL_GFX
        sweatdrop->rd = 0;
        sweatdrop->bd = 0;
        sweatdrop->gd = 0;
        sweatdrop->ad = -255 / 2;
#else
        sweatdrop->rd = -220 / 2;
        sweatdrop->gd = -220 / 2;
        sweatdrop->bd = -255 / 2;
#endif
        sweatdrop->vector.x = 0;
        sweatdrop->vector.y = 0;
    }
}

/*** PILOT ANIMATION ***/
void animate_pilots (void) {
    int p;
    for(p=0;p<4;p++) {
        struct Pilot *pilot = &players[p].pilot;
        if(players[p].state != ALIVE || players[p].ship)
            continue;

        animate_walker(&pilot->walker,0,NULL);

        /* Cool down weapon */
        if (pilot->weap_cooloff > 0)
            pilot->weap_cooloff--;

        /* Autoaim */
        if(pilot->lock==0) {
            double dist;
            int nearest = find_nearest_enemy(pilot->walker.physics.x,pilot->walker.physics.y, p, &dist);
            if (nearest>=0 && dist < 150) {
                pilot->attack_vector.x = (players[nearest].ship->physics.x - pilot->walker.physics.x)/dist;
                pilot->attack_vector.y = (players[nearest].ship->physics.y - pilot->walker.physics.y)/dist;
                pilot->crosshair_color = col_plrs[nearest];
            } else {
                pilot->crosshair_color = col_white;
            }
        } else {
            pilot->crosshair_color = col_white;
        }

        /* Rope simulation */
        if(pilot->rope) {
            if(pilot->ropectrl<0) {
                if(pilot->rope->nodelen>pilot_rope_minlen)
                    pilot->rope->nodelen -= 0.05;
            } else if(pilot->ropectrl>0) {
                if(pilot->rope->nodelen<pilot_rope_maxlen)
                    pilot->rope->nodelen += 0.05;
            }

            animate_spring(pilot->rope);
        }

        /* Stop parachuting when hitting something other than air */
        if((pilot->walker.physics.hitground||pilot->walker.physics.underwater)
                && pilot->parachuting)
            undeploy_parachute(pilot);

        /* Check if falling too fast and hit the ground */
        if(ter_walkable(pilot->walker.physics.hitground)) {

            if(pilot->toofast > PILOT_TOOFAST) {
                kill_pilot(pilot);
            }
        } else {
            if(fabs(pilot->walker.physics.vel.y)>=LETHAL_VELOCITY) {
                make_sweatdrops(pilot);
                pilot->toofast++;
            } else {
                pilot->toofast = 0;
            }
        }
    }
}

/* Draw the crosshair */
static void draw_pilot_crosshair (const struct Pilot *pilot, const SDL_Rect *targ)
{
    const int w = pilot->sprite[pilot->parachuting?2:0]->w;
    const int h = pilot->sprite[pilot->parachuting?2:0]->h;
    const int x = targ->x + w/2 + pilot->attack_vector.x * 12;
    const int y = targ->y + h/2 + pilot->attack_vector.y * 12;
    putpixel (screen, x, y, pilot->crosshair_color);
    putpixel (screen, x, y + 2, pilot->crosshair_color);
    putpixel (screen, x, y - 2, pilot->crosshair_color);
    putpixel (screen, x - 2, y, pilot->crosshair_color);
    putpixel (screen, x + 2, y, pilot->crosshair_color);
}

/* Draw pilots on all active viewports */
void draw_pilots (void)
{
    struct dllist *lst = pilot_list;
    
    while(lst) {
        struct Pilot *pilot = lst->data;
        unsigned char sn;
        SDL_Rect rect;
        int plr;

        if (pilot->parachuting)
            sn = PARACHUTE_FRAME;
        else
            sn =0;// abs (players[p].pilot.walking / 2) - 2;
        if (sn > PARACHUTE_FRAME)
            sn = 0;
        for (plr = 0; plr < 4; plr++) {
            /* Draw the pilot */
            if (players[plr].state==ALIVE || players[plr].state==DEAD) {
                rect.x =
                    Round(pilot->walker.physics.x) -
                    cam_rects[plr].x + viewport_rects[plr].x -
                    pilot->sprite[sn]->w/2;
                rect.y =
                    Round(pilot->walker.physics.y) -
                    cam_rects[plr].y + viewport_rects[plr].y -
                    pilot->sprite[sn]->h;
                if ((rect.x >= viewport_rects[plr].x
                     && rect.x < viewport_rects[plr].x + cam_rects[plr].w)
                    && (rect.y >= viewport_rects[plr].y
                        && rect.y < viewport_rects[plr].y + cam_rects[plr].h)) {
                    SDL_BlitSurface (pilot->sprite[sn], NULL,
                                     screen, &rect);
                    if(&players[plr].pilot == pilot)
                        draw_pilot_crosshair (pilot, &rect);
                }
                if(pilot->rope) {
                    draw_spring(pilot->rope,&cam_rects[plr],&viewport_rects[plr]);
                }
            }
        }
        lst=lst->next;
    }
}

/* Shoot in the direction of attack_vector */
static void pilot_shoot(struct Pilot *pilot) {
    double xoff,yoff;
    xoff = pilot->attack_vector.x * (pilot->walker.physics.radius+1);
    yoff = pilot->attack_vector.y * (pilot->walker.physics.radius+1);
    add_projectile(make_bullet (
                pilot->walker.physics.x + xoff,
                pilot->walker.physics.y + yoff,
                addVectors(pilot->walker.physics.vel,
                    multVector(pilot->attack_vector,10.0))));
    pilot->weap_cooloff = 7;
}

/* Shoot ninjarope at the direction pointed by v */
static void pilot_shoot_rope(struct Pilot *pilot, Vector v) {
    int r;
    v = multVector(v,24.0);
    pilot->rope = create_spring(&pilot->walker.physics,6.0,10);
    pilot->rope->tail->vel = addVectors(pilot->walker.physics.vel,v);

#if 1
    for(r=pilot->rope->nodecount-1;r>=0;r--) {
        v = multVector(v,0.1);
        pilot->rope->nodes[r].vel = addVectors(pilot->walker.physics.vel,v);
    }
#endif
}

/* Detach the ninjarope */
void pilot_detach_rope(struct Pilot *pilot) {
    if(pilot->rope) {
        free_spring(pilot->rope);
        pilot->rope = NULL;
    }
}

/* Interpret commands from game controller */
void control_pilot (int plr) {
    struct Pilot *pilot = &players[plr].pilot;
    pilot->lock = players[plr].controller.weapon2;
    /* Horizontal axis */
    if (players[plr].controller.axis[1] > 0) {
        /* Walk/glide left */
        if(!pilot->lock)
            pilot->walker.walking = -1;
        if(pilot->attack_vector.x > 0 && pilot->crosshair_color==col_white) {
            pilot->attack_vector.x = -pilot->attack_vector.x;
        }
    } else if (players[plr].controller.axis[1] < 0) {
        /* Walk/glide right */
        if(!pilot->lock)
            pilot->walker.walking = 1;
        if(pilot->attack_vector.x < 0 && pilot->crosshair_color==col_white) {
            pilot->attack_vector.x = -pilot->attack_vector.x;
        }
    } else {
        pilot->walker.walking = 0;
    }
    /* Vertical axis */
    if (players[plr].controller.axis[0] > 0) {
        /* Jump/swim/climb up/aim up/shoot rope at a 45 degree angle */
        if(pilot->lock || pilot->parachuting) {
            rotateVector(&pilot->attack_vector,
                    pilot->attack_vector.x>0?-0.5:0.5);
        } else if(pilot->rope) {
            pilot->ropectrl = -1;
        } else {
            if(pilot->walker.physics.hitground==0 &&
                    pilot->walker.physics.underwater==0 &&
                    pilot->parachuting==0)
            {
                pilot_shoot_rope(pilot,makeVector(pilot->attack_vector.x>0?1:-1,-1));
            } else {
                walker_jump(&pilot->walker);
            }
        }
    } else if (players[plr].controller.axis[0] < 0) {
        /* Swim/climb down, shoot rope straight up/aim down */
        if(pilot->lock || pilot->parachuting) {
            rotateVector(&pilot->attack_vector,pilot->attack_vector.x<0?-0.5:0.5);
        } else if(pilot->rope) {
                pilot->ropectrl = 1;
        } else {
            if(pilot->walker.physics.underwater==0)
                pilot_shoot_rope(pilot,makeVector(0,-1));
            else
                walker_dive(&pilot->walker);
        }
    } else {
        pilot->walker.dive = 0;
        pilot->walker.jump = 0;
        pilot->ropectrl = 0;
    }

    /* Weapon1 button */
    if (players[plr].controller.weapon1) {
        /* Shoot / Board another players ship by force */
#if 0 /* TODO take over ship */
        if (pilot->updown == -1 && pilot->rope_ship
            && abs (pilot->rope_ship->physics.x - Round(pilot->x)) < 8
            && abs (pilot->rope_ship->physics.y - Round(pilot->y) + 4) < 8) {
            int targ = find_player (pilot->rope_ship);
            if (targ < 0)
                return;
            players[plr].ship = pilot->rope_ship;
            pilot->rope_ship = NULL;
            players[targ].ship = NULL;
            players[targ].pilot.x = pilot->x;
            players[targ].pilot.y = pilot->y;
            players[targ].pilot.rope = 0;
            return;
        }
#endif
        if(pilot->weap_cooloff==0)
            pilot_shoot(&players[plr].pilot);
    }

    /* Weapon2 button */
    if (players[plr].controller.weapon2) {
        /* Open parachute / Get off the rope / Recall ship */
        if (pilot->rope)
            pilot_detach_rope(pilot);
        else if (pilot->walker.physics.hitground==0 && pilot->parachuting == 0)
            deploy_parachute(pilot);
        else if (pilot->walker.physics.hitground == TER_BASE &&
                game_settings.recall)
            recall_ship (plr);
    }
}

/* Find the nearest player that is outside his/hers ship       */
/* Returns the number of that player. my_team excludes a team */
int find_nearest_pilot(float x,float y,int myplr, double *dist) {
    int p, plr = -1;
    double distance = 9999999, d;
    for (p = 0; p < 4; p++) {
        if (same_team(p,myplr)) continue;
        if (players[p].state==ALIVE && players[p].ship==NULL) {
            d = hypot (players[p].pilot.walker.physics.x - x,
                       players[p].pilot.walker.physics.y - y);
            if (d < distance) {
                plr = p;
                distance = d;
            }
        }
    }
    if (dist)
        *dist = distance;
    return plr;
}


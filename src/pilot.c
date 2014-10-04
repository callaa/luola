/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
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

#include "defines.h"
#include "console.h"
#include "level.h"
#include "particle.h"
#include "player.h"
#include "animation.h"
#include "critter.h"
#include "ship.h"
#include "ldat.h"
#include "fs.h"

/* Exported globals */
char pilot_any_ejected;

/* Internally used globals */
static SDL_Surface *pilot_sprite[4][3];        /* Normal,Normal2, Parachute */

/* Internally used functions */
static void draw_pilot_crosshair (Pilot * pilot, SDL_Rect * rect);
static void pilot_rope_code (Pilot * pilot);
static void pilot_motion_code (Pilot * pilot);
static void pilot_rope_state (Pilot * pilot, int attach);

/* Initialize pilot code */
int init_pilots (void) {
    int r, p;
    SDL_Surface *tmp;
    SDL_Rect rect, targrect;
    LDAT *playerdata;
    pilot_any_ejected = 0;
    playerdata =
        ldat_open_file (getfullpath (GFX_DIRECTORY, "player.ldat"));
    if(!playerdata) return 1;
    /* Load pilot sprites */
    for (r = 0; r < 3; r++) {   /* Pilots have 3 frames */
        rect.y = 0;
        targrect.x = 0;
        targrect.y = 0;
        tmp = load_image_ldat (playerdata, 0, 2, "PILOT", r);   /* Use a colour key instead of alpha */
        for (p = 0; p < 4; p++) {       /* There is a total of 4 pilots in this game */
            rect.w = tmp->w / 4;
            rect.h = tmp->h;
            pilot_sprite[p][r] =
                SDL_CreateRGBSurface (tmp->flags, rect.w, rect.h,
                                      tmp->format->BitsPerPixel,
                                      tmp->format->Rmask, tmp->format->Gmask,
                                      tmp->format->Bmask, tmp->format->Amask);
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
    ldat_free (playerdata);
    return 0;
}

/* Initialize a single pilot */
void init_pilot(Pilot *pilot,int playernum) {
    int r;
    memset (pilot, 0, sizeof (Pilot));
    for (r = 0; r < 3; r++)
        pilot->sprite[r] = pilot_sprite[playernum][r];
    pilot->maxspeed = MAXSPEED;
    pilot->hy = pilot->sprite[0]->h;
    pilot->hx = pilot->sprite[0]->w / 2;
    pilot->attack_vector.x = 1.0;
    pilot->attack_vector.y = 0.0;
}

/*** PILOT ANIMATION ***/
void animate_pilots (void) {
    signed char solid, solid2;
    double newx,newy;
    Pilot *pilot;
    Vector tmpv;
    int p;
    if (pilot_any_ejected == 0)
        return;
    for (p = 0; p < 4; p++) {
        if (players[p].state != ALIVE || players[p].ship)
            continue;
        pilot = &players[p].pilot;
        if (pilot->weap_cooloff > 0)
            pilot->weap_cooloff--;
        /* Pilot controls */
        pilot_motion_code (pilot);
        if (pilot->rope)
            pilot_rope_code (pilot);
        /* Pilot autoaim */
        if (pilot->lock == 0) {
            double dist;
            int nearest = find_nearest_player(Round(pilot->x),Round(pilot->y), p, &dist);
            if (nearest>=0 && dist < 150) {
                pilot->attack_vector.x = -(players[nearest].ship->x - pilot->x)/dist;
                pilot->attack_vector.y = -(players[nearest].ship->y - pilot->y)/dist;
                pilot->crosshair_color = col_plrs[nearest];
            } else {
                pilot->crosshair_color = col_white;
            }
        } else {
            pilot->crosshair_color = col_white;
        }
        /* Pilot physics */
        tmpv = pilot->vector;
        solid2 = hit_solid(Round(pilot->x)+pilot->hx,Round(pilot->y)+pilot->hy);
        if (solid2 == TER_WALKWAY)
            solid2 = TER_FREE;
        if (pilot->rope != USEROPE) {
            if (solid2 < 0) {
                if (solid2 == -2)
                    tmpv.y = MAXSPEED / 2;
                else if (solid2 == -3)
                    tmpv.x = -MAXSPEED / 2;
                else if (solid2 == -4)
                    tmpv.y = -MAXSPEED / 2;
                else if (solid2 == -5)
                    tmpv.x = MAXSPEED / 2;
                else
                    tmpv.y -= gravity.y * 1.04;
                tmpv.x /= 1.1;
            } else {
                tmpv.y += gravity.y / (1.0 + pilot->parachuting * 2.0);
                tmpv.x /= 1.02;
            }
            if (tmpv.x > MAXSPEED / ((solid2 < 0) + 1))
                tmpv.x = MAXSPEED / ((solid2 < 0) + 1);
            if (tmpv.x < -MAXSPEED / ((solid2 < 0) + 1))
                tmpv.x = -MAXSPEED / ((solid2 < 0) + 1);
            if (tmpv.y > pilot->maxspeed / ((solid2 < 0) + 1))
                tmpv.y = pilot->maxspeed / ((solid2 < 0) + 1);
            if (tmpv.y < -pilot->maxspeed / ((solid2 < 0) + 1))
                tmpv.y = -pilot->maxspeed / ((solid2 < 0) + 1);
            pilot->vector = tmpv;
            newx = pilot->x - tmpv.x;
            newy = pilot->y - tmpv.y;
            /* Warn the player if falling too fast */
            if (pilot->vector.y < -(MAXSPEED - 0.1)) {
                Particle *sweatdrop;
                int r;
                pilot->toofast++;
                for (r = 0; r < 6; r++) {
                    sweatdrop =
                        make_particle (newx + (pilot->sprite[0]->w / 2) +
                                       (8 - rand () % 16), newy - rand () % 12,
                                       2);
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
            } else {
                pilot->toofast=0;
            }
        } else {                /* Rope based movement */
            newx = pilot->rope_x +
                sin (pilot->rope_th) * (double) pilot->rope_len;
            newy = pilot->rope_y +
                cos (pilot->rope_th) * (double) pilot->rope_len;
        }
        solid=0;
        if (newx < 0) {
            newx = 0;
            solid=TER_INDESTRUCT;
        } else if (newx + pilot->hx > lev_level.width - 1) {
            newx = lev_level.width - 1 - pilot->hx;
            solid=TER_INDESTRUCT;
        }
        if (newy < 0)
            newy = 0;
        else if (newy + pilot->hy > lev_level.height - 1)
            newy = lev_level.height - 1 - pilot->hy;
        if(!solid)
            solid = is_walkable (Round(newx) + pilot->hx,
                    Round(newy) + pilot->hy);
        pilot->onbase = 0;
        if (solid) {
            if (pilot->toofast>PILOT_TOOFAST && pilot->rope!=USEROPE) {
                /* Player hit the ground too fast */
                splatter (Round(pilot->x), Round(pilot->y), Blood);
                buryplayer (p);
            }
            if (solid2 == TER_BASE)
                pilot->onbase = 1;
            pilot->vector.x = 0;
            pilot->vector.y = 0;
            pilot->jumping = 0;
            newx = pilot->x;
            if (newy < pilot->y)
                newy = pilot->y;
            else
                newy = Round(newy);
            if (pilot->parachuting) {
                /* Stop parachuting */
                pilot->parachuting = 0;
                pilot->maxspeed = MAXSPEED;
                newy += pilot->sprite[2]->h - pilot->sprite[0]->h;
                newx += pilot->sprite[2]->w / 2 - pilot->sprite[0]->w / 2;
                pilot->hx = pilot->sprite[0]->w / 2;
                pilot->hy = pilot->sprite[0]->h;
            } else if (pilot->rope == USEROPE) {
                /* Bounce of walls while roping */
                pilot->rope_v *= -0.5;
                pilot->rope_th += pilot->rope_v;
                newx = pilot->rope_x +
                    sin (pilot->rope_th) * (double) pilot->rope_len;
                newy = pilot->rope_y +
                    cos (pilot->rope_th) * (double) pilot->rope_len;
                if (newx < 0)
                    newx = 0;
                else if (newx + pilot->hx > lev_level.width - 1)
                    newx = lev_level.width - 1 - pilot->hx;
                if (newy < 0)
                    newy = 0;
                else if (newy + pilot->hy > lev_level.height - 1)
                    newy = lev_level.height - 1 - pilot->hy;
                if (pilot->rope_ship) {
                    pilot->rope_ship->vector.x *= -1.0;
                    pilot->rope_ship->vector.y *= -1.0;
                    pilot->rope_ship->x = pilot->rope_x;
                    pilot->rope_ship->y = pilot->rope_y;
                }
            }
        } else if (is_water (newx + pilot->hx, newy + pilot->hy)) {
            if (pilot->parachuting) {
                pilot->parachuting = 0;
                pilot->maxspeed = MAXSPEED;
                newy += pilot->sprite[2]->h - pilot->sprite[0]->h;
                newx += pilot->sprite[2]->w / 2 - pilot->sprite[0]->w / 2;
                pilot->hx = pilot->sprite[0]->w / 2;
                pilot->hy = pilot->sprite[0]->h;
            }
        }
        pilot->x = newx;
        pilot->y = newy;
    }
}

/* Detach rope. Used from outside this module */
void pilot_detach_rope(Pilot *pilot) {
    pilot_rope_state(pilot, 0);
}

/* Start/stop roping with apropriate physics */
static void pilot_rope_state (Pilot * pilot, int attach) {
    if (attach) {
        pilot->rope = USEROPE;
        pilot->vector.x = 0;
        pilot->vector.y = 0;
        pilot->rope_v = 0;
        pilot->rope_th =
            atan2 (pilot->x - pilot->rope_x, pilot->y - pilot->rope_y);
    } else {
        double phi, v;
        if (pilot->rope == USEROPE) {
            if (pilot->rope_th < 0) {
                phi = -M_PI_2;
                v = -pilot->rope_v;
            } else {
                phi = M_PI_2;
                v = pilot->rope_v;
            }
            pilot->vector.x =
                -sin (pilot->rope_th + phi) * (double) pilot->rope_len * v;
            pilot->vector.y =
                -cos (pilot->rope_th + phi) * (double) pilot->rope_len * v;
        }
        pilot->rope = NOROPE;
    }
}

/* Pilot rope code */
static void pilot_rope_code (Pilot * pilot) {
    struct Ship *ship;
    if (pilot->rope == ROPE_EXTENDING) {
        pilot->rope_len += hypot (pilot->rope_dir * ROPE_SPEED, ROPE_SPEED);
        if (pilot->rope_len > MAX_ROPE_LEN)
            pilot->rope = 0;
        else {
            if (hit_solid_line
                (pilot->rope_x, pilot->rope_y,
                 pilot->x + pilot->rope_len * pilot->rope_dir,
                 pilot->y - pilot->rope_len, &pilot->rope_x,
                 &pilot->rope_y) > 0) {
                pilot->rope_ship = NULL;
                pilot->rope_len =
                    hypot (pilot->x - pilot->rope_x,
                           pilot->y - pilot->rope_y);
                if (pilot->rope_len == 0)
                    pilot->rope_len = 1;
                pilot_rope_state (pilot, 1);
            } else
                if ((ship =
                     hit_ship (pilot->x + pilot->rope_len * pilot->rope_dir,
                               pilot->y - pilot->rope_len, NULL, 5))) {
                pilot->rope_ship = ship;
                pilot_rope_state (pilot, 1);
            } else {
                pilot->rope_x =
                    pilot->x + (pilot->rope_len * pilot->rope_dir);
                pilot->rope_y = pilot->y - pilot->rope_len;
            }
        }
    } else {
        /* Rope physics */
        double dd;
        if (pilot->rope_ship) {
            pilot->rope_x = pilot->rope_ship->x;
            pilot->rope_y = pilot->rope_ship->y;
            pilot->rope_th =
                atan2 (pilot->x - pilot->rope_x, pilot->y - pilot->rope_y);
        }
        dd = -(GRAVITY / (double) pilot->rope_len) * sin (pilot->rope_th);
        pilot->rope_v += dd;
        pilot->rope_v /= 1.01;
        pilot->rope_th += pilot->rope_v;
    }
}

/* Pilot controls, walking around and shooting */
static void pilot_motion_code (Pilot * pilot) {
    signed char solid;
    int i1;
    if (Round(pilot->y) + pilot->hy >= lev_level.height - 1)
        solid = TER_INDESTRUCT;
    else if (Round(pilot->x) + pilot->hx >= lev_level.width - 1)
        solid = TER_INDESTRUCT;
    else
        solid = lev_level.solid[Round(pilot->x)+pilot->hx][Round(pilot->y)+pilot->hy];
    /* Directional controls */
    if (pilot->walking && !pilot->parachuting) {
        if (pilot->walking < -1)
            pilot->walking++;
        else if (pilot->walking > 1)
            pilot->walking--;
    }
    if (pilot->walking < 0 && pilot->lock == 0) {       /* Walk/glide left */
        if (pilot->walking == -1)
            pilot->walking = -6;
        if (solid != TER_FREE && solid != TER_TUNNEL && solid != TER_WALKWAY) {
            if (solid == TER_WATER
                || (solid >= TER_WATERFU && solid <= TER_WATERFL)) {
                pilot->vector.x += 1;
            } else {
                pilot->x -= 3;
                if (pilot->x < 0)
                    pilot->x = 0;
                i1 = find_nearest_terrain (Round(pilot->x) + pilot->hx,
                        Round(pilot->y), pilot->hy);
                if(Round(pilot->x)<0) pilot->x=0;
                else if(Round(pilot->x)>=lev_level.width) pilot->x=lev_level.width-1;
                if (i1 != -1)
                    pilot->y = i1;
                else if (lev_level.solid[Round(pilot->x)][Round(pilot->y)]!=
                        TER_FREE
                        && lev_level.solid[Round(pilot->x)][Round(pilot->y)] !=
                        TER_TUNNEL
                        && lev_level.solid[Round(pilot->x)][Round(pilot->y)] !=
                        TER_WALKWAY)
                    pilot->x += 3;
            }
        } else if (pilot->jumping || pilot->parachuting) {
            pilot->vector.x += 1;
            pilot->jumping = 0;
        } else if (pilot->rope == USEROPE && pilot->lock == 0
                   && pilot->rope_th >= 0)
            pilot->rope_v -= 0.2 / (double) pilot->rope_len;

    }
    if (pilot->walking > 0 && pilot->lock == 0) {       /* Walk/glide right */
        if (pilot->walking == 1)
            pilot->walking = 6;
        if (solid != TER_FREE && solid != TER_TUNNEL && solid != TER_WALKWAY) {
            if (solid == TER_WATER
                || (solid >= TER_WATERFU && solid <= TER_WATERFL)) {
                pilot->vector.x -= 1;
            } else {
                pilot->x += 3;
                if (Round(pilot->x) + pilot->hx >= lev_level.width)
                    pilot->x = lev_level.width - pilot->hx - 1;
                i1 = find_nearest_terrain (Round(pilot->x) + pilot->hx,
                        Round(pilot->y), pilot->hy);
                if(Round(pilot->x)<0) pilot->x=0;
                else if(Round(pilot->x)>=lev_level.width) pilot->x=lev_level.width-1;
                if (i1 != -1)
                    pilot->y = i1;
                else if (lev_level.solid[Round(pilot->x)][Round(pilot->y)]!=
                        TER_FREE
                        && lev_level.solid[Round(pilot->x)][Round(pilot->y)]!=
                        TER_TUNNEL
                        && lev_level.solid[Round(pilot->x)][Round(pilot->y)]!=
                        TER_WALKWAY)
                    pilot->x -= 3;
            }
        } else if (pilot->jumping || pilot->parachuting) {
            pilot->vector.x -= 1;
            pilot->jumping = 0;
        } else if (pilot->rope == USEROPE && pilot->lock == 0
                   && pilot->rope_th <= 0)
            pilot->rope_v += 0.2 / (double) pilot->rope_len;
    }
    if (pilot->updown == -1 && pilot->lock == 0) {    /* Jump/swim/climb up */
        if (pilot->rope == USEROPE) {
            if (pilot->rope_len > 4) {  /* Retract rope */
                int newx,newy;
                    pilot->rope_len -= 4;
                newx = pilot->rope_x +
                    sin (pilot->rope_th) * (double) pilot->rope_len;
                newy = pilot->rope_y +
                    cos (pilot->rope_th) * (double) pilot->rope_len;
                if(newx<0 || newx>=lev_level.width || newy<0 || newy>=lev_level.height || is_walkable(newx,newy)) pilot->rope_len+=4;
            }
        } else if (solid != TER_FREE && solid != TER_TUNNEL
                   && solid != TER_WALKWAY) { /* Jump or climb up */
            if (solid == TER_WATER
                || (solid >= TER_WATERFU && solid <= TER_WATERFL)) {
                pilot->vector.y += 1;
            } else {
                pilot->vector.y += 3;
                pilot->jumping = 1;
                pilot->updown = 0;
            }
        } else if (pilot->parachuting == 0) {/* Shoot rope at 45 degree angle */
            pilot->rope = ROPE_EXTENDING;
            pilot->rope_dir = (pilot->attack_vector.x < 0) ? 1 : -1;
            pilot->rope_len = 0;
            pilot->rope_ship = NULL;
            pilot->updown = 0;
            pilot->rope_x =
                Round(pilot->x) + (pilot->rope_dir > 0) * pilot->sprite[0]->w;
            pilot->rope_y = Round(pilot->y);
        }
    } else if (pilot->updown == 1 && pilot->lock == 0) { /* Extend rope,dive or shoot the rope straight up */
        if (pilot->rope == USEROPE) {
            if (pilot->rope_len < MAX_ROPE_LEN - 4
                && hit_solid (Round(pilot->x) + pilot->hx,
                              Round(pilot->y) + pilot->hy) == 0) {
                int newx,newy;
                pilot->rope_len += 4;
                newx = pilot->rope_x +
                    sin (pilot->rope_th) * (double) pilot->rope_len;
                newy = pilot->rope_y +
                    cos (pilot->rope_th) * (double) pilot->rope_len;
                if(newx<0 || newx>=lev_level.width || newy<0 || newy>=lev_level.height || is_walkable(newx,newy)) pilot->rope_len-=4;
            }
        } else
            if (solid == TER_WATER
                || (solid >= TER_WATERFU && solid <= TER_WATERFL)) {
            pilot->vector.y -= 1;
        } else if (pilot->parachuting == 0) {
            pilot->rope = ROPE_EXTENDING;
            pilot->rope_dir = 0;
            pilot->rope_len = 0;
            pilot->rope_ship = NULL;
            pilot->updown = 0;
            pilot->rope_x = Round(pilot->x);
            pilot->rope_y = Round(pilot->y);
        }
    }
}

void draw_pilots (void)
{
    int p, plr;
    int rx, ry, rx2, ry2;
    SDL_Rect rect;
    unsigned char sn;
    if (pilot_any_ejected == 0)
        return;
    for (p = 0; p < 4; p++) {
        if (players[p].state != ALIVE || players[p].ship)
            continue;
        if (players[p].pilot.parachuting)
            sn = 2;
        else
            sn = abs (players[p].pilot.walking / 2) - 2;
        if (sn > 2)
            sn = 0;
        for (plr = 0; plr < 4; plr++) {
            if (players[plr].state==ALIVE || players[plr].state==DEAD) {
                rect.x =
                    Round(players[p].pilot.x) - cam_rects[plr].x +
                    lev_rects[plr].x;
                rect.y =
                    Round(players[p].pilot.y) - cam_rects[plr].y +
                    lev_rects[plr].y;
                if ((rect.x >= lev_rects[plr].x
                     && rect.x < lev_rects[plr].x + cam_rects[plr].w)
                    && (rect.y >= lev_rects[plr].y
                        && rect.y < lev_rects[plr].y + cam_rects[plr].h)) {
                    SDL_BlitSurface (players[p].pilot.sprite[sn], NULL,
                                     screen, &rect);
                    draw_pilot_crosshair (&players[p].pilot, &rect);
                }
                if (players[p].pilot.rope) {
                    rx = rect.x;
                    ry = rect.y;
                    rx2 =
                        players[p].pilot.rope_x - cam_rects[plr].x +
                        lev_rects[plr].x;
                    ry2 =
                        players[p].pilot.rope_y - cam_rects[plr].y +
                        lev_rects[plr].y;
                    if (clip_line
                        (&rx, &ry, &rx2, &ry2, lev_rects[plr].x,
                         lev_rects[plr].y,
                         lev_rects[plr].x + lev_rects[plr].w,
                         lev_rects[plr].y + lev_rects[plr].h)) {
                        draw_line (screen, rx, ry, rx2, ry2, col_rope);
                        if (ry2 ==
                            players[p].pilot.rope_y - cam_rects[plr].y)
                            putpixel (screen, rx2, ry2, col_white);
                    }
                }
            }
        }
    }
}

static void draw_pilot_crosshair (Pilot *pilot, SDL_Rect *rect)
{
    int x = rect->x + (pilot->hx) - pilot->attack_vector.x * 12;
    int y = rect->y + pilot->hy / 2 - pilot->attack_vector.y * 12;
    putpixel (screen, x, y, pilot->crosshair_color);
    putpixel (screen, x, y + 2, pilot->crosshair_color);
    putpixel (screen, x, y - 2, pilot->crosshair_color);
    putpixel (screen, x - 2, y, pilot->crosshair_color);
    putpixel (screen, x + 2, y, pilot->crosshair_color);
}

void control_pilot (int plr) {
    Pilot *pilot;
    Uint8 solid;

    pilot = &players[plr].pilot;
    solid=lev_level.solid[Round(pilot->x)+pilot->hx][Round(pilot->y)+pilot->hy];
    pilot->lock = players[plr].controller.weapon2;
    if (players[plr].controller.axis[1] > 0) {
        /* Walk/glide left */
        pilot->walking = -1;
        if(pilot->attack_vector.x < 0 && pilot->crosshair_color==col_white) {
            pilot->attack_vector.x = -pilot->attack_vector.x;
        }
    } else if (players[plr].controller.axis[1] < 0) {
        /* Walk/glide right */
        pilot->walking = 1;
        if(pilot->attack_vector.x > 0 && pilot->crosshair_color==col_white) {
            pilot->attack_vector.x = -pilot->attack_vector.x;
        }
    } else {
        pilot->walking = 0;
    }
    if (players[plr].controller.axis[0] > 0) {
        /* Jump/swim/climb up/aim up */
        pilot->updown = -1;
        if(pilot->lock || pilot->parachuting)
            rotateVector(&pilot->attack_vector,pilot->attack_vector.x<0?-0.5:0.5);
    } else if (players[plr].controller.axis[0] < 0) {
        /* Swim/climb down, shoot rope straight up/aim down */
        pilot->updown = 1;
        if(pilot->lock || pilot->parachuting)
            rotateVector(&pilot->attack_vector,pilot->attack_vector.x<0?0.5:-0.5);
    } else {
        pilot->updown = 0;
    }

    if (players[plr].controller.weapon2 && pilot->lock_btn2 == 0) {
        /* Open parachute / Get off the rope */
        pilot->lock_btn2 = 1;
        if (pilot->rope)
            pilot_rope_state (pilot, 0);
        else if ((solid == TER_FREE || solid == TER_TUNNEL
                  || solid == TER_WALKWAY) && pilot->parachuting == 0) {
            pilot->parachuting = 1;
            pilot->maxspeed = MAXSPEED / 2;
            pilot->y -= pilot->sprite[2]->h - pilot->sprite[0]->h;
            pilot->x -= pilot->sprite[2]->w / 2 - pilot->sprite[0]->w / 2;
            pilot->hx = pilot->sprite[2]->w / 2;
            pilot->hy = pilot->sprite[2]->h;
        } else if (solid == TER_BASE && game_settings.recall) {
            recall_ship (plr);
        }
    } else {
        pilot->lock_btn2 = 0;
    }
    if (players[plr].controller.weapon1) {
        /* Shoot / Board another players ship by force */
        Projectile *p;
        if (pilot->weap_cooloff > 0)
            return;
        if (pilot->updown == -1 && pilot->rope_ship
            && abs (pilot->rope_ship->x - Round(pilot->x)) < 8
            && abs (pilot->rope_ship->y - Round(pilot->y) + 4) < 8) {
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
        p = make_projectile (pilot->x + pilot->hx, pilot->y + pilot->hy / 2.0,
                             pilot->attack_vector);
        p->ownerteam = player_teams[plr];
        add_projectile (p);
        pilot->weap_cooloff = 7;
    }
}

/** Check if a projectile hits a pilot and kill it if it does **/
int hit_pilot (int x, int y,int team) {
    int p, ofsx, ofsy, h, w;
    if (pilot_any_ejected == 0)
        return 0;
    for (p = 0; p < 4; p++)
        if (players[p].state==ALIVE && players[p].ship==NULL &&
                (team<0 || (team>=0 && team!=player_teams[p]))) {
            if (players[p].pilot.parachuting) {
                ofsx = 6;
                ofsy = 10;
                w = 6;
                h = 10;
            } else {
                ofsx = 0;
                ofsy = 0;
                w = players[p].pilot.sprite[0]->w;
                h = players[p].pilot.sprite[0]->h;
            }
            if (x >= players[p].pilot.x + ofsx
                && y >= players[p].pilot.y + ofsy)
                if (x <= players[p].pilot.x + ofsx + w
                    && y <= players[p].pilot.y + ofsy + h) {
                    splatter (players[p].pilot.x + ofsx,
                              players[p].pilot.y + ofsy, Blood);
                    buryplayer (p);
                    return 1;
                }
        }
    return 0;
}

/* Find the nearest player that is outside his/hers ship       */
/* Returns the number of that player. not_this excludes a team */
int find_nearest_pilot(int myX,int myY,int not_this, double *dist) {
    int p, plr = -1;
    double distance = 9999999, d;
    for (p = 0; p < 4; p++) {
        if (not_this >= 0)
            if (player_teams[p] == player_teams[not_this])
                continue;
        if (players[p].state==ALIVE && players[p].ship==NULL) {
            d = hypot (abs (players[p].pilot.x - myX),
                       abs (players[p].pilot.y - myY));
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


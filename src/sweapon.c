/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : sweapon.c
 * Description : Special weapon specific code
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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "sweapon.h"
#include "particle.h"
#include "weather.h"
#include "player.h"
#include "level.h"
#include "ship.h"

/* Homing missile. Seeks players */
void missile_movement (Projectile * proj)
{
    double f0, f1;
    int p;
    make_particle (proj->x, proj->y, 5);
    p = find_nearest_player (proj->x, proj->y, proj->ownerteam, &f0);
    if (p == -1)
        return;
    /* If player is invisible, homing missile can only track it from afar */
    if((players[p].ship->visible == 0 && (players[p].ship->tagged || f0 > 300)) ||
            players[p].ship->visible)
        f1 = atan2 (players[p].ship->x - proj->x, players[p].ship->y - proj->y);
    else
        f1 = proj->angle;
    if (proj->angle < 0 && f1 > 0) {
        if ((f1 - proj->angle) > (fabs (-M_PI - proj->angle) + M_PI - f1))
            proj->angle -= 0.2;
        else
            proj->angle += 0.2;
    } else if (proj->angle > 0 && f1 < 0) {
        if ((proj->angle - f1) > (fabs (-M_PI - f1) + M_PI - proj->angle))
            proj->angle += 0.2;
        else
            proj->angle -= 0.2;
    } else if (proj->angle < f1)
        proj->angle += 0.2;
    else if (proj->angle != f1)
        proj->angle -= 0.2;
    if (proj->angle < -M_PI)
        proj->angle = M_PI;
    else if (proj->angle > M_PI)
        proj->angle = -M_PI;
    if (players[p].ship->shieldup && f0 < 70) {
        if (proj->angle > f1)
            proj->angle += 1;
        else
            proj->angle -= 1;
    }
    proj->vector =
        makeVector (sin (proj->angle) * -6.0, cos (proj->angle) * -6.0);
}

void magmine_movement (Projectile * proj) {
    struct Ship *ship;
    double f1;
    ship = find_nearest_ship (proj->x, proj->y, NULL, &f1);
    if (ship && f1 < 60) {
        Vector tmpv;
        double n;
        tmpv.x=ship->x-proj->x;
        tmpv.y=ship->y-proj->y;
        n=fabs(tmpv.x)+fabs(tmpv.y);
        tmpv.x/=n; tmpv.y/=n;
        if(f1==0) f1=0.01;
        proj->vector.x=-tmpv.x*180.0/f1;
        proj->vector.y=-tmpv.y*180.0/f1;
    } else {
        proj->vector.x = 0;
        proj->vector.y = 0;
    }
}

void dividingmine_movement (Projectile * proj, struct dllist *others) {
    int crowd = 0;
    double d;
    while (others) {
        Projectile *projectile=others->data;
        /* Tip. Comment out the second half of this if clause for extra fun... ;) */
        if (projectile != proj
            && projectile->type == DividingMine) {
            d = hypot (projectile->x - proj->x, projectile->y - proj->y);
            if (d < 55.0) {
                crowd++;
                if (d < 25.0) {
                    proj->vector.x +=
                        ((projectile->x -
                          proj->x) / (abs (projectile->x - proj->x) +
                                      0.1)) / 1.7;
                    proj->vector.y +=
                        ((projectile->y -
                          proj->y) / (abs (projectile->y - proj->y) +
                                      0.1)) / 1.7;
                }
            }
        }
        others = others->next;
    }
    if (crowd > 6)
        proj->var1 = 0;
    else if (proj->var1 == 0)
        proj->var1 =
            DIVIDINGMINE_INTERVAL + DIVIDINGMINE_RAND / 2 -
            rand () % DIVIDINGMINE_RAND;
    proj->vector.x /= 1.2;
    proj->vector.y /= 1.2;
    /* Divide */
    if (proj->var1 == 1) {
        Projectile *newmine;
        proj->var1 =
            DIVIDINGMINE_INTERVAL + DIVIDINGMINE_RAND / 2 -
            rand () % DIVIDINGMINE_RAND;
        proj->primed = 5;
        newmine = malloc (sizeof (Projectile));
        memcpy (newmine, proj, sizeof (Projectile));
        d = (rand () % 360) / 360.0 * (2.0 * M_PI);
        newmine->vector.x = sin (d) * 1.0;
        newmine->vector.y = cos (d) * 1.0;
        add_projectile (newmine);
    } else if (proj->var1 > 1)
        proj->var1--;
}

void mine_movement (Projectile * proj)
{
    int p;
    double f1;
    p = find_nearest_player (proj->x, proj->y, -1, &f1);
    if (f1 < 40)
        proj->color = col_gray;
    else
        proj->color = 0;
}

void rocket_movement (Projectile * proj)
{
    if (proj->primed == 0)
        make_particle (proj->x, proj->y, 5);
}

void mirv_movement (Projectile * proj)
{
    if (proj->primed == 0)
        make_particle (proj->x, proj->y, 5);
    if (proj->life == 1) {
        Projectile *mirv2;
        int p;
        proj->var1--;
        if (proj->var1 > 0)
            proj->life = MIRV_LIFE;
        else
            proj->life = -1;
        for (p = -1; p < 2; p += 2) {
            mirv2 = make_projectile (proj->x, proj->y, proj->vector);
            mirv2->gravity = proj->gravity;
            mirv2->color = proj->color;
            mirv2->angle = proj->angle - ((double) p) / 4.0;
            mirv2->vector.x = sin (mirv2->angle) * -3.0;
            mirv2->vector.y = cos (mirv2->angle) * -3.0;
            mirv2->type = Mirv;
            mirv2->var1 = proj->var1;
            mirv2->life = proj->life;
            add_projectile (mirv2);
        }
        proj->vector.x = sin (proj->angle) * -3.0;
        proj->vector.y = cos (proj->angle) * -3.0;
    }
}

void spear_movement (Projectile * proj)
{
    proj->angle = atan2 (proj->vector.x, proj->vector.y);
}

void energy_movement (Projectile * proj)
{
    Particle *part;
    float p;
    for (p = 1; p <= 3; p++) {
        part =
            make_particle (Round(proj->x - proj->vector.x / p),
                           Round(proj->y - proj->vector.y / p), 30);
        part->color[0] = 255;
        part->color[1] = 255;
        part->color[2] = 0;
#if !HAVE_LIBSDL_GFX
        part->rd = -8;
        part->gd = -8;
        part->bd = 0;
#else
        part->rd = 0;
        part->gd = 0;
        part->bd = 0;
#endif
        part->vector.x = 0;
        part->vector.y = 0;
    }
}

void magwave_movement (Projectile * proj)
{
    Particle *part;
    double r;
    char c;
    for (r = -M_PI; r < M_PI; r += 0.6) {
        c = rand () % 128;
        part = make_particle (proj->x, proj->y, 4);
        part->color[0] = c;
        part->color[1] = c;
        part->color[2] = 255;
#if !HAVE_LIBSDL_GFX
        part->rd = -c >> 3;
        part->gd = -c >> 3;
        part->bd = -255 >> 3;
#else
        part->rd = 0;
        part->gd = 0;
        part->bd = 0;
        part->ad = -255 >> 3;
#endif
        part->vector.x = sin (r) * 2.25;
        part->vector.y = cos (r) * 2.25;
    }
}

void tag_movement (Projectile * proj)
{
    Particle *part;
    part = make_particle (proj->x, proj->y, 30);
    part->color[0] = 255;
    part->color[1] = 110;
    part->color[2] = 180;
    part->rd = 0;
    part->gd = 0;
    part->bd = 0;
    part->x += (rand () % 8 - 4);
    part->vector.x = (rand () % 4 - 2) / 2.0 - weather_wind_vector;
    part->vector.y = 2;
    proj->var1++;
    if (proj->color == col_red) {
        if (proj->var1 > 4) {
            proj->var1 = 0;
            proj->color = col_white;
        }
    } else {
        if (proj->var1 > 2) {
            proj->var1 = 0;
            proj->color = col_red;
        }
    }
}

void acid_movement (Projectile * proj)
{
    Particle *part;
    struct Ship *hp;
    part = make_particle (proj->x, proj->y, 15);
    part->color[0] = 255;
    part->color[1] = 255;
    part->color[2] = 255;
    part->ad = -17;
    part->rd = 0;
    part->gd = 0;
    part->bd = 0;
    part->vector.x = (rand () % 4 - 2) / 2.0 - weather_wind_vector;
    part->vector.y = 2;
    hp = hit_ship (proj->x, proj->y, proj->owner, 15);
    if (hp) {
        proj->x = hp->x+hp->ship[0]->w/2;
        proj->y = hp->y+hp->ship[0]->h/2;
        ship_damage (hp, proj);
    }
}

void water_movement (Projectile * proj)
{
    Particle *part;
    part = make_particle (proj->x, proj->y, 2);
    part->color[0] = lev_watercolrgb[0];
    part->color[1] = lev_watercolrgb[1];
    part->color[2] = lev_watercolrgb[2];
    part->color[3] = 255;
    part->rd = 0;
    part->gd = 0;
    part->bd = 0;
    part->ad = -128;
    part->vector.x = 0;
    part->vector.y = 0;
}

void ember_movement (Projectile * proj)
{
    int dx, dy, p;
    for (p = 0; p < 2 + proj->var1; p++) {
        dx = 3 - rand () % 6;
        dy = 3 - rand () % 6;
        start_burning (proj->x + dx, proj->y + dy);
    }
    proj->color = burncolor[rand () % FIRE_FRAMES];
    if (proj->life == 1) {
        Projectile *ember2;
        proj->var1--;
        if (proj->var1 == 1)
            proj->life = EMBER_LIFE;
        else
            proj->life = -1;
        proj->vector.y /= 3.0;
        spawn_clusters (proj->x, proj->y, 16, FireStarter);
        for (p = -1; p < 3; p++) {
            ember2 = make_projectile (proj->x, proj->y, proj->vector);
            ember2->gravity = proj->gravity;
            ember2->vector.x = proj->vector.x - p/1.3;
            ember2->vector.y = proj->vector.y;
            ember2->type = Ember;
            ember2->var1 = proj->var1;
            ember2->life = proj->life;
            add_projectile (ember2);
        }
        proj->vector.x += 1.5;
    }
}
void napalm_movement (Projectile * proj)
{
    struct Ship *hp;
    char r;
    start_burning (proj->x, proj->y);
    hp = hit_ship (proj->x, proj->y, proj->owner, 15);
    if (hp) {
        proj->x = hp->x+hp->ship[0]->w/2;
        proj->y = hp->y+hp->ship[0]->h/2;
        ship_damage (hp, proj);
        for (r = 0; r < 3; r++)
            start_burning (proj->x + (5 - rand () % 10),
                           proj->y + (5 - rand () % 10));
    }
}

void boomerang_movement (Projectile * proj) {
    if (proj->var1 <= 10)
        proj->var1++;
    if (proj->var1 > 10) {
        struct dllist *ships;
        /* First check that the ship still exists */
        ships=ship_list;
        while(ships) {
            if(ships->data==proj->owner) break;
            ships=ships->next;
        }
        if(ships) { /* Ship found */
            double f1;
            f1 = atan2 (proj->owner->x - proj->x,
                        proj->owner->y - proj->y);
            if (proj->angle < 0 && f1 > 0) {
                if ((f1 - proj->angle) > (fabs (-M_PI - proj->angle) + M_PI - f1))
                    proj->angle -= 0.3;
                else
                    proj->angle += 0.3;
            } else if (proj->angle > 0 && f1 < 0) {
                if ((proj->angle - f1) > (fabs (-M_PI - f1) + M_PI - proj->angle))
                    proj->angle += 0.3;
                else
                    proj->angle -= 0.3;
            } else if (proj->angle < f1)
                proj->angle += 0.3;
            else if (proj->angle != f1)
                proj->angle -= 0.3;
            if (proj->angle < -M_PI)
                proj->angle = M_PI;
            else if (proj->angle > M_PI)
                proj->angle = -M_PI;
            proj->vector =
                makeVector (sin (proj->angle) * -6.0, cos (proj->angle) * -6.0);
        }
    }
}

void mush_movement (Projectile * proj)
{
    if (fabs (proj->vector.x) > 0.5) {
        if (proj->var1 == 2) {
            add_projectile (make_projectile
                            (proj->x, proj->y, makeVector (0, 0)));
            proj->var1 = 0;
        } else
            proj->var1++;
    }
}

/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : demo.c
 * Description : Eyecandy
 * Author(s)   : Calle Laakkonen
 * Starfield effect is copied from The Demo Effects Collection,
 * written by W.P. van Paassen.
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
#include <stdio.h>
#include "defines.h"    /* FADE_STEPS */
#include "physics.h"    /* GAME_SPEED */
#include "console.h"
#include "demo.h"

#define STARFIELD_COUNT 256
#define FADE_STEPS	255/FADE_STEP

/* Star structure */
typedef struct {
    float x, y;
    short z, speed;
    short px, py;
    Uint8 color;
} Star;

/* Internally used globals */
static Star stars[STARFIELD_COUNT];
static Uint32 starcolors[256];
static SDL_Surface *demo_black;

/* Initialize a single star */
static void init_star (Star * star, short i)
{
    star->x = -10.0 + (20.0 * (rand () / (RAND_MAX + 1.0)));
    star->y = -10.0 + (20.0 * (rand () / (RAND_MAX + 1.0)));

    star->x *= 3072.0;          /*change viewpoint */
    star->y *= 3072.0;

    star->z = i;
    star->speed = 2 + (int) (2.0 * (rand () / (RAND_MAX + 1.0)));
    star->px = 0;
    star->py = 0;

    star->color = i >> 2;       /*the closer to the viewer the brighter */
    star->px = (star->x / star->z) + screen->w / 2;
    star->py = (star->y / star->z) + screen->h / 2;

}

/* Initialize */
void init_demos (void)
{
    int r;
    for (r = 0; r < STARFIELD_COUNT; r++)
        init_star (&stars[r], r + 1);
    for (r = 0; r < 256; r++)
        starcolors[r] = map_rgba(r*3, r*3, r*3, 255);
    demo_black = make_surface(screen,0,0);
    if(demo_black) {
        SDL_SetAlpha (demo_black, SDL_SRCALPHA | SDL_RLEACCEL, FADE_STEP);
    } else {
        fprintf(stderr,"init_demos(): %s\n",SDL_GetError());
    }
}

/* Draw and animate a starfield */
void draw_starfield (void)
{
    int i;
    int tempx, tempy;
    for (i = 0; i < STARFIELD_COUNT; i++) {
        stars[i].z -= stars[i].speed;

        if (stars[i].z <= 0) {
            init_star (stars + i, i + 1);
        }

        /*compute 3D position */
        tempx = (stars[i].x / stars[i].z) + screen->w / 2;
        tempy = (stars[i].y / stars[i].z) + screen->h / 2;
        if (tempx < 0 || tempx > screen->w - 1 || tempy < 0 || tempy > screen->h - 1) {   /* check if a star leaves the screen */
            init_star (stars + i, i + 1);
            continue;
        }
#ifdef HAVE_LIBSDL_GFX
        /* If SDL_gfx is available, draw_line is #defined to be aalineColor.
           Antialiasing doesn't make this look any better so we might as well
           call the plain one here.
         */
        lineColor (screen, tempx, tempy, stars[i].px, stars[i].py,
                   starcolors[stars[i].color]);
#else
        draw_line (screen, tempx, tempy, stars[i].px, stars[i].py,
                   starcolors[stars[i].color]);
#endif
        stars[i].px = tempx;
        stars[i].py = tempy;
    }
}

/* Fade the screen to black */
void fade_to_black (void) {
    Uint32 lasttime,delay;
    int r;
    SDL_Event Event;
    SDL_Surface *tmpsurface;
    tmpsurface = copy_surface(screen);
    if(tmpsurface==NULL || demo_black==NULL) {
        if(!tmpsurface)
            fprintf(stderr,"fade_to_black(): %s\n",SDL_GetError());
        else
            SDL_FreeSurface(tmpsurface);
        SDL_FillRect(screen,NULL,0);
        return;
    }

    for (r = 0; r <= FADE_STEPS; r++) {
        lasttime=SDL_GetTicks();
        while (SDL_PollEvent (&Event)) {
            if (Event.type == SDL_KEYDOWN
                && (Event.key.keysym.sym == SDLK_ESCAPE
                    || Event.key.keysym.sym == SDLK_RETURN) && r < FADE_STEPS)
                r = FADE_STEPS;
            else if (Event.type == SDL_JOYBUTTONDOWN)
                r = FADE_STEPS;
        }
        SDL_BlitSurface (tmpsurface, NULL, screen, NULL);
        SDL_SetAlpha (demo_black, SDL_SRCALPHA|SDL_RLEACCEL, FADE_STEP * r);
        SDL_BlitSurface (demo_black, NULL, screen, NULL);
        SDL_UpdateRect (screen, 0, 0, 0, 0);
        delay=SDL_GetTicks()-lasttime;
        if(delay >= GAME_SPEED)
            delay=0;
        else
            delay=GAME_SPEED-delay;
        SDL_Delay (delay);
    }
    SDL_FreeSurface (tmpsurface);
}

/* Fade from black to surface (Note. program is locked up while fading) */
void fade_from_black (SDL_Surface * surface) {
    SDL_Event Event;
    Uint32 lasttime,delay;
    int r;
    /* Fade */
    for (r = 0; r <= FADE_STEPS; r++) {
        lasttime=SDL_GetTicks();
        while (SDL_PollEvent (&Event)) {
            if (Event.type == SDL_KEYDOWN
                && (Event.key.keysym.sym == SDLK_ESCAPE
                    || Event.key.keysym.sym == SDLK_RETURN) && r < FADE_STEPS)
                r = FADE_STEPS;
            else if (Event.type == SDL_JOYBUTTONDOWN)
                r = FADE_STEPS;
        }
        memset (screen->pixels, 0, screen->pitch * screen->h);
        SDL_SetAlpha (surface, SDL_SRCALPHA|SDL_RLEACCEL, FADE_STEP * r);
        SDL_BlitSurface (surface, NULL, screen, NULL);
        SDL_UpdateRect (screen, 0, 0, 0, 0);
        delay=SDL_GetTicks()-lasttime;
        if(delay >= GAME_SPEED)
            delay=0;
        else
            delay=GAME_SPEED-delay;
        SDL_Delay (delay);
    }
}


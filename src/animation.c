/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : animation.c
 * Description : This module handles all the animation and redraw timings
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
#include "SDL.h"

#include "fs.h"
#include "console.h"
#include "level.h"
#include "player.h"
#include "projectile.h"
#include "animation.h"
#include "particle.h"
#include "special.h"
#include "critter.h"
#include "decor.h"
#include "ship.h"

/* Internally used globals */
static SDL_Rect anim_update_rects[2];
static int anim_rects;
static char anim_gamepaused;
static enum {SCR_UNDEF,SCR_QUARTER,SCR_HALF,SCR_FULL} screen_geometry;
static Uint32 anim_fadescr;     /* Fade dead player screens */

/* Exported globals */
int endgame;

/* Set quarter screens */
static void set_quarter_geom(void)
{
    int p;
    for(p=0;p<4;p++) {
        cam_rects[p].w = screen->w/2;
        cam_rects[p].h = screen->h/2;
        viewport_rects[p].x = (screen->w/2) * (p%2);
        viewport_rects[p].y = (screen->h/2) * (p/2);
    }
    screen_geometry = SCR_QUARTER;
}

/* Set half screens for two players */
static void set_half_geom(int p1,int p2)
{
    cam_rects[p1].w=screen->w;
    cam_rects[p1].h=screen->h/2;
    cam_rects[p2] = cam_rects[p1];
    viewport_rects[p1].x=0;
    viewport_rects[p1].y=0;

    viewport_rects[p2].x=0;
    viewport_rects[p2].y=screen->h/2;

    screen_geometry = SCR_HALF;
}

/* Set single player as fullscreen */
static void set_full_geom(int p) {
    cam_rects[p].w=screen->w;
    cam_rects[p].h=screen->h;
    viewport_rects[p].x=0;
    viewport_rects[p].y=0;
    screen_geometry = SCR_FULL;
}

/* Recalculate player screen geometry */
void recalc_geometry(void)
{
    int oldgeom=screen_geometry;
    if(game_settings.bigscreens==0) {
        /* Always use quarter screens */
        set_quarter_geom();
    } else {
        /* Maximize available screen estate */
        int r,numplayers=0;
        int my_players[4] = {0};

        for (r = 0; r < 4; r++) {
            if (players[r].state==ALIVE) {
                my_players[r] = 1;
                numplayers++;
            }
        }
        if( numplayers==1 ) {
            for(r=0;r<4;r++) {
                if(my_players[r]) {
                    set_full_geom(r);
                    break;
                }
            }
        } else if( numplayers==2 ) {
            int p1=-1,p2=-1;
            for(r=0;r<4;r++) {
                if(my_players[r]) {
                    if(p1==-1) p1=r; else p2=r;
                }
            }
            if(p1<0 || p2<0) {
                printf("Bug! recalc_geometry(): p1==%d, p2==%d\n",p1,p2);
                abort();
            }
            set_half_geom(p1,p2);
        } else {
            set_quarter_geom();
        }
    }

    /* Stars must be recalculated after geometry change */
    if(oldgeom != screen_geometry)
        reinit_stars();
}

/* Get viewport size */
SDL_Rect get_viewport_size(void) {
    SDL_Rect size;
    if(game_settings.bigscreens) {
        int r,num=0;
        for(r=0;r<4;r++)
            if(players[r].state!=INACTIVE) num++;
        if(num==1) {
            size.w = screen->w;
            size.h = screen->h;
        } else if(num==2) {
            size.w = screen->w;
            size.h = screen->h/2;
        } else {
            size.w = screen->w/2;
            size.h = screen->h/2;
        }
    } else {
        size.w = screen->w/2;
        size.h = screen->h/2;
    }

    return size;
}

/* Arrange screen update rectangles for quarter screens */
static void rearrange_quarter(int numplayers, int my_players[4])
{
    /* What should we update on frame redraw? */
    anim_update_rects[0].x = 0;
    anim_update_rects[0].y = 0;
    anim_update_rects[0].w = screen->w/2;
    anim_update_rects[0].h = screen->h/2;
    anim_update_rects[1].x = 0;
    anim_update_rects[1].y = screen->h/2;
    anim_update_rects[1].w = screen->w/2;
    anim_update_rects[1].h = screen->h/2;
    anim_rects = 1;

    if (numplayers == 4) {
        /* 4 players, update whole screen */
        anim_update_rects[0].w = screen->w;
        anim_update_rects[0].h = screen->h;
    } else {
        if(numplayers==2 && my_players[0] && my_players[2]) {
            /* Special case, player 1 & 3 */
            anim_update_rects[0].h = screen->h;
        } else if(numplayers==2 && my_players[1] && my_players[3]) {
            /* Special case, player 2 & 4 */
            anim_update_rects[0].x = screen->w/2;
            anim_update_rects[0].h = screen->h;
        } else {
            /* Players 1 & 2  and 3 & 4*/
            int r;
            for(r=0;r<4;r+=2) {
                if(my_players[r+1]) {
                    if(my_players[r]) {
                        anim_update_rects[r/2].w = screen->w;
                    } else {
                        anim_update_rects[r/2].x += screen->w/2;
                    }
                } else if(my_players[r]==0) {
                    anim_update_rects[r/2].w = 0;
                }
            }
            anim_rects = (anim_update_rects[0].w != 0) + (anim_update_rects[1].w != 0);
            if(anim_update_rects[0].w==0 && anim_update_rects[1].w!=0)
                anim_update_rects[0] = anim_update_rects[1];
        }
    }
}

/* Arrange screen update rectangles for half screens */
static void rearrange_half(int numplayers, int my_players[4])
{
    anim_rects = 1;
    if(numplayers==2) {
        anim_update_rects[0].x = 0;
        anim_update_rects[0].y = 0;
        anim_update_rects[0].w = screen->w;
        anim_update_rects[0].h = screen->h;
    } else {
        int r,deadplr=-1,aliveplr=-1;
        for(r=0;r<4;r++) {
            if(players[r].state == BURIED)
                deadplr = r;
            else if(my_players[r])
                aliveplr = r;
        }
        if(deadplr<0 || aliveplr<0) {
            fprintf(stderr,"%s: BUG: deadplr==%d, aliveplr==%d\n",
                    __func__,deadplr,aliveplr);
        }
        anim_update_rects[0].x = 0;
        anim_update_rects[0].y = (screen->h/2) * (aliveplr>deadplr);
        anim_update_rects[0].w = screen->w;
        anim_update_rects[0].h = screen->h/2;
    }
}

/* Reinitialize animation */
void reinit_animation (void)
{
    anim_gamepaused = 0;
    anim_fadescr = 0;
    endgame = -1;
}

/* Arrange screen update rectangles */
void rearrange_animation (void)
{
    int r, numplayers=0;
    int my_players[4] = {0};
    /* Which players are to be updated */
    for (r = 0; r < 4; r++) {
        if (players[r].state==ALIVE || players[r].state==DEAD) {
            my_players[r] = 1;
            numplayers++;
        }
    }

    if(numplayers==0) {
        anim_rects = 0;
    } else {
        switch(screen_geometry) {
            case SCR_UNDEF:
                fputs("rearrange_animation(): BUG: screen geometry in undefined!\n",stderr);
                abort();
            case SCR_QUARTER:
                rearrange_quarter(numplayers,my_players);
                break;
            case SCR_HALF:
                rearrange_half(numplayers,my_players);
                break;
            case SCR_FULL:
                anim_update_rects[0].x = 0;
                anim_update_rects[0].y = 0;
                anim_update_rects[0].w = screen->w;
                anim_update_rects[0].h = screen->h;
                anim_rects = 1;
                break;
        }
    }
}

/* Fade out a player viewport */
static void fade_plr_screen(int plr,Uint8 opacity)
{
    SDL_Rect msg;
#ifdef HAVE_LIBSDL_GFX
    boxRGBA(screen,viewport_rects[plr].x,viewport_rects[plr].y,
            viewport_rects[plr].x+cam_rects[plr].w,
            viewport_rects[plr].y+cam_rects[plr].h,
            0,0,0,opacity);
#else
    SDL_Rect rect;
    rect.x = viewport_rects[plr].x;
    rect.y = viewport_rects[plr].y;
    rect.w = cam_rects[plr].w;
    rect.h = cam_rects[plr].h*opacity/510;

    SDL_FillRect(screen,&rect,0);
    rect.y+=cam_rects[plr].h-rect.h;
    SDL_FillRect(screen,&rect,0);

    rect.y = viewport_rects[plr].y+rect.h;
    rect.h = cam_rects[plr].h-rect.h*2;
    rect.w = cam_rects[plr].w*opacity/510;

    SDL_FillRect(screen,&rect,0);
    rect.x+=cam_rects[plr].w-rect.w;
    SDL_FillRect(screen,&rect,0);
#endif
    msg.x = viewport_rects[plr].x + cam_rects[plr].w/2 - plr_messages[plr]->w/2;
    msg.y = viewport_rects[plr].y + cam_rects[plr].h/2 - plr_messages[plr]->h/2;
    if(plr_messages[plr])
        SDL_BlitSurface (plr_messages[plr], NULL, screen, &msg);
    else
        printf("Bug! fade_plr_screen(%d,%d): plr_messages[%d] is NULL!\n",plr,opacity,plr);
}

void kill_plr_screen (int plr)
{
    anim_fadescr |= FADE_STEP << (plr*8);
}

int pause_game (void)
{
    int p;
    SDL_Rect rect;
    if (anim_gamepaused) {
        anim_gamepaused = 0;
    } else {
        SDL_Surface *pause_msg;
        SDL_Rect msg_rect;
        if ((game_settings.endmode == 0 && plr_teams_left <= 1)
            || (game_settings.endmode == 1 && plr_teams_left < 1))
            return 1;  /* Dont bother pausing the game, its already over */
        anim_gamepaused = 1;

        /* Render the message string */
        pause_msg = renderstring(Bigfont,"Paused",font_color_red);

        /* Draw pause messages */
        for (p = 0; p < 4; p++)
            if (players[p].state==ALIVE) {
                rect.x = viewport_rects[p].x;
                rect.y = viewport_rects[p].y;
                rect.w = cam_rects[p].w;
                rect.h = cam_rects[p].h;
                fill_box(screen, viewport_rects[p].x, viewport_rects[p].y, viewport_rects[p].w,
                        viewport_rects[p].h, col_pause_backg);
                msg_rect.x = rect.x + rect.w/2 - pause_msg->w/2;
                msg_rect.y = rect.y + rect.h/2 - pause_msg->h/2;
                SDL_BlitSurface(pause_msg,NULL,screen,&msg_rect);
            }
        SDL_UpdateRects (screen, anim_rects, anim_update_rects);
        SDL_FreeSurface(pause_msg);
    }
    return !anim_gamepaused;
}

/**** Redraw frame ****/
void animate_frame (void)
{
    /* Delay after game ends */
    if (endgame > 0)
        endgame--;
    else if (endgame == 0)
        game_loop = 0;

    /* Do animations */
    animate_players ();
    animate_ships ();
    animate_pilots ();
    animate_level ();
    animate_specials ();
    animate_critters ();
    animate_decorations ();
    /* Draw */
    draw_ships ();
    draw_pilots ();
    animate_projectiles ();
    animate_particles ();
    draw_bat_attack ();
    draw_player_hud ();

    /* Fade dead player screens to black */
    if(anim_fadescr) {
        Uint8 fades[4];
        int r;
        for(r=0;r<4;r++) {
            fades[r] = (anim_fadescr >> (r*8)) & 0xff;
            if(fades[r]) {
                fades[r]--;
                fade_plr_screen(r,(FADE_STEP-fades[r])/(double)FADE_STEP*255);
                if(fades[r]==0) {
                    players[r].state = BURIED;
                    rearrange_animation();
                }
            }
        }
        anim_fadescr = fades[0] | (fades[1] << 8) | (fades[2] << 16) | (fades[3] << 24);
    }

    /* Update screen */
    SDL_UpdateRects (screen, anim_rects, anim_update_rects);

    /* End the level if there are less than two teams left
     * and endmode is last player wins or if there are less than 10
     * pixels of base terrain left or all players are dead. */
    if (endgame == -1) {
        if (plr_teams_left < 2) {
            if (game_settings.endmode == 0 || lev_level.base_area < 10
                    || plr_teams_left == 0)
                endgame = 30;
        }
    }
}


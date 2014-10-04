/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2005-2006 Calle Laakkonen
 *
 * File        : selection.c
 * Description : Level/weapon selection screens
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

#include <string.h>
#include <stdlib.h>

#include "defines.h" /* GAME_SPEED */
#include "console.h"
#include "font.h"
#include "list.h"
#include "game.h"
#include "fs.h"
#include "startup.h"
#include "ldat.h"
#include "player.h"
#include "selection.h"
#include "weapon.h"
#include "demo.h"
#include "audio.h"

#define HEADER_HEIGHT 90
#define THUMBNAIL_HEIGHT 120
#define BAR_HEIGHT 200
#define THUMBNAIL_SEP 20
#define WEAPON_SEP 10

struct LevelThumbnail {
    struct LevelFile *file;
    SDL_Surface *name;
    SDL_Surface *thumbnail;
};

static SDL_Surface *trophy_gfx[4];

/* Initialize selection screen */
int init_selection(LDAT *trophies) {
    int r;
    /* Load trophy images */
    for (r = 0; r < 4; r++)
        trophy_gfx[r] = load_image_ldat (trophies, 0, T_ALPHA, "TROPHY", r);
    return 0;
}

/* Draw the header bar common to all selection screens */
static void draw_header(SDL_Surface *surface, const char *title) {
    SDL_Rect rect;
    rect.x = 0; rect.y = 0;
    rect.w = screen->w; rect.h = HEADER_HEIGHT;
    SDL_FillRect(surface,&rect,SDL_MapRGB(surface->format,220,220,220));

    putstring_direct(surface,Bigfont,10,
            HEADER_HEIGHT/2-font_height(Bigfont)/2,title, font_color_blue);

    if(game_status.lastwin > 0) {
        SDL_Rect trophy_rect;
        trophy_rect.x = screen->w - trophy_gfx[game_status.lastwin-1]->w - 20;
        trophy_rect.y = 16;
        SDL_BlitSurface (trophy_gfx[game_status.lastwin - 1], NULL, surface,
                         &trophy_rect);
    }
}

/* Get a list of level thumbnails */
static struct dllist *get_level_thumbnails(void) {
    struct dllist *levels=game_settings.levels;
    struct dllist *thumbnails=NULL;

    while(levels->prev) levels = levels->prev;

    while(levels) {
        struct LevelThumbnail *level = malloc(sizeof(struct LevelThumbnail));

        level->file = levels->data;
        level->name = renderstring(Smallfont,
                level->file->settings->mainblock.name, font_color_cyan);
        level->thumbnail = level->file->settings->thumbnail;

        if(level->thumbnail && level->thumbnail->h != THUMBNAIL_HEIGHT) {
            fprintf(stderr,"Level \"%s\" thumbnail height is not 120 (%d)!\n",
                    level->file->settings->mainblock.name,level->thumbnail->h);
        }

        thumbnails = dllist_append(thumbnails,level);
        levels=levels->next;
    }
    while(thumbnails->prev) thumbnails=thumbnails->prev;
    return thumbnails;
}

/* Free a level thumbnail */
static void free_level_thumbnail(void *data) {
    struct LevelThumbnail *l = data;
    SDL_FreeSurface(l->name);
    free(l);
}

/* Draw a single level thumbnail */
/* x and y are the top left coordinates for the thumbnail. */
/* visibility is the number of horizontal pixels available. */
/* Returns the width of the thumbnail */
static int draw_level(SDL_Surface *surface, struct LevelThumbnail *level,
        int x,int y,int visibility,int selected)
{
    int thwidth;
    SDL_Rect rect;
    Uint8 alpha;
    rect.x = x;
    rect.y = y;
    if(level->thumbnail) {
        SDL_Rect src = {0,0,level->thumbnail->w,level->thumbnail->h};
        thwidth = level->thumbnail->w;
        if(rect.x<0) {
            src.x = -rect.x;
            src.w -= src.x;
            rect.x = 0;
        }
        if(visibility > level->thumbnail->w)
            alpha = SDL_ALPHA_OPAQUE;
        else
            alpha = visibility/(double)level->thumbnail->w*SDL_ALPHA_OPAQUE;
        SDL_SetAlpha(level->thumbnail,SDL_SRCALPHA,alpha);
        SDL_BlitSurface(level->thumbnail,&src,surface,&rect);
    } else {
        rect.w = THUMBNAIL_HEIGHT;
        rect.h = THUMBNAIL_HEIGHT;
        thwidth = rect.w;
        if(visibility > THUMBNAIL_HEIGHT)
            alpha = SDL_ALPHA_OPAQUE;
        else
            alpha = visibility/(double)THUMBNAIL_HEIGHT*SDL_ALPHA_OPAQUE;
        fill_box(surface,rect.x,rect.y,rect.w,rect.h,map_rgba(0,0,0,alpha));
    }
    if(selected) {
        draw_box(rect.x-3, rect.y-3, rect.w+6, rect.h+6, 3, col_cyan);
        rect.y += rect.h + 4;
        rect.x += rect.w/2 - level->name->w/2;
        SDL_BlitSurface(level->name,NULL,surface,&rect);
    }

    return thwidth;
}

/* Return the width of the level thumbnail */
static int level_width(struct LevelThumbnail *level) {
    if(level->thumbnail)
        return level->thumbnail->w;
    else
        return THUMBNAIL_HEIGHT;
}

/* Draw the level bar in the middle of level selection screen */
static void draw_level_bar(SDL_Surface *surface, struct dllist *thumbnails,
        int offset) {
    struct dllist *ptr;
    SDL_Rect bar;
    int x,y;

    bar.x = 0; bar.w=screen->w;
    bar.y = screen->h/2 - BAR_HEIGHT/2; bar.h = BAR_HEIGHT;

    SDL_FillRect(surface,&bar,SDL_MapRGB(surface->format,220,220,220));

    /* Draw the selected level */
    x = bar.x + bar.w/2 - level_width(thumbnails->data) + offset;
    y = bar.y+bar.h/2 - THUMBNAIL_HEIGHT/2;
    x += draw_level(surface,thumbnails->data,x,y,bar.x+bar.w-x,1) + THUMBNAIL_SEP;

    /* Draw levels to the right of the selection */
    ptr = thumbnails->next;
    while(ptr && x<bar.x + bar.w) {
        int visible = bar.x + bar.w - x;
        x += draw_level(surface,ptr->data,x,y,visible,0) + THUMBNAIL_SEP;
        ptr=ptr->next;
    }

    /* Draw the levels to the left of the selection */
    ptr = thumbnails->prev;
    x = bar.x + bar.w/2 - level_width(thumbnails->data) + offset;
    while(ptr) {
        int curwidth = level_width(ptr->data);
        x -= curwidth + THUMBNAIL_SEP;
        if(x+curwidth < bar.x) break;
        draw_level(surface,ptr->data,x,y,x+curwidth-bar.x,0);
        ptr = ptr->prev;
    }
}

/* Return a pointer to a random element */
static struct dllist *randomize_level(struct dllist *levels) {
    struct dllist *ptr = levels;
    int count=0,n;
    while(ptr->next) count++,ptr=ptr->next;
    n = rand() % count;

    for(ptr=levels;n>=0;n--)
        ptr=ptr->next;
    return ptr;
}

/* Level selection screen. Returns the selected level or NULL if cancelled */
struct LevelFile *select_level(int fade) {
    struct dllist *levels;
    struct LevelFile *selection=NULL;
    SDL_Event event;
    int loop=1,animate=0;
    Uint32 lasttime=0;
    char roundstr[32];

    levels = randomize_level(get_level_thumbnails());

    /* Draw the screen */
    sprintf(roundstr,"Round %d of %d",game_status.total_rounds + 1,
                     game_settings.rounds + game_status.total_rounds);
    if(luola_options.mbg_anim && fade) {
        SDL_Surface *tmp = make_surface(screen,0,0);
        draw_header(tmp,roundstr);
        draw_level_bar(tmp,levels,0);
        fade_from_black(tmp);
        SDL_FreeSurface(tmp);
    } else {
        memset(screen->pixels,0,screen->pitch*screen->h);
        draw_header(screen,roundstr);
        draw_level_bar(screen,levels,0);
        SDL_UpdateRect(screen,0,0,0,0);
    }

    /* Level selection event loop */
    while(loop) {
        enum {DO_NOTHING,MOVE_LEFT,MOVE_RIGHT,CHOOSE,CANCEL} cmd = DO_NOTHING;
        if(animate) {
            if(SDL_PollEvent(&event)==0)
                event.type = SDL_NOEVENT;
        } else {
            SDL_WaitEvent(&event);
        }
        /* Handle event */
        switch(event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_F11)
                    screenshot ();
                else if (event.key.keysym.sym == SDLK_LEFT)
                    cmd = MOVE_LEFT;
                else if (event.key.keysym.sym == SDLK_RIGHT)
                    cmd = MOVE_RIGHT;
                else if (event.key.keysym.sym == SDLK_ESCAPE)
                    cmd = CANCEL;
                else if(event.key.keysym.sym == SDLK_RETURN) {
                    if((event.key.keysym.mod & (KMOD_LALT|KMOD_RALT)))
                        toggle_fullscreen();
                    else
                        cmd = CHOOSE;
                }
                break;
            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
                joystick_button (&event.jbutton);
                break;
            case SDL_JOYAXISMOTION:
                joystick_motion (&event.jaxis, 0);
                break;
            case SDL_QUIT:
                exit(0);
            default:
                break;
        }
        /* Handle action */
        switch(cmd) {
            case DO_NOTHING:
                break;
            case MOVE_LEFT:
                if(levels->prev) {
                    if(luola_options.mbg_anim)
                        animate -= level_width(levels->data) + THUMBNAIL_SEP;
                    levels = levels->prev;
                } else cmd = DO_NOTHING;
                break;
            case MOVE_RIGHT:
                if(levels->next) {
                    levels = levels->next;
                    if(luola_options.mbg_anim)
                        animate += level_width(levels->data) + THUMBNAIL_SEP;
                } else cmd = DO_NOTHING;
                break;
            case CHOOSE:
                selection = ((struct LevelThumbnail*)levels->data)->file;
                loop=0;
                break;
            case CANCEL:
                selection = NULL;
                loop=0;
                break;
        }
        if(cmd == MOVE_LEFT || cmd==MOVE_RIGHT || animate) {
            if(animate!=0) {
                int delta=abs(animate)/16+1;
                lasttime = SDL_GetTicks();
                if(animate<0)
                    animate += delta;
                else
                    animate -= delta;
            }
            draw_level_bar(screen,levels,animate);
            SDL_UpdateRect(screen,0,screen->h/2 - BAR_HEIGHT/2,
                    screen->w,BAR_HEIGHT);
        }

        if (animate) {
            Uint32 delay = SDL_GetTicks () - lasttime;
            if (delay >= GAME_SPEED)
                delay = 0;
            else
                delay = GAME_SPEED - delay;
            SDL_Delay (delay);
        }

    }

    dllist_free(levels,free_level_thumbnail);
    return selection;
}

/* Draw a single weapon selection bar */
static SDL_Rect draw_weapon_bar(SDL_Surface *surface,int plr) {
    SDL_Rect rect;
    Uint32 color;
    int i,plrs=0;
    switch(plr) {
        case 0: color = map_rgba(156,0,0,255); break;
        case 1: color = map_rgba(0,0,156,255); break;
        case 2: color = map_rgba(0,156,0,255); break;
        case 3: color = map_rgba(156,156,0,255); break;
        default: color = 0;
    }

    rect.x = 0; rect.w = screen->w/4-WEAPON_SEP;
    rect.h = font_height(Bigfont);

    rect.y = 0;
    for(i=0;i<4;i++) {
        if(players[i].state != INACTIVE) {
            plrs++;
            if(i<plr) rect.y += rect.h + WEAPON_SEP;
        }
    }

    rect.y += screen->h/2 - ((rect.h + WEAPON_SEP) * plrs)/2;

    fill_box(surface,rect.x,rect.y,rect.w,rect.h,color);

    putstring_direct(surface, Bigfont, rect.x + 10, rect.y,
            normal_weapon[players[plr].standardWeapon].name, font_color_white);

    rect.x = rect.w + WEAPON_SEP;
    rect.w = screen->w - rect.x;

    fill_box(surface,rect.x,rect.y,rect.w,rect.h,color);

    putstring_direct(surface, Bigfont, rect.x + 10, rect.y,
            special_weapon[players[plr].specialWeapon].name, font_color_white);

    rect.x = 0;
    rect.w = screen->w;

    return rect;
}

/* Weapon selection. Returns 1 when players have made their choices */
/* or 0 if selection was cancelled. */
int select_weapon(struct LevelFile *level) {

    int i;
    memset(screen->pixels,0,screen->pitch*screen->h);
    draw_header(screen,level->settings->mainblock.name);
    for(i=0;i<4;i++)
        if(players[i].state != INACTIVE)
            draw_weapon_bar(screen,i);

    SDL_UpdateRect(screen,0,0,0,0);

    while(1) {
        SDL_Event event;
        SDL_WaitEvent(&event);

        /* Handle event */
        if(event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_F11)
                screenshot ();
            else if(event.key.keysym.sym == SDLK_RETURN) {
                if((event.key.keysym.mod & (KMOD_LALT|KMOD_RALT)))
                    toggle_fullscreen();
                else
                    return 1;
            } else if(event.key.keysym.sym == SDLK_ESCAPE)
                break;
            else for(i=0;i<4;i++) {
                int update = 0;
                if(players[i].state == INACTIVE) continue;
                if(event.key.keysym.sym ==
                        game_settings.controller[i].keys[2]) {
                    players[i].specialWeapon--;
                    update=1;
                } else if (event.key.keysym.sym ==
                        game_settings.controller[i].keys[3]) {
                    players[i].specialWeapon++;
                    update=1;
                } else if (event.key.keysym.sym ==
                           game_settings.controller[i].keys[1]) {
                    players[i].standardWeapon--;
                    update=1;
                } else if (event.key.keysym.sym ==
                           game_settings.controller[i].keys[0]) {
                    players[i].standardWeapon++;
                    update=1;
                }
                if(update) {
                    SDL_Rect rect;
                    playwave(WAV_BLIP);

                    if ((int)players[i].specialWeapon < 0)
                        players[i].specialWeapon = special_weapon_count() - 1;
                    else if (players[i].specialWeapon == special_weapon_count() )
                        players[i].specialWeapon = 0;
                    if ((int) players[i].standardWeapon < 0)
                        players[i].standardWeapon = normal_weapon_count() - 1;
                    else if ((int) players[i].standardWeapon == normal_weapon_count())
                        players[i].standardWeapon = 0;

                    rect = draw_weapon_bar(screen,i);
                    SDL_UpdateRect(screen,rect.x,rect.y,rect.w,rect.h);
                }
            }
        } else if(event.type == SDL_JOYBUTTONDOWN)
            joystick_button(&event.jbutton);
        else if(event.type == SDL_JOYAXISMOTION)
            joystick_motion(&event.jaxis,1);
        else if(event.type == SDL_QUIT)
            exit(0);
    }
    return 0;
}


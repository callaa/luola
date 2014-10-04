/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : hotseat.c
 * Description : Hotseat game support code
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
#include "defines.h"
#include "startup.h"
#include "console.h"
#include "fs.h"
#include "stringutil.h"
#include "game.h"
#include "hotseat.h"
#include "animation.h"
#include "player.h"
#include "intro.h"
#include "level.h"
#include "levelfile.h"
#include "ship.h"
#include "weapon.h"
#include "special.h"
#include "weather.h"
#include "critter.h"
#include "particle.h"
#include "ldat.h"
#include "audio.h"
#include "font.h"
#include "menu.h"
#include "demo.h"

#define MENU_ITEM_PLAYMODE	100

#define MENU_RVALUE_START	0xa0
#define MENU_RVALUE_CANCEL	0xa1

#define CHANGE_WEA_BOX_W	500
#define CHANGE_WEA_BOX_H	250

#define CURSORPOS 2

/* Internally used globals */
static ToplevelMenu *hotseat_startup_menu;
static SDL_Surface *trophy_gfx[4];
static SDL_Surface *weaponsel_title, *weaponsel_prompt;
static SDL_Surface *weaponsel_weapons[2][4];
static struct LCache {
    SDL_Surface *icon;
    SDL_Surface *name;
    struct LCache *next;
} *weaponsel_levelcache;
static enum {LEVELSEL,WEAPONSEL} hotseat_weaponsel_mode;
static MenuItem *startgame_item;

/* Internally used functions */
static Uint8 hotseat_weaponsel (void);

static void draw_hotseat_startup (void);
static void draw_hotseat_weaponsel (SDL_Surface * surface);

static Uint8 hotseat_startup_eventloop (void);
static Uint8 hotseat_weaponsel_eventloop (int levelcount);

/* Functions called by menu callbacks */
static SDL_Surface *ship_icon (Uint8 mid, MenuItem * item);
static int menu_icon_spacer (int x, int y, Uint8 align, int mid, MenuItem * item);
static Uint8 menu_toggle_player (MenuCommand cmd, int mid, MenuItem * item);
static char *hotseat_label_function (int mid, MenuItem * item);

/* Initialize */
int init_hotseat (void) {
    MenuDrawingOptions dopts;
    LDAT *ldat;
    Menu *m;
    int r;
    int yoffset=100;
    switch(luola_options.videomode) {
        case VID_640:
            yoffset=100;
            break;
        case VID_800:
            yoffset=120;
            break;
        case VID_1024:
            yoffset=200;
            break;
    }

    dopts = make_menu_drawingoptions (0, yoffset, screen->w, screen->h);
    dopts.spacing = MENU_SPACING;
    dopts.left_offset = screen->h/2;
    dopts.selection_color = SDL_MapRGB (screen->format, 0, 128, 255);
    m = create_menu (MENU_ID_HOTSEAT, MNU_DRAWING_OPTIONS, dopts,
                     MNU_ITEM_SEP, 0, MNU_OPT_LABEL, "Who's in?",
                     MNU_OPT_ALIGN, MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                     font_color_green, 0, MNU_ITEM_LABEL, 1, MNU_OPT_LABEL,
                     "Player 1", MNU_OPT_ACTION, menu_toggle_player,
                     MNU_OPT_FICON, MNU_ALIGN_LEFT, menu_icon_spacer,
                     MNU_OPT_ICONF, MNU_ALIGN_LEFT, ship_icon, MNU_OPT_FICON,
                     MNU_ALIGN_LEFT, menu_icon_spacer, MNU_OPT_FICON,
                     MNU_ALIGN_LEFT, draw_input_icon, MNU_OPT_FICON,
                     MNU_ALIGN_RIGHT, menu_icon_spacer, MNU_OPT_FICON,
                     MNU_ALIGN_RIGHT, draw_team_icon, 0, MNU_ITEM_LABEL, 2,
                     MNU_OPT_LABEL, "Player 2", MNU_OPT_ACTION,
                     menu_toggle_player, MNU_OPT_FICON, MNU_ALIGN_LEFT,
                     menu_icon_spacer, MNU_OPT_ICONF, MNU_ALIGN_LEFT,
                     ship_icon, MNU_OPT_FICON, MNU_ALIGN_LEFT,
                     menu_icon_spacer, MNU_OPT_FICON, MNU_ALIGN_LEFT,
                     draw_input_icon, MNU_OPT_FICON, MNU_ALIGN_RIGHT,
                     menu_icon_spacer, MNU_OPT_FICON, MNU_ALIGN_RIGHT,
                     draw_team_icon, 0, MNU_ITEM_LABEL, 3, MNU_OPT_LABEL,
                     "Player 3", MNU_OPT_ACTION, menu_toggle_player,
                     MNU_OPT_FICON, MNU_ALIGN_LEFT, menu_icon_spacer,
                     MNU_OPT_ICONF, MNU_ALIGN_LEFT, ship_icon, MNU_OPT_FICON,
                     MNU_ALIGN_LEFT, menu_icon_spacer, MNU_OPT_FICON,
                     MNU_ALIGN_LEFT, draw_input_icon, MNU_OPT_FICON,
                     MNU_ALIGN_RIGHT, menu_icon_spacer, MNU_OPT_FICON,
                     MNU_ALIGN_RIGHT, draw_team_icon, 0, MNU_ITEM_LABEL, 4,
                     MNU_OPT_LABEL, "Player 4", MNU_OPT_ACTION,
                     menu_toggle_player, MNU_OPT_FICON, MNU_ALIGN_LEFT,
                     menu_icon_spacer, MNU_OPT_ICONF, MNU_ALIGN_LEFT,
                     ship_icon, MNU_OPT_FICON, MNU_ALIGN_LEFT,
                     menu_icon_spacer, MNU_OPT_FICON, MNU_ALIGN_LEFT,
                     draw_input_icon, MNU_OPT_FICON, MNU_ALIGN_RIGHT,
                     menu_icon_spacer, MNU_OPT_FICON, MNU_ALIGN_RIGHT,
                     draw_team_icon, 0, MNU_ITEM_SEP, 0, MNU_OPT_LABEL,
                     "Number of rounds", MNU_OPT_ALIGN, MNU_ALIGN_CENTER,
                     MNU_OPT_COLOR, font_color_green, 0, MNU_ITEM_VALUE, 5,
                     MNU_OPT_LABEL, "%d", MNU_OPT_VALUE,
                     &game_settings.rounds, MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                     MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE, 99, 0,
                     MNU_ITEM_SEP, 6, MNU_OPT_LABEL, "Playmode",
                     MNU_OPT_ALIGN, MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                     font_color_green, 0, MNU_ITEM_VALUE, MENU_ITEM_PLAYMODE,
                     MNU_OPT_LABELF, hotseat_label_function,
                     MNU_OPT_VALUE_TYPE, MNU_TYP_INT, MNU_OPT_VALUE,
                     &game_settings.playmode, MNU_OPT_MINVALUE, 0,
                     MNU_OPT_MAXVALUE, PLAYMODE_COUNT - 1, 0, MNU_ITEM_RETURN,
                     8, MNU_OPT_LABEL, "Go!", MNU_OPT_COLOR,
                     font_color_green, MNU_OPT_RVAL, MENU_RVALUE_START,
                     MNU_OPT_ALIGN, MNU_ALIGN_CENTER, MNU_OPT_DISABLED, 1, 0,
                     MNU_ITEM_RETURN, 9, MNU_OPT_LABEL, "Cancel",
                     MNU_OPT_COLOR, font_color_red, MNU_OPT_RVAL,
                     MENU_RVALUE_CANCEL, MNU_OPT_ALIGN, MNU_ALIGN_CENTER, 0,
                     0);
    hotseat_startup_menu = create_toplevel_menu (1, &m);
    hotseat_startup_menu->escvalue = MENU_RVALUE_CANCEL;
    startgame_item = find_item (m, 8);
    ldat = ldat_open_file (getfullpath (GFX_DIRECTORY, "misc.ldat"));
    if(!ldat) return 1;

    for (r = 0; r < 4; r++)
        trophy_gfx[r] = load_image_ldat (ldat, 0, 1, "TROPHY", r);
    ldat_free (ldat);

    return 0;
}

/* Start a hotseat game */
void hotseat_game (void)
{
    int rval, r;
    LevelBgMusic *bgm;
    reset_game ();
    for (r = 1; r <= 4; r++)
        find_item (hotseat_startup_menu->menu, r)->color = font_color_gray;
    startgame_item->disabled = 1;
    update_menu_cache (hotseat_startup_menu);
    draw_hotseat_startup ();
    rval = hotseat_startup_eventloop ();
    hotseat_startup_menu->current_position = 0; /* Reset the cursor position */
    menu_control (hotseat_startup_menu, MenuDown);
    if (rval != MENU_RVALUE_START)
        return;
    if (game_settings.mbg_anim)
        fade_to_black ();
    game_status.total_rounds = 0;
    while (game_settings.rounds > 0) {
        struct LevelFile *curlevel;
        prematch_game ();
        rval = hotseat_weaponsel ();
        if (rval)
            break;
        curlevel = game_settings.levels->data;
        fill_unused_player_screens ();
        load_level (curlevel);
        apply_per_level_settings(curlevel->settings);
        prepare_specials (curlevel->settings);
        prepare_critters (curlevel->settings);

        clear_weapons ();
        clear_particles ();
        reinit_players ();
        reinit_ships (curlevel->settings);
        prepare_weather ();
        bgm = curlevel->settings->mainblock->music;
        if (bgm) {
            music_newplaylist ();
            while (bgm) {
                music_add_song (bgm->file);
                bgm = bgm->next;
            }
            bgm = (LevelBgMusic *) 1;   /* Just so we know if the playlist was changed for this level */
            music_skip (-1);
        }
        music_play ();
        if(game_settings.playmode==RndWeapon) {
            for(r=0;r<4;r++) {
                if(players[r].state==ALIVE) {
                    set_player_message(r,Smallfont,font_color_green,25,
                            weap2str(players[r].specialWeapon));
                }
            }
        }
        /* Game starts */
        SDL_EnableKeyRepeat(0,0);
        game_eventloop ();
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

        /* Game finished, clean up */
        if (game_settings.mbg_anim)
            fade_to_black ();
        music_stop ();
        if (bgm) {
            music_restoreplaylist ();
            music_skip (0);
            bgm = NULL;
        } else {
            music_skip (1);
        }
        /* Check who won */
        game_status.lastwin = -1;
        for (r = 0; r < 4; r++) {
            if (players[r].state==ALIVE) {
                game_status.wins[r]++;
                game_status.lastwin = r + 1;
            }
        }
        /* Unload level and the game goes on... */
        unload_level ();
        clear_specials ();
        clear_critters ();
        player_cleanup ();
        game_settings.rounds--;
        game_status.total_rounds++;
    }
    if (game_status.total_rounds)
        game_statistics ();
}

/* Internal. */
/* Draw the hotseat startup screen */
static void draw_hotseat_startup (void)
{
    SDL_FillRect (screen, NULL, 0);
    if (game_settings.mbg_anim)
        draw_starfield ();
    draw_menu (screen, hotseat_startup_menu);
    SDL_UpdateRect (screen, 0, 0, 0, 0);
}

/* Internal */
/* Hotseat game startup eventloop */
/* this is where you choose who joins the game and such */
static Uint8 hotseat_startup_eventloop (void)
{
    SDL_Event Event;
    Uint8 needupdate, rval, isevent;
    int command;
    Uint32 delay, lasttime;
    while (1) {
        if (game_settings.mbg_anim) {
            needupdate = 1;
            isevent = SDL_PollEvent (&Event);
        } else {
            needupdate = 0;
            isevent = 1;
            if (!SDL_WaitEvent (&Event)) {
                printf ("Error occured while waiting for an event: %s\n",
                        SDL_GetError ());
                exit (1);
            }
        }
        if (isevent)
            switch (Event.type) {
            case SDL_KEYDOWN:
                command = -1;
                if (Event.key.keysym.sym == SDLK_F11)
                    screenshot ();
                else if (Event.key.keysym.sym == SDLK_UP)
                    command = MenuUp;
                else if (Event.key.keysym.sym == SDLK_DOWN)
                    command = MenuDown;
                else if (Event.key.keysym.sym == SDLK_LEFT)
                    command = MenuLeft;
                else if (Event.key.keysym.sym == SDLK_RIGHT)
                    command = MenuRight;
                else if (Event.key.keysym.sym == SDLK_RETURN) {
                    if((Event.key.keysym.mod & (KMOD_LALT|KMOD_RALT)))
                        toggle_fullscreen();
                    else
                        command = MenuEnter;
                } else if (Event.key.keysym.sym == SDLK_ESCAPE)
                    command = MenuESC;
                if (command >= 0) {
                    if (command != MenuEnter && command != MenuESC
                        && command != MenuToggleLock)
                        playwave (WAV_BLIP);
                    else
                        playwave (WAV_BLIP2);
                    rval = menu_control (hotseat_startup_menu, command);
                    if (rval == MENU_RVALUE_START
                        || rval == MENU_RVALUE_CANCEL)
                        return rval;
                    needupdate = 1;
                }
                break;
            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
                joystick_button (&Event.jbutton);
                break;
            case SDL_JOYAXISMOTION:
                joystick_motion (&Event.jaxis, 0);
                break;
            default:
                break;
            }
        if (!(game_settings.mbg_anim && isevent)) {
            lasttime = SDL_GetTicks ();
            if (needupdate)
                draw_hotseat_startup ();
            if (game_settings.mbg_anim) {
                delay = SDL_GetTicks () - lasttime;
                if (delay >= 60)
                    delay = 0;
                else
                    delay = 60 - delay;
                SDL_Delay (delay);
            }
        }
    }
    /* Never reached */
    return 0;
}

/* Called by a menu callback */
static SDL_Surface *ship_icon (Uint8 mid, MenuItem * item)
{
    Uint8 plr;
    plr = item->ID - 1;
    if (game_settings.players_in[plr] == ' ')
        return ship_gfx[Grey][0];
    else
        return ship_gfx[1 + plr][0];
}

/* Called by a menu callback */
/* A dummy icon */
static int menu_icon_spacer (int x, int y, Uint8 align, int mid, MenuItem * item)
{
    return 10;
}

/* Called by a menu callback */
static Uint8 menu_toggle_player (MenuCommand cmd, int mid, MenuItem * item)
{
    Uint8 plr, newstate;
    plr = item->ID - 1;
    if (cmd == MenuEnter) {
        if (game_settings.players_in[plr] == ' ') {
            game_settings.players_in[plr] = '+';
            newstate = 1;
        } else {
            game_settings.players_in[plr] = ' ';
            newstate = 0;
        }
        item->color = newstate ? font_color_white : font_color_gray;
    } else if (cmd == MenuLeft) {
        player_teams[plr]--;
        if (player_teams[plr] < 0)
            player_teams[plr] = 3;
    } else if (cmd == MenuRight) {
        player_teams[plr]++;
        if (player_teams[plr] > 3)
            player_teams[plr] = 0;
    }
    if (strcmp (game_settings.players_in, "    ") == 0)
        startgame_item->disabled = 1;
    else
        startgame_item->disabled = 0;
    return 1;
}

/* Called by a menu callback */
static char *hotseat_label_function (int mid, MenuItem * item)
{
    if (item->ID == MENU_ITEM_PLAYMODE) {
        switch (game_settings.playmode) {
        case Normal:
            return "Normal";
        case OutsideShip:
            return "Start outside ships";
        case OutsideShip1:
            return "Start outside ships-1";
        case RndCritical:
            return "Random criticals";
        case RndWeapon:
            return "Random weapons";
        }
    }
    return "Foo";
}

/* Weapon selection */
static Uint8 hotseat_weaponsel (void)
{
    SDL_Surface *tmpsurface;
    struct LCache *lc, *prev = NULL;
    struct LevelFile *level;
    struct dllist *lev;
    int r, rval,levelcount=5;
    /* Do some preparations */
    hotseat_weaponsel_mode = LEVELSEL;
    weaponsel_title = NULL;
    weaponsel_prompt = NULL;
    weaponsel_levelcache = NULL;
    for (r = 0; r < 4; r++) {
        weaponsel_weapons[0][r] = NULL;
        weaponsel_weapons[1][r] = NULL;
    }
    lev = game_settings.levels;
    /* Rewind the level pointer back. (levels above the cursor) */
    for (r = 0; r < CURSORPOS; r++) {
        lev = lev->prev;
        if (lev == NULL)
            lev = game_settings.last_level;
    }
    switch(luola_options.videomode) {
        case VID_640: levelcount=5; break;
        case VID_800: levelcount=8; break;
        case VID_1024: levelcount= 12; break;
    }
    /* Make a cache of rendered level names */
    for (r = 0; r < levelcount; r++) {
        level = lev->data;
        lc = malloc (sizeof (struct LCache));
        if (weaponsel_levelcache == NULL)
            weaponsel_levelcache = lc;
        lc->icon = level->settings->icon;
        lc->name = renderstring (Bigfont, level->settings->mainblock->name,
                font_color_white);
        lc->next = NULL;
        if (prev)
            prev->next = lc;
        prev = lc;
        lev = lev->next;
        if (lev == NULL)
            lev = game_settings.first_level;
    }
    /* Draw the level/weapon selection screen and enter eventloop */
    if (game_settings.mbg_anim) {
        tmpsurface =
            SDL_CreateRGBSurface (screen->flags, screen->w, screen->h,
                                  screen->format->BitsPerPixel,
                                  screen->format->Rmask,
                                  screen->format->Gmask,
                                  screen->format->Bmask,
                                  screen->format->Amask);
        draw_hotseat_weaponsel (tmpsurface);
        fade_from_black (tmpsurface);
        SDL_FreeSurface (tmpsurface);
    } else {
        draw_hotseat_weaponsel (screen);
        SDL_UpdateRect (screen, 0, 0, 0, 0);
    }
    rval = hotseat_weaponsel_eventloop (levelcount);
    /* Clean up */
    SDL_FreeSurface (weaponsel_title);
    SDL_FreeSurface (weaponsel_prompt);
    for (r = 0; r < 4; r++) {
        if (weaponsel_weapons[0][r])
            SDL_FreeSurface (weaponsel_weapons[0][r]);
        if (weaponsel_weapons[1][r])
            SDL_FreeSurface (weaponsel_weapons[1][r]);
    }
    while (weaponsel_levelcache) {
        lc = weaponsel_levelcache->next;
        SDL_FreeSurface (weaponsel_levelcache->name);
        free (weaponsel_levelcache);
        weaponsel_levelcache = lc;
    }
    return rval;
}

/* Draw weapon selection screen */
static void draw_hotseat_weaponsel (SDL_Surface * surface)
{
    static SDL_Rect title_rect = { 100, 64, 0, 0 };
    struct LCache *lnames = weaponsel_levelcache;
    SDL_Rect rect, rect2;
    Uint32 cursorcol;
    char str[256];
    Uint32 colors[4];
    int r;
    cursorcol = map_rgba(211,165,255,200);
    colors[0] = map_rgba(200,0,0,200);
    colors[1] = map_rgba(0,0,200,200);
    colors[2] = map_rgba(0,200,0,200);
    colors[3] = map_rgba(200,200,0,200);

    /* Draw common background */
    SDL_FillRect (surface, NULL, 0);
    fill_box(surface,32,0,32,surface->h,map_rgba(255,255,255,220));
    fill_box(surface,0,32,surface->w,64,map_rgba(255,255,255,220));

    /* Level selection mode */
    if (hotseat_weaponsel_mode == LEVELSEL) {
        if (!weaponsel_title) {
            sprintf (str, "Round %d of %d", game_status.total_rounds + 1,
                     game_settings.rounds + game_status.total_rounds);
            weaponsel_title = renderstring (Bigfont, str, font_color_blue);
            title_rect.y = 64 - font_height (Bigfont) / 2;
        }
        if (!weaponsel_prompt) {
            weaponsel_prompt =
                renderstring (Bigfont, "Select the next level",
                              font_color_cyan);
        }
        /* Draw level selector */
        rect.x = 108;
        rect.y = 128;
        rect.h = font_height (Bigfont);
        SDL_BlitSurface (weaponsel_prompt, NULL, surface, &rect);
        rect.y += rect.h;
        r = 0;
        while (lnames) {
            SDL_Rect namerect;
            if (r == CURSORPOS) {
                fill_box(surface,48,rect.y,surface->w-48,rect.h,cursorcol);
            }
            namerect = rect;
            if (lnames->icon) {
                SDL_Rect iconrect = rect;
                iconrect.x -= lnames->icon->w;
                if (iconrect.x < 64) {
                    namerect.x += 64 - iconrect.x;
                    iconrect.x = 64;
                }
                iconrect.y += rect.h / 2 - lnames->icon->h / 2;
                SDL_BlitSurface (lnames->icon, NULL, surface, &iconrect);
            }
            SDL_BlitSurface (lnames->name, NULL, surface, &namerect);
            lnames = lnames->next;
            rect.y += rect.h;
            r++;
        }
    } else {                    /* Draw weapon selection */
        if (!weaponsel_title) {
            struct LevelFile *level = game_settings.levels->data;
            weaponsel_title =
                renderstring (Bigfont,
                        level->settings->mainblock->name, font_color_blue);
            title_rect.y = 64 - font_height (Bigfont) / 2;
        }
        if (!weaponsel_prompt) {
            weaponsel_prompt =
                renderstring (Bigfont, "Select weapons", font_color_cyan);
        }
        rect.x = 96;
        rect.y = 128;
        SDL_BlitSurface (weaponsel_prompt, NULL, surface, &rect);
        rect.y += rect.h;
        rect.x = 48;
        rect.h = 32;
        rect.w = surface->w - rect.x;
        rect2.x = rect.x + 32;
        rect2.w = surface->w - rect2.x;
        rect2.h = rect.h;
        rect2.y = rect.y + rect2.h + 2;
        rect.w -= 32;
        for (r = 0; r < 4; r++) {
            if (game_settings.players_in[r] != ' ') {
                SDL_Rect r1, r2;
                fill_box(surface,rect.x, rect.y, rect.w, rect.h, colors[r]);
                fill_box(surface,rect2.x, rect2.y, rect2.w, rect2.h, colors[r]);
                if (!weaponsel_weapons[0][r])
                    weaponsel_weapons[0][r] =
                        renderstring (Bigfont,
                                      sweap2str (players[r].standardWeapon),
                                      font_color_white);
                if (!weaponsel_weapons[1][r])
                    weaponsel_weapons[1][r] =
                        renderstring (Bigfont,
                                      weap2str (players[r].specialWeapon),
                                      font_color_white);
                r1 = rect;
                r1.x += 32;
                r2 = rect2;
                r2.x += 32;
                SDL_BlitSurface (weaponsel_weapons[0][r], NULL, surface, &r1);
                SDL_BlitSurface (weaponsel_weapons[1][r], NULL, surface, &r2);
                rect.y += 2 * rect.h + 10;
                rect2.y += 2 * rect2.h + 10;
            }
        }
    }
    /* Common. Draw title and last match result */
    SDL_BlitSurface (weaponsel_title, NULL, surface, &title_rect);
    if (game_status.lastwin > 0) {
        SDL_Rect trophy_rect;
        trophy_rect.x = screen->w - trophy_gfx[game_status.lastwin-1]->w - 20;
        trophy_rect.y = 16;
        SDL_BlitSurface (trophy_gfx[game_status.lastwin - 1], NULL, surface,
                         &trophy_rect);
    }
}

/* Weapon/level selection screen eventloop */
static Uint8 hotseat_weaponsel_eventloop (int levelcount)
{
    SDL_Event Event;
    Uint8 p, oldmode, needupdate;
    int cursor;
    while (1) {
        if (!SDL_WaitEvent (&Event)) {
            printf ("Error occured while waiting for an event: %s\n",
                    SDL_GetError ());
            exit (1);
        }
        oldmode = hotseat_weaponsel_mode;
        needupdate = 0;
        switch (Event.type) {
        case SDL_KEYDOWN:
            if (Event.key.keysym.sym == SDLK_F11)
                screenshot ();
            else if (hotseat_weaponsel_mode == LEVELSEL) {
                if (Event.key.keysym.sym == SDLK_ESCAPE)
                    return 1;
                else if (Event.key.keysym.sym == SDLK_RETURN
                         || Event.key.keysym.sym == SDLK_KP_ENTER) {
                    if((Event.key.keysym.mod & (KMOD_LALT|KMOD_RALT))) {
                        toggle_fullscreen();
                    } else {
                        playwave (WAV_BLIP2);
                        if(game_settings.playmode==RndWeapon)
                            return 0;
                        else
                            hotseat_weaponsel_mode=WEAPONSEL;
                    }
                }
                else if (Event.key.keysym.sym == SDLK_LEFT ||
                        Event.key.keysym.sym == SDLK_UP) { /* Level-- */
                    struct LCache *lcache = weaponsel_levelcache;
                    struct LCache *tmpcache;
                    struct dllist *lev;
                    playwave (WAV_BLIP);
                    /* Set level pointer to previous level */
                    if (game_settings.levels->prev == NULL)
                        game_settings.levels = game_settings.last_level;
                    else
                        game_settings.levels = game_settings.levels->prev;
                    lev = game_settings.levels;
                    /* Get the level on the top of the list */
                    for(cursor=CURSORPOS;cursor>0;cursor--) {
                        lev = lev->prev;
                        if (lev == NULL)
                            lev = game_settings.last_level;
                    }
                    /* Update level cache */
                    while (lcache->next->next)
                        lcache = lcache->next;
                    SDL_FreeSurface (lcache->next->name);
                    tmpcache = lcache->next;
                    lcache->next = NULL;
                    lcache = weaponsel_levelcache;
                    weaponsel_levelcache = tmpcache;
                    weaponsel_levelcache->next = lcache;
                    weaponsel_levelcache->icon =
                        ((struct LevelFile*)lev->data)->settings->icon;
                    weaponsel_levelcache->name =
                        renderstring (Bigfont,
                            ((struct LevelFile*)lev->data)->settings->mainblock->name,
                            font_color_white);
                    needupdate = 1;

                } else if (Event.key.keysym.sym == SDLK_RIGHT ||
                        Event.key.keysym.sym == SDLK_DOWN) { /* Level++ */
                    struct LCache *lcache = weaponsel_levelcache->next;
                    struct LCache *tmpcache;
                    struct dllist *lev;
                    playwave (WAV_BLIP);
                    /* Set level pointer to next level */
                    if (game_settings.levels->next == NULL)
                        game_settings.levels = game_settings.first_level;
                    else
                        game_settings.levels = game_settings.levels->next;
                    /* Get the level on the bottom of the list */
                    lev = game_settings.levels;
                    for(cursor=CURSORPOS;cursor<levelcount-1;cursor++) {
                        lev = lev->next;
                        if (lev == NULL)
                            lev = game_settings.first_level;
                    }
                    /* Update level cache */
                    SDL_FreeSurface (weaponsel_levelcache->name);
                    tmpcache = weaponsel_levelcache;
                    weaponsel_levelcache = lcache;
                    while (lcache->next)
                        lcache = lcache->next;
                    lcache->next = tmpcache;
                    lcache = lcache->next;
                    lcache->next = NULL;
                    lcache->icon = ((struct LevelFile*)lev->data)->settings->icon;
                    lcache->name =
                        renderstring (Bigfont,
                                ((struct LevelFile*)lev->data)->settings->mainblock->name,
                                font_color_white);
                    needupdate = 1;
                }
            } else {
                /* Weapon selection mode */
                if (Event.key.keysym.sym == SDLK_ESCAPE)
                    hotseat_weaponsel_mode = LEVELSEL;
                else if (Event.key.keysym.sym == SDLK_RETURN ||
                        Event.key.keysym.sym == SDLK_KP_ENTER) {
                    if((Event.key.keysym.mod & (KMOD_LALT|KMOD_RALT))) {
                        toggle_fullscreen();
                    } else {
                        return 0;
                    }
                }
                for (p = 0; p < 4; p++) {
                    if (game_settings.players_in[p] == ' ') continue;
                    if (Event.key.keysym.sym == game_settings.buttons[p][2]) {
                        players[p].specialWeapon--;
                        SDL_FreeSurface (weaponsel_weapons[1][p]);
                        weaponsel_weapons[1][p] = 0;
                        playwave (WAV_BLIP);
                    } else if (Event.key.keysym.sym ==
                               game_settings.buttons[p][3]) {
                        players[p].specialWeapon++;
                        SDL_FreeSurface (weaponsel_weapons[1][p]);
                        weaponsel_weapons[1][p] = 0;
                        playwave (WAV_BLIP);
                    } else if (Event.key.keysym.sym ==
                               game_settings.buttons[p][1]) {
                        players[p].standardWeapon--;
                        SDL_FreeSurface (weaponsel_weapons[0][p]);
                        weaponsel_weapons[0][p] = 0;
                        playwave (WAV_BLIP);
                    } else if (Event.key.keysym.sym ==
                               game_settings.buttons[p][0]) {
                        players[p].standardWeapon++;
                        SDL_FreeSurface (weaponsel_weapons[0][p]);
                        weaponsel_weapons[0][p] = 0;
                        playwave (WAV_BLIP);
                    }
                    if (players[p].specialWeapon < 1)
                        players[p].specialWeapon = WeaponCount - 1;
                    else if (players[p].specialWeapon == WeaponCount)
                        players[p].specialWeapon = 1;
                    if ((int) players[p].standardWeapon < 0)
                        players[p].standardWeapon = SWeaponCount - 1;
                    else if ((int) players[p].standardWeapon == SWeaponCount)
                        players[p].standardWeapon = 0;
                }
                needupdate = 1;
            }
            break;
        case SDL_JOYBUTTONDOWN:
            joystick_button (&Event.jbutton);
            break;
        case SDL_JOYAXISMOTION:
            joystick_motion (&Event.jaxis, hotseat_weaponsel_mode);
            break;
        default:
            break;
        }
        if (hotseat_weaponsel_mode != oldmode) {
            SDL_FreeSurface (weaponsel_title);
            weaponsel_title = NULL;
            SDL_FreeSurface (weaponsel_prompt);
            weaponsel_prompt = NULL;
            if (hotseat_weaponsel_mode == LEVELSEL) {
                int r;
                for (r = 0; r < 4; r++) {
                    if (weaponsel_weapons[0][r]) {
                        SDL_FreeSurface (weaponsel_weapons[0][r]);
                        weaponsel_weapons[0][r] = NULL;
                    }
                    if (weaponsel_weapons[1][r]) {
                        SDL_FreeSurface (weaponsel_weapons[1][r]);
                        weaponsel_weapons[1][r] = NULL;
                    }
                }
            }
            needupdate = 1;
        }
        if (needupdate) {
            draw_hotseat_weaponsel (screen);
            SDL_UpdateRect (screen, 0, 0, 0, 0);
        }
    }
    return 0;
}


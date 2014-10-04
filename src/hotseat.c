/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
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
#include "startup.h"
#include "console.h"
#include "fs.h"
#include "game.h"
#include "hotseat.h"
#include "selection.h"
#include "animation.h"
#include "player.h"
#include "intro.h"
#include "level.h"
#include "levelfile.h"
#include "ship.h"
#include "weapon.h"
#include "special.h"
#include "decor.h"
#include "critter.h"
#include "particle.h"
#include "ldat.h"
#include "audio.h"
#include "font.h"
#include "menu.h"
#include "demo.h"

#include "number.h"

#define MENU_ITEM_PLAYMODE	100

#define MENU_RVALUE_START	0xa0
#define MENU_RVALUE_CANCEL	0xa1

#define CHANGE_WEA_BOX_W	500
#define CHANGE_WEA_BOX_H	250

#define CURSORPOS 2

/* Internally used globals */
static struct Menu *hotseat_startup_menu;
static struct MenuItem *startgame_item;

/* Return a string containing the current playmode */
static const char *playmode_label(struct MenuItem *item) {
    static const char *modes[PLAYMODE_COUNT] = {
        "Normal",
        "Start outside ships",
        "Start outside ships-1",
        "Random criticals",
        "Random weapons"};
    int mode = *(int*)item->value.value;
    if(mode<0 || mode>=PLAYMODE_COUNT) {
        fprintf(stderr,"Bug: unknown playmode %d\n",mode);
        return "???";
    }
    return modes[mode];
        
}

/* Draw a team flag */
static int draw_team_icon (int x, int y,MenuAlign align,struct MenuItem * item)
{
    Uint32 color;
    int id = item->ID-1;
    int team = get_team(id);
    int px,py;

    if (align == MNU_ALIGN_LEFT)
        x -= 12;
    y += 5;
    if (id > 3) {
        fprintf (stderr,"Bug: No player team flag %d\n", id);
        return 0;
    }
    if (team > 1)
        color = col_black;
    else
        color = col_white;
    for(py=0;py<19;py++) {
        putpixel(screen,x,y+py, col_gray);
        putpixel(screen,x+1,y+py, col_gray);
    }
    for (px = 2; px < 13; px++)
        for (py = px / 2; py < 12 - (px / 2); py++)
            putpixel (screen, x + px, y + py, col_plrs[team]);
    draw_number(screen,x+2,y+2,team+1,color);

    return 14;
}

/* Set player options */
static int menu_toggle_player (MenuCommand cmd,struct MenuItem * item)
{
    int plr = item->ID - 1;

    if (cmd == MNU_ENTER) {
        if (players[plr].state == INACTIVE) {
            players[plr].state = ALIVE;
            item->label.color = font_color_white;
        } else {
            players[plr].state = INACTIVE;
            item->label.color = font_color_gray;
        }
    } else if (cmd == MNU_LEFT) {
        set_team(plr,get_team(plr)-1);
    } else if (cmd == MNU_RIGHT) {
        set_team(plr,get_team(plr)+1);
    } else {
        return 0;
    }
    if (active_players() > 0)
        startgame_item->sensitive = 1;
    else
        startgame_item->sensitive = 0;
    return 1;
}


/* Initialize */
void init_hotseat (void) {
    static const char *players[] = {"Player 1", "Player 2", "Player 3", "Player 4"};
    struct MenuDrawingOptions opts;
    struct MenuText label;
    struct MenuValue val;
    int r;

    /* Set menu options */
    opts.area.x = 0;
    switch(luola_options.videomode) {
        case VID_640: opts.area.y = 100; break;
        case VID_800: opts.area.y = 120; break;
        case VID_1024: opts.area.y = 200; break;
    }
    opts.area.w = screen->w;
    opts.area.h = screen->h-opts.area.h;
    opts.spacing = MENU_SPACING;
    opts.selection_color = map_rgba(0,128,255,255);
    opts.left_offset = screen->h/2;
    opts.right_offset = 0;

    /* Create menu */
    hotseat_startup_menu = create_menu(0,NULL,&opts,"","",MENU_RVALUE_CANCEL);

    label = menu_txt_label("Who's in?");
    label.color = font_color_green;
    label.align = MNU_ALIGN_CENTER;
    add_menu_item(hotseat_startup_menu,MNU_ITEM_SEP,0,label,MnuNullValue);
    for(r=0;r<4;r++) {
        struct MenuText plrlbl = menu_txt_label(players[r]);
        struct MenuIcon *ctrl,*team;
        struct MenuItem *i;
        plrlbl.color = font_color_gray;
        i = add_menu_item(hotseat_startup_menu,MNU_ITEM_LABEL,r+1,
                plrlbl,MnuNullValue);
        i->action = menu_toggle_player;

        ctrl = menu_icon_draw(draw_input_icon);
        ctrl->align = MNU_ALIGN_LEFT;
        team = menu_icon_draw(draw_team_icon);

        i->icons = dllist_append(NULL,ctrl);
        dllist_append(i->icons,team);
    }

    label.txt.text = "Number of rounds";
    add_menu_item(hotseat_startup_menu,MNU_ITEM_SEP,0,label,MnuNullValue);
    val.value = &game_settings.rounds;
    val.min = 1;
    val.max = 99;
    val.inc = 1;
    add_menu_item(hotseat_startup_menu,MNU_ITEM_VALUE,0,
            menu_txt_label("%d"),val);

    label.txt.text = "Playmode";
    add_menu_item(hotseat_startup_menu,MNU_ITEM_SEP,0,label,MnuNullValue);
    val.value = (int*)&game_settings.playmode;
    val.min = 0;
    val.max = PLAYMODE_COUNT - 1;
    add_menu_item(hotseat_startup_menu,MNU_ITEM_VALUE,0,
            menu_func_label(playmode_label),val);

    label.txt.text = "Go!";
    startgame_item = add_menu_item(hotseat_startup_menu,MNU_ITEM_RETURN,
            MENU_RVALUE_START, label,MnuNullValue);
    startgame_item->sensitive = 0;
    label.txt.text = "Cancel";
    label.color = font_color_red;
    add_menu_item(hotseat_startup_menu,MNU_ITEM_RETURN,MENU_RVALUE_CANCEL,
                label,MnuNullValue);
}

/* Draw the hotseat startup screen */
static void draw_hotseat_startup (void)
{
    memset(screen->pixels,0,screen->pitch*screen->h);
    if (game_settings.mbg_anim)
        draw_starfield ();
    draw_menu (screen, hotseat_startup_menu);
    SDL_UpdateRect (screen, 0, 0, 0, 0);
}

/* Hotseat game startup eventloop */
/* this is where you choose who joins the game and such */
static int hotseat_startup_eventloop (void)
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
                fprintf(stderr,"Error occured while waiting for an event: %s\n",
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
                    command = MNU_UP;
                else if (Event.key.keysym.sym == SDLK_DOWN)
                    command = MNU_DOWN;
                else if (Event.key.keysym.sym == SDLK_LEFT)
                    command = MNU_LEFT;
                else if (Event.key.keysym.sym == SDLK_RIGHT)
                    command = MNU_RIGHT;
                else if (Event.key.keysym.sym == SDLK_RETURN) {
                    if((Event.key.keysym.mod & (KMOD_LALT|KMOD_RALT)))
                        toggle_fullscreen();
                    else
                        command = MNU_ENTER;
                } else if (Event.key.keysym.sym == SDLK_ESCAPE)
                    command = MNU_BACK;
                if (command >= 0) {
                    if (command != MNU_ENTER && command != MNU_BACK)
                        playwave (WAV_BLIP);
                    else
                        playwave (WAV_BLIP2);
                    rval = menu_control (&hotseat_startup_menu, command);
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
            case SDL_QUIT:
                exit(0);
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

/* Start a hotseat game */
void hotseat_game (void)
{
    SDL_Rect viewport;
    int rval, r;

    /* Restore game and menu state to default */
    reset_game ();
    flush_menu(hotseat_startup_menu);
    startgame_item->sensitive = 0;
    for (r = 1; r <= 4; r++)
        get_menu_item(hotseat_startup_menu, r)->label.color = font_color_gray;
    hotseat_startup_menu->selection = 1;

    /* Draw hotseat startup screen and enter eventloop */
    draw_hotseat_startup ();
    rval = hotseat_startup_eventloop ();
    if (rval != MENU_RVALUE_START)
        return;
    if (game_settings.mbg_anim)
        fade_to_black ();

    /* Get player viewport size */
    viewport = get_viewport_size();

    /* Open player joypads now */
    open_joypads();

    /* Game started */
    game_status.total_rounds = 0;
    while (game_settings.rounds > 0) {
        struct LevelFile *curlevel;
        int selections_done = 0, need_fade = 1;
        struct dllist *music;

        /* Select level and weapons */
        while(!selections_done) {
            curlevel = select_level(need_fade);
            need_fade = 0;
            if(curlevel==NULL)
                selections_done = -1;
            else
                selections_done = select_weapon(curlevel);
        }
        if(selections_done<0)
            break;

        /* Load selected level */
        fill_player_screens ();
        open_level (curlevel);
        load_level (curlevel);

        /* Check level size */
        if(lev_level.width < viewport.w || lev_level.height < viewport.h) {
            const char *toosmall[] = {
                "Level is smaller than the viewport!",
                "Increase level zoom or use quarter screens.",NULL
            };
            error_screen(curlevel->settings->mainblock.name,
                    "Press enter to skip level", toosmall);
            close_level(curlevel);
            continue;
        }
        /* Prepare for a match */
        apply_per_level_settings(curlevel->settings);
        prepare_specials (curlevel->settings);
        prepare_critters (curlevel->settings);

        clear_projectiles ();
        clear_particles ();
        reset_physics ();
        reinit_players ();
        reinit_ships (curlevel->settings);
        prepare_decorations ();
        
        /* Load custom backgroud music */
        music = curlevel->settings->mainblock.music;
        if (music) {
            struct dllist *ptr = music;
            music_newplaylist ();
            while (ptr) {
                music_add_song (ptr->data);
                ptr = ptr->next;
            }
        }
        /* Start playing the background music (if any) */
        music_play ();
        
        /* Display weapon names to players in random weapon mode */
        if(game_settings.playmode==RndWeapon) {
            for(r=0;r<4;r++) {
                if(players[r].state==ALIVE) {
                    set_player_message(r,Smallfont,font_color_green,25,
                            special_weapon[players[r].specialWeapon].name);
                }
            }
        }

        /* Game starts */
        SDL_EnableKeyRepeat(0,0);
        game_eventloop ();
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

        /* Game finished, clean up */
        music_stop ();
        if (game_settings.mbg_anim)
            fade_to_black ();
        music_wait_stopped();
        if (music) {
            music_restoreplaylist ();
        }
        music_skip(1);

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
        close_level(curlevel);
        clear_specials ();
        game_settings.rounds--;
        game_status.total_rounds++;
    }
    /* Game ended */
    close_joypads();

    /* Display statistics if at least one round was played */
    if (game_status.total_rounds>0)
        game_statistics ();
}


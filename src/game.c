/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
 *
 * File        : game.c
 * Description : Game configuration and initialization
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
#include <stdio.h>
#include <dirent.h>
#include <SDL.h>

#include "startup.h"
#include "console.h"
#include "fs.h"
#include "game.h"
#include "levelfile.h"
#include "player.h"
#include "intro.h"
#include "font.h"
#include "animation.h"
#include "startup.h"
#include "parser.h"
#include "audio.h"

/* Some globals */
static SDL_Surface *gam_filler;
GameInfo game_settings;
PerLevelSettings level_settings;
GameStatus game_status;
int game_loop;

static void load_game_config (void);

/* Set default configuration values and load player screen filler images */
void init_game (LDAT *miscfile)
{
    int p;

    gam_filler = load_image_ldat (miscfile, 1, T_OPAQUE, "FILLER", luola_options.videomode);
    if (gam_filler)
        recolor (gam_filler, 0.4, 0.4, 0.4, 1.0);

    /* Set up the default keys */
    game_settings.controller[0].keys[0] = SDLK_w;
    game_settings.controller[0].keys[1] = SDLK_s;
    game_settings.controller[0].keys[2] = SDLK_a;
    game_settings.controller[0].keys[3] = SDLK_d;
    game_settings.controller[0].keys[4] = SDLK_LSHIFT;
    game_settings.controller[0].keys[5] = SDLK_LCTRL;

    game_settings.controller[1].keys[0] = SDLK_UP;
    game_settings.controller[1].keys[1] = SDLK_DOWN;
    game_settings.controller[1].keys[2] = SDLK_LEFT;
    game_settings.controller[1].keys[3] = SDLK_RIGHT;
    game_settings.controller[1].keys[4] = SDLK_RSHIFT;
    game_settings.controller[1].keys[5] = SDLK_RCTRL;

    game_settings.controller[2].keys[0] = SDLK_i;
    game_settings.controller[2].keys[1] = SDLK_k;
    game_settings.controller[2].keys[2] = SDLK_j;
    game_settings.controller[2].keys[3] = SDLK_l;
    game_settings.controller[2].keys[4] = SDLK_y;
    game_settings.controller[2].keys[5] = SDLK_h;

    game_settings.controller[3].keys[0] = SDLK_KP8;
    game_settings.controller[3].keys[1] = SDLK_KP5;
    game_settings.controller[3].keys[2] = SDLK_KP4;
    game_settings.controller[3].keys[3] = SDLK_KP6;
    game_settings.controller[3].keys[4] = SDLK_KP1;
    game_settings.controller[3].keys[5] = SDLK_KP2;

    for (p = 0; p < 4; p++) {
        game_settings.controller[p].number = 0;
        game_settings.controller[p].device = NULL;
    }
    game_settings.ls.indstr_base = 0;
    game_settings.base_regen = 1;
    game_settings.ship_collisions = 1;
    game_settings.coll_damage = 1;
    game_settings.enable_smoke = 1;
    game_settings.ls.jumpgates = 0;
    game_settings.jumplife = 1;
    game_settings.onewayjp = 1;
    game_settings.soldiers = 15;
    game_settings.helicopters = 5;
    game_settings.ls.turrets = 0;
    game_settings.ls.critters = 1;
    game_settings.ls.cows = 5;
    game_settings.ls.fish = 5;
    game_settings.ls.birds = 5;
    game_settings.ls.bats = 5;
    game_settings.ls.snowfall = 0;
    game_settings.ls.stars = 1;
    game_settings.endmode = 0;
    game_settings.levels = NULL;
    game_settings.large_bullets = 0;
    game_settings.weapon_switch = 0;
    game_settings.eject = 1;
    game_settings.explosions = 1;
    game_settings.recall = 0;
    game_settings.criticals = 0;
    game_settings.bigscreens = 1;

    game_settings.sounds = 0;
    game_settings.music = 0;
    game_settings.playlist = PLS_ORDERED;
    game_settings.sound_vol = 128;
    game_settings.music_vol = 128;

    /* Set the temporary settings */
    game_settings.mbg_anim = luola_options.mbg_anim;

    /* Load the configuration file */
    load_game_config();
}

/* Reset player and game states for a new game */
void reset_game (void)
{
    int p;
    for (p = 0; p < 4; p++) {
        game_status.wins[p] = 0;
        game_status.lifetime[p] = 0;
    }
    reset_players();
    game_settings.rounds = 5;
    game_settings.playmode = Normal;
    game_status.lastwin = 0;
}

/* Override current settings with the ones set in the level */
void apply_per_level_settings (struct LevelSettings * settings)
{
    level_settings = game_settings.ls;
    if (settings->override) {
        struct LSB_Override *o=settings->override;
        if(o->indstr_base>=0) level_settings.indstr_base=o->indstr_base;
        if(o->critters>=0) level_settings.critters=o->critters;
        if(o->stars>=0) level_settings.stars=o->stars;
        if(o->snowfall>=0) level_settings.snowfall=o->snowfall;
        if(o->turrets>=0) level_settings.turrets=o->turrets;
        if(o->jumpgates>=0) level_settings.jumpgates=o->jumpgates;
        if(o->cows>=0) level_settings.cows=o->cows;
        if(o->fish>=0) level_settings.fish=o->fish;
        if(o->birds>=0) level_settings.birds=o->birds;
        if(o->bats>=0) level_settings.bats=o->bats;
    }
}

/*
 * Show game statistics screen
 */
void game_statistics (void)
{
    Uint32 sep_col, team_col, plr_col;
    SDL_Rect rect;
    int t, p, teams[4];
    char tmps[256];
    char data[8][3][32];    /* Four teams and four players */
    char summary[128];
    char data_format[8];    /* Some formatting instructions */
    int data_line, team_data_line;
    long int avglife;
    int mostwins;
    char tiegame;
    int x, y, x2, y2;       /* Variables used when outputting data to screen */
    sep_col = map_rgba(80, 80, 255, 240);
    team_col = map_rgba(10, 10, 80, 240);
    plr_col = map_rgba(0, 0, 50, 160);
    /* Draw background */
    rect.x = screen->w / 2 - 300;
    rect.y = screen->h / 2 - 200;
    rect.w = 600;
    rect.h = 400;
    fill_box(screen, rect.x, rect.y, rect.w, rect.h, map_rgba(0,0,0,191));
    draw_box(rect.x, rect.y, rect.w, rect.h,2, map_rgba(200,80,80,255));

    if (game_status.total_rounds == 1)
        sprintf (tmps, "Game over after one round");
    else
        sprintf (tmps, "Game over after %d rounds",
                 game_status.total_rounds);
    centered_string (screen, Bigfont, rect.y + 10, tmps, font_color_white);
    /* Generate game statistics */
    memset (teams, 0, sizeof (int) * 4);
    data_line = 0;
    for (p = 0; p < 4; p++)
        if (players[p].state)
            teams[get_team(p)] = 1;
    for (t = 0; t < 4; t++) {
        data_format[data_line] = 0;
        sprintf (data[data_line][0], "Team %d", t + 1);
        if (teams[t]) {
            team_data_line = data_line;
            mostwins = 0;
            avglife = 0;
            for (p = 0; p < 4; p++) {
                if (players[p].state && get_team(p) == t) {
                    data_line++;
                    data_format[data_line] = 1;
                    sprintf (data[data_line][0], "Player %d", p + 1);
                    sprintf (data[data_line][1], "%d", game_status.wins[p]);
                    sprintf (data[data_line][2], "%ld seconds",
                             game_status.lifetime[p] / 25 /
                             game_status.total_rounds);
                    if (game_status.wins[p] > mostwins)
                        mostwins = game_status.wins[p];
                    avglife +=
                        game_status.lifetime[p] / 25 /
                        game_status.total_rounds;
                }
            }
            avglife /= (data_line - team_data_line);
            sprintf (data[team_data_line][1], "%d", mostwins);
            sprintf (data[team_data_line][2], "%ld seconds", avglife);
            teams[t] = mostwins;        /* Reuse the 'teams' variable */
        } else {
            data[data_line][1][0] = '\0';
            data[data_line][2][0] = '\0';
            teams[t] = -1;
        }
        data_line++;
    }
    /* Generate summary */
    tiegame = 0;
    mostwins = -1;
    for (t = 0; t < 4; t++) {
        if (teams[t] == mostwins && mostwins > 0) {
            tiegame = 1;
            break;
        }
        if (teams[t] > mostwins)
            mostwins = teams[t];
    }
    if (mostwins == 0 || tiegame) {
        sprintf (summary, "Tie game between teams");
        for (t = 0; t < 4; t++)
            if (teams[t] == mostwins)
                sprintf (summary, "%s %d ", summary, t + 1);
    } else {
        for (t = 0; t < 4; t++)
            if (teams[t] == mostwins) {
                sprintf (summary, "Team %d wins!", t + 1);
                break;
            }
    }
    /* Draw game statistics */
    x = rect.x + 10;
    x2 = rect.x + rect.w - 10;
    y = rect.y + 75;
    y2 = y + 30;
    putstring_direct (screen, Bigfont, rect.x + 200, rect.y + 50, "Wins",
                      font_color_green);
    putstring_direct (screen, Bigfont, rect.x + 300, rect.y + 50,
                      "Average lifetime", font_color_green);
    for (p = 0; p < data_line; p++) {
        if (data_format[p] == 0) {      /* Draw team line */
            SDL_Rect fr;
            fr.x = x;
            fr.y = y;
            fr.w = x2 - x;
            fr.h = y2 - y;
            fill_box(screen,fr.x, fr.y, fr.w, fr.h, team_col);
            draw_line (screen, x, y, x2, y, sep_col);
            putstring_direct (screen, Bigfont, x, y, data[p][0],
                              font_color_white);
        } else {
            SDL_Rect fr;
            fr.x = x;
            fr.y = y;
            fr.w = x2 - x;
            fr.h = y2 - y;
            fill_box(screen,fr.x, fr.y, fr.w, fr.h, plr_col);
            putstring_direct (screen, Bigfont, x + 40, y, data[p][0],
                              font_color_white);
        }
        putstring_direct (screen, Bigfont, rect.x + 200, y, data[p][1],
                          font_color_white);
        putstring_direct (screen, Bigfont, rect.x + 300, y, data[p][2],
                          font_color_white);
        y += 30;
        y2 += 30;
    }
    /* Draw summary line */
    putstring_direct (screen, Bigfont, x, y, summary, font_color_white);
    /* Draw the "press enter to continue" message */
    centered_string (screen, Bigfont, rect.y + rect.h - 30,
                     "Press enter to continue", font_color_white);
    SDL_UpdateRect (screen, rect.x, rect.y, rect.w, rect.h);
    /* Wait for Enter to be pressed */
    wait_for_enter();
}

/* Draw the luola logo on all screen quarters.
 * Active player screens will just draw over it. */
void fill_player_screens (void) {
    if (gam_filler) {
        SDL_Rect rect;
        int p;
        rect.x = (screen->w/2 - gam_filler->w) / 2;
        rect.y = (screen->h/2 - gam_filler->h) / 2;
        for (p = 0; p < 4; p++) {
            SDL_BlitSurface (gam_filler, NULL, screen, &rect);
            rect.x += screen->w/2;
            if (p == 1) {
                rect.y += screen->h/2;
                rect.x -= screen->w;
            }
        }
    } else {
        SDL_FillRect (screen, NULL, 0);
    }
    SDL_UpdateRect (screen, 0, 0, 0, 0);
}

/* Ingame event loop */
void game_eventloop (void) {
    SDL_Event Event;
    Uint32 lasttime = SDL_GetTicks (), delay;
    Uint8 is_not_paused = 1;
    game_loop = 1;
    while (game_loop) {
        while (SDL_PollEvent (&Event)) {
            switch (Event.type) {
            case SDL_KEYDOWN:
                /* Key down event */
                if (Event.key.keysym.sym == SDLK_ESCAPE) return;
                else if (Event.key.keysym.sym == SDLK_F1)
                    radars_visible = !radars_visible;
                else if (Event.key.keysym.sym == SDLK_F5) {
                    game_settings.sound_vol -= 10;
                    if(game_settings.sound_vol<0) game_settings.sound_vol=0;
                    audio_setsndvolume(game_settings.sound_vol);
                    playwave(WAV_BLIP);
                } else if (Event.key.keysym.sym == SDLK_F6) {
                    game_settings.sound_vol += 10;
                    if(game_settings.sound_vol>128) game_settings.sound_vol=128;
                    audio_setsndvolume(game_settings.sound_vol);
                    playwave(WAV_BLIP);
                } else if (Event.key.keysym.sym == SDLK_F7) {
                    game_settings.music_vol -= 10;
                    if(game_settings.music_vol<0) game_settings.music_vol=0;
                    audio_setmusvolume(game_settings.music_vol);
                    playwave(WAV_BLIP2);
                } else if (Event.key.keysym.sym == SDLK_F8) {
                    game_settings.music_vol += 10;
                    if(game_settings.music_vol>128) game_settings.music_vol=128;
                    audio_setmusvolume(game_settings.music_vol);
                    playwave(WAV_BLIP2);
                } else if (Event.key.keysym.sym == SDLK_F11)
                    screenshot ();
                else if (Event.key.keysym.sym == SDLK_PAUSE)
                    is_not_paused = pause_game ();

            case SDL_KEYUP:
                /* Key up event. Fall through from key down */
                if (Event.key.keysym.sym == SDLK_RETURN
                         && Event.type == SDL_KEYUP
                         && (Event.key.keysym.mod & (KMOD_LALT|KMOD_RALT)))
                    toggle_fullscreen();
                else
                    player_keyhandler (&Event.key, Event.type);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                player_joybuttonhandler (&Event.jbutton);
                break;
            case SDL_JOYAXISMOTION:
                player_joyaxishandler (&Event.jaxis);
                break;
            case SDL_QUIT:
                exit(0);
            default:
                break;
            }
        }
        if (is_not_paused) {
            lasttime = SDL_GetTicks ();
            animate_frame ();
            delay = SDL_GetTicks () - lasttime;
            if (delay >= GAME_SPEED)
                delay = 0;
            else
                delay = GAME_SPEED - delay;
            SDL_Delay (delay);
        }
    }
}

/* Save game configuration to file */
void save_game_config (void)
{
    int p, k;
    FILE *fp;
    const char *filename;
    filename = getfullpath (HOME_DIRECTORY, "luola.cfg");
    fp = fopen (filename, "w");
    if (!fp) {
        printf ("Error! Cannot open file \"%s\" for writing\n", filename);
        exit (1);
    }
    /* Write controllers */
    fprintf (fp, "[controllers]\n");
    for (p = 0; p < 4; p++) {
        fprintf (fp, "%d=%d\n", p, game_settings.controller[p].number);
    }
    /* Write keys */
    for (p = 0; p < 4; p++) {
        fprintf (fp, "[keys%d]\n", p + 1);
        for (k = 0; k < 6; k++) {
            fprintf (fp, "%d=%d\n", k, game_settings.controller[p].keys[k]);
        }
    }
    /* Write settings */
    fprintf (fp, "[settings]\n");
    fprintf (fp, "indestructable_base=%d\n", game_settings.ls.indstr_base);
    fprintf (fp, "regenerate_base=%d\n", game_settings.base_regen);
    fprintf (fp, "ship_collisions=%d\n", game_settings.ship_collisions);
    fprintf (fp, "collision_damage=%d\n", game_settings.coll_damage);
    fprintf (fp, "jumpgates=%d\n", game_settings.ls.jumpgates);
    fprintf (fp, "jumplife=%d\n", game_settings.jumplife);
    fprintf (fp, "onewayjp=%d\n", game_settings.onewayjp);
    fprintf (fp, "soldiers=%d\n", game_settings.soldiers);
    fprintf (fp, "helicopters=%d\n", game_settings.helicopters);
    fprintf (fp, "turrets=%d\n", game_settings.ls.turrets);
    fprintf (fp, "critters=%d\n", game_settings.ls.critters);
    fprintf (fp, "cows=%d\n", game_settings.ls.cows);
    fprintf (fp, "birds=%d\n", game_settings.ls.birds);
    fprintf (fp, "fish=%d\n", game_settings.ls.fish);
    fprintf (fp, "bats=%d\n", game_settings.ls.bats);
    fprintf (fp, "smoke=%d\n", game_settings.enable_smoke);
    fprintf (fp, "large_bullets=%d\n", game_settings.large_bullets);
    fprintf (fp, "bigscreens=%d\n", game_settings.bigscreens);
    fprintf (fp, "snowfall=%d\n", game_settings.ls.snowfall);
    fprintf (fp, "stars=%d\n", game_settings.ls.stars);
    fprintf (fp, "endmode=%d\n", game_settings.endmode);
    fprintf (fp, "weapon_switch=%d\n", game_settings.weapon_switch);
    fprintf (fp, "eject=%d\n", game_settings.eject);
    fprintf (fp, "explosions=%d\n", game_settings.explosions);
    fprintf (fp, "recall=%d\n", game_settings.recall);
    fprintf (fp, "criticalhits=%d\n", game_settings.criticals);

    fprintf (fp, "sounds=%d\n", game_settings.sounds);
    fprintf (fp, "music=%d\n", game_settings.music);
    fprintf (fp, "playlist=%d\n", game_settings.playlist);
    fprintf (fp, "sound_volume=%d\n", game_settings.sound_vol);
    fprintf (fp, "music_volume=%d\n", game_settings.music_vol);

    fclose (fp);
}

/* Parse configuration file settings block */
static void parse_settings_block(struct dllist *values) {
    struct Translate tr[] = {
        {"indestructable_base", CFG_INT, &game_settings.ls.indstr_base},
        {"regenerate_base", CFG_INT, &game_settings.base_regen},
        {"ship_collisions", CFG_INT, &game_settings.ship_collisions},
        {"collision_damage", CFG_INT, &game_settings.coll_damage},
        {"jumpgates", CFG_INT, &game_settings.ls.jumpgates},
        {"jumplife", CFG_INT, &game_settings.jumplife},
        {"turrets", CFG_INT, &game_settings.ls.turrets},
        {"critters", CFG_INT, &game_settings.ls.critters},
        {"cows", CFG_INT, &game_settings.ls.cows},
        {"birds", CFG_INT, &game_settings.ls.birds},
        {"fish", CFG_INT, &game_settings.ls.fish},
        {"soldiers", CFG_INT, &game_settings.soldiers},
        {"helicopters", CFG_INT, &game_settings.helicopters},
        {"bats", CFG_INT, &game_settings.ls.bats},
        {"onewayjp", CFG_INT, &game_settings.onewayjp},
        {"smoke", CFG_INT, &game_settings.enable_smoke},
        {"snowfall", CFG_INT, &game_settings.ls.snowfall},
        {"stars", CFG_INT, &game_settings.ls.stars},
        {"endmode", CFG_INT, &game_settings.endmode},
        {"weapon_switch", CFG_INT, &game_settings.weapon_switch},
        {"eject", CFG_INT, &game_settings.eject},
        {"explosions", CFG_INT, &game_settings.explosions},
        {"recall", CFG_INT, &game_settings.recall},
        {"criticalhits", CFG_INT, &game_settings.criticals},
        {"sounds", CFG_INT, &game_settings.sounds},
        {"music", CFG_INT, &game_settings.music},
        {"playlist", CFG_INT, &game_settings.playlist},
        {"music_volume", CFG_INT, &game_settings.music_vol},
        {"sound_volume", CFG_INT, &game_settings.sound_vol},
        {"large_bullets", CFG_INT, &game_settings.large_bullets},
        {"bigscreens", CFG_INT, &game_settings.bigscreens},
        {0,0,0}
    };
    translate_config(values,tr,0);
}

static void parse_controllers_block(struct dllist *values) {
    while(values) {
        struct KeyValue *pair=values->data;
        int plr=atoi(pair->key);
        if(plr<0 || plr>3) {
            printf("No such player (%d)\n",plr);
        } else {
            game_settings.controller[plr].number = atoi (pair->value);
        }
        values=values->next;
    }
}

static void parse_keys_block(struct dllist *values,int player) {
    while(values) {
        struct KeyValue *pair=values->data;
        int key;
        key=atoi(pair->key);
        if(key<0 || key>5)
            printf("No such key (%d)\n",key);
        else
            game_settings.controller[player].keys[key] = atoi (pair->value);
        values=values->next;
    }
}

/* Load game configuration */
static void load_game_config (void) {
    struct dllist *gamecfg,*cfgptr;
    
    gamecfg=read_config_file(getfullpath(HOME_DIRECTORY,"luola.cfg"),0);
    if(!gamecfg) {
        printf("Couldn't load configuration file. Using built in defaults.\n");
        return;
    }
    cfgptr=gamecfg;
    while(cfgptr) {
        struct ConfigBlock *block=cfgptr->data;
        if(block->title==NULL||strcmp(block->title,"settings")==0)
            parse_settings_block(block->values);
        else if(strcmp(block->title,"controllers")==0) parse_controllers_block(block->values);
        else if(strcmp(block->title,"keys1")==0) parse_keys_block(block->values,0);
        else if(strcmp(block->title,"keys2")==0) parse_keys_block(block->values,1);
        else if(strcmp(block->title,"keys3")==0) parse_keys_block(block->values,2);
        else if(strcmp(block->title,"keys4")==0) parse_keys_block(block->values,3);
        else printf("Unknown block \"%s\"\n",block->title);
        cfgptr=cfgptr->next;
    }
    dllist_free(gamecfg,free_config_file);
}


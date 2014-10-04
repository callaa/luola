/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : intro.c
 * Description : Intro and configuration screens
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

#include "SDL.h"
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "console.h"
#include "intro.h"
#include "player.h"
#include "game.h"
#include "fs.h"
#include "stringutil.h"
#include "font.h"
#include "menu.h"
#include "startup.h"
#include "demo.h"

#include "audio.h"

#include "number.h"
#define MENU_ID_TOPLEVEL		0x00
#define MENU_ID_SETTINGS		0x01
#define MENU_ID_INPUT1			0x21
#define MENU_ID_INPUT2			0x22
#define MENU_ID_INPUT3			0x23
#define MENU_ID_INPUT4			0x24
#define MENU_ID_GAME			0x03
#define MENU_ID_WEAPON			0x04
#define MENU_ID_LEVEL			0x05
#define MENU_ID_AUDIO			0x06
#define MENU_ID_CRITTER			0x07
#define MENU_ID_STARTUP			0x08

#define MENU_ITEM_ID_CONTROLLER		0x01
#define MENU_ITEM_ID_THRUST		0x02
#define MENU_ITEM_ID_DOWN		0x03
#define MENU_ITEM_ID_LEFT		0x04
#define MENU_ITEM_ID_RIGHT		0x05
#define MENU_ITEM_ID_FIRE		0x06
#define MENU_ITEM_ID_SPECIAL		0x07
#define MENU_ITEM_ID_JUMPLIFE		0xa0
#define MENU_ITEM_ID_HOLESIZE		0xa1
#define MENU_ITEM_ID_MUSVOL		0xa1
#define MENU_ITEM_ID_SNDVOL		0xa2
#define MENU_ITEM_ID_RETURN		0xFF

/* Internally used globals */
static ToplevelMenu *intro_menu;
static SDL_Surface *intr_logo;
static SDL_Rect intr_logo_rect;

static struct Message {
    Uint8 show;
    Uint32 framecolor, fillcolor;
    int x, y, w, h;
    SDL_Surface *text;
    SDL_Rect textrect;
    SDLKey *setkey;
} intr_message;

/* Exported globals */
SDL_Surface *keyb_icon[4], *pad_icon[4];

/* Internally used functions */
static void intro_message (char *msg);
static Uint8 intro_event_loop (void);
static void draw_intro_screen (void);
static void intro_draw_message (void);

/* Functions called by menu callbacks */
static void draw_logo (int menu_id);
static Uint8 update_sndvolume (MenuCommand cmd, int menu_id, MenuItem * item);
static Uint8 set_player_key (MenuCommand cmd, int menu_id, MenuItem * item);
static Uint8 save_settings (MenuCommand cmd, int menu_id, MenuItem * item);
static Uint8 save_startup_settings (MenuCommand cmd, int menu_id, MenuItem * item);
static char *intro_label_function (int mid, MenuItem * item);


/* Initialize. Load menus and such */
int init_intro (void) {
    Menu *menulist[13];
    MenuDrawingOptions mainmenu, optionsmenu;
    LDAT *datafile;
    int logoy=10;
    int yoffset=250;
    int optionsy=80;
    int p;
    datafile =
        ldat_open_file (getfullpath (GFX_DIRECTORY, "misc.ldat"));
    if(!datafile) return 1;
    /* Load the input device icons */
    for (p = 0; p < 4; p++) {
        keyb_icon[p] = load_image_ldat (datafile, 1, 2, "KEYBOARD", 0);
        pad_icon[p] = load_image_ldat (datafile, 1, 2, "PAD", 0);
    }
    if (keyb_icon[0]) {
        recolor (keyb_icon[0], 0.5, 0.5, 0.6, 1);
        recolor (keyb_icon[1], 0.6, 0.5, 0.5, 1);
        recolor (keyb_icon[2], 0.5, 0.6, 0.5, 1);
        recolor (keyb_icon[3], 0.8, 0.8, 0.5, 1);
    }
    if (pad_icon[0]) {
        recolor (pad_icon[0], 0.5, 0.5, 0.6, 1);
        recolor (pad_icon[1], 0.6, 0.5, 0.5, 1);
        recolor (pad_icon[2], 0.5, 0.6, 0.5, 1);
        recolor (pad_icon[3], 0.8, 0.8, 0.5, 1);
    }
    switch(luola_options.videomode) {
        case VID_640:
            yoffset = 250;
            logoy = 10;
            optionsy = 80;
            break;
        case VID_800:
            yoffset = 280;
            logoy = 40;
            optionsy = 120;
            break;
        case VID_1024:
            yoffset = 380;
            logoy = 90;
            optionsy = 220;
            break;
    }

    /* Menu drawing options */
    mainmenu = make_menu_drawingoptions (0, yoffset, screen->w, screen->h);
    mainmenu.spacing = MENU_SPACING;
    mainmenu.selection_color = SDL_MapRGB (screen->format, 0, 128, 255);
    mainmenu.left_offset = screen->h/2;
    optionsmenu = make_menu_drawingoptions (0, optionsy, screen->w, screen->h);
    optionsmenu.spacing = mainmenu.spacing;
    optionsmenu.selection_color = mainmenu.selection_color;
    optionsmenu.left_offset = 130;
    /* Load graphics */
    intr_logo = load_image_ldat (datafile, 1, 2, "LOGO", 0);
    if (intr_logo) {
        intr_logo_rect.x = screen->w / 2 - intr_logo->w / 2;
        intr_logo_rect.y = logoy;
        centered_string (intr_logo, Smallfont, intr_logo->h - 20, VERSION,
                         font_color_red);
    }
    /* Build the main menu */
    menulist[0] =
        create_menu (MENU_ID_TOPLEVEL, MNU_PREDRAW_FUNC, draw_logo,
                     MNU_DRAWING_OPTIONS, mainmenu, MNU_ITEM_RETURN, 0,
                     MNU_OPT_LABEL, "Start game", MNU_OPT_RVAL,
                     INTRO_RVAL_STARTGAME, 0, MNU_ITEM_SUBMENU, 1,
                     MNU_OPT_LABEL, "Settings", MNU_OPT_VALUE,
                     MENU_ID_SETTINGS, 0, MNU_ITEM_RETURN, 2, MNU_OPT_LABEL,
                     "Exit", MNU_OPT_RVAL, INTRO_RVAL_EXIT, 0, 0);
    /* Build settings menu */
    menulist[1] =
        create_menu (MENU_ID_SETTINGS, MNU_DRAWING_OPTIONS, optionsmenu,
                     MNU_TOGGLE_TEXTS, "enabled", "disabled",
                     MNU_ITEM_SEP, 0, MNU_OPT_LABEL, "Luola configuration",
                     MNU_OPT_ALIGN, MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                     font_color_gray, 0, MNU_ITEM_SEP, 0, MNU_OPT_ALIGN,
                     MNU_ALIGN_CENTER, 0, MNU_ITEM_SUBMENU, MENU_ID_INPUT,
                     MNU_OPT_LABEL, "Input settings", MNU_OPT_VALUE,
                     MENU_ID_INPUT, 0, MNU_ITEM_SUBMENU, MENU_ID_GAME,
                     MNU_OPT_LABEL, "Game settings", MNU_OPT_VALUE,
                     MENU_ID_GAME, 0, MNU_ITEM_SUBMENU, MENU_ID_LEVEL,
                     MNU_OPT_LABEL, "Level settings", MNU_OPT_VALUE,
                     MENU_ID_LEVEL, 0, MNU_ITEM_SUBMENU, MENU_ID_AUDIO,
                     MNU_OPT_LABEL, "Audio settings", MNU_OPT_VALUE,
                     MENU_ID_AUDIO, MNU_OPT_DISABLED, !luola_options.sounds,
                     0, MNU_ITEM_SUBMENU, MENU_ID_AUDIO, MNU_OPT_LABEL,
                     "Startup defaults", MNU_OPT_VALUE, MENU_ID_STARTUP, 0,
                     MNU_ITEM_SEP, 0, MNU_OPT_LABEL, "- - -", 0,
                     MNU_ITEM_LABEL, 1, MNU_OPT_LABEL, "Save settings",
                     MNU_OPT_COLOR, font_color_red, MNU_OPT_ACTION,
                     save_settings, 0, MNU_ITEM_RETURN, MENU_ITEM_ID_RETURN,
                     MNU_OPT_LABEL, "Return", MNU_OPT_COLOR,
                     font_color_green, 0, 0);
    /* Build input settings menu */
    menulist[2] = create_menu (MENU_ID_INPUT,
                               MNU_ITEM_SEP, 0, MNU_OPT_LABEL,
                               "Input settings", MNU_OPT_ALIGN,
                               MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                               font_color_gray, 0, MNU_ITEM_SEP, 0, 0,
                               MNU_ITEM_SUBMENU, MENU_ID_INPUT1,
                               MNU_OPT_LABEL, "Player 1 controller",
                               MNU_OPT_VALUE, MENU_ID_INPUT1, MNU_OPT_FICON,
                               MNU_ALIGN_RIGHT, draw_input_icon, 0,
                               MNU_ITEM_SUBMENU, MENU_ID_INPUT2,
                               MNU_OPT_LABEL, "Player 2 controller",
                               MNU_OPT_VALUE, MENU_ID_INPUT2, MNU_OPT_FICON,
                               MNU_ALIGN_RIGHT, draw_input_icon, 0,
                               MNU_ITEM_SUBMENU, MENU_ID_INPUT3,
                               MNU_OPT_LABEL, "Player 3 controller",
                               MNU_OPT_VALUE, MENU_ID_INPUT3, MNU_OPT_FICON,
                               MNU_ALIGN_RIGHT, draw_input_icon, 0,
                               MNU_ITEM_SUBMENU, MENU_ID_INPUT4,
                               MNU_OPT_LABEL, "Player 4 controller",
                               MNU_OPT_VALUE, MENU_ID_INPUT4, MNU_OPT_FICON,
                               MNU_ALIGN_RIGHT, draw_input_icon, 0,
                               MNU_ITEM_RETURN, MENU_ITEM_ID_RETURN,
                               MNU_OPT_LABEL, "Ok", MNU_OPT_COLOR,
                               font_color_green, 0, 0);
    /* Build game settings menu */
    menulist[3] = create_menu (MENU_ID_GAME,
                               MNU_ITEM_SEP, 0, MNU_OPT_LABEL,
                               "Game settings", MNU_OPT_ALIGN,
                               MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                               font_color_gray, 0, MNU_ITEM_SEP, 0, 0,
                               MNU_ITEM_SUBMENU, MENU_ID_WEAPON,
                               MNU_OPT_LABEL, "Weapon settings",
                               MNU_OPT_VALUE, MENU_ID_WEAPON, 0, MNU_ITEM_SEP,
                               0, MNU_OPT_LABEL, "- - -", 0, MNU_ITEM_TOGGLE,
                               1, MNU_OPT_LABEL, "Ship collisions are %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ship_collisions,
                               0, MNU_ITEM_TOGGLE, 2, MNU_OPT_LABEL,
                               "Collision damage is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.coll_damage, 0,
                               MNU_ITEM_VALUE, MENU_ITEM_ID_JUMPLIFE,
                               MNU_OPT_LABELF, intro_label_function,
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.jumplife,
                               MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE, 2, 0,
                               MNU_ITEM_TOGGLE, 3, MNU_OPT_LABEL,
                               "Smoke is %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.enable_smoke, 0,
                               MNU_ITEM_TOGGLE, 4, MNU_OPT_LABEL,
                               "Endmode: %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.endmode, MNU_OPT_TOGGLE_TEXTS,
                               "last player lands on a base",
                               "one player survives", 0, MNU_ITEM_TOGGLE,
                               5, MNU_OPT_LABEL, "Pilot ejection is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.eject, 0,
                               MNU_ITEM_TOGGLE, 6, MNU_OPT_LABEL,
                               "Ship recall is %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.recall, 0,
                               MNU_ITEM_TOGGLE, 7, MNU_OPT_LABEL,
                               "Player screen size: %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.bigscreens, MNU_OPT_TOGGLE_TEXTS,
                               "maximum", "quarter", 0,
                               MNU_ITEM_RETURN, MENU_ITEM_ID_RETURN,
                               MNU_OPT_LABEL, "Ok", MNU_OPT_COLOR,
                               font_color_green, 0, 0);
    /* Create weapon menu */
    menulist[4] = create_menu (MENU_ID_WEAPON,
                               MNU_ITEM_SEP, 0, MNU_OPT_LABEL,
                               "Weapon settings", MNU_OPT_ALIGN,
                               MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                               font_color_gray, 0, MNU_ITEM_SEP, 0, 0,
                               MNU_ITEM_TOGGLE, 1, MNU_OPT_LABEL,
                               "Projectiles %s affected by gravity",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.gravity_bullets,
                               MNU_OPT_TOGGLE_TEXTS, "are", "are not",
                               0, MNU_ITEM_TOGGLE, 2, MNU_OPT_LABEL,
                               "Projectiles %s affected by wind",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.wind_bullets,
                               MNU_OPT_TOGGLE_TEXTS, "are", "are not",
                               0, MNU_ITEM_TOGGLE, 3, MNU_OPT_LABEL,
                               "Projectiles are drawn %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.large_bullets,
                               MNU_OPT_TOGGLE_TEXTS, "large", "normal",
                               0, MNU_ITEM_TOGGLE, 3, MNU_OPT_LABEL,
                               "Ingame weapon switching is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.weapon_switch, 0,
                               MNU_ITEM_TOGGLE, 4, MNU_OPT_LABEL,
                               "Explosion animation is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.explosions, 0,
                               MNU_ITEM_VALUE, 5, MNU_OPT_LABELF,
                               intro_label_function, MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.holesize, MNU_OPT_MINVALUE, 0,
                               MNU_OPT_MAXVALUE, 3, 0, MNU_ITEM_TOGGLE, 6,
                               MNU_OPT_LABEL, "Critical hits are %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.criticals, 0,
                               MNU_ITEM_RETURN, MENU_ITEM_ID_RETURN,
                               MNU_OPT_LABEL, "Ok", MNU_OPT_COLOR,
                               font_color_green, 0, 0);
    /* Build level settings menu */
    menulist[5] = create_menu (MENU_ID_LEVEL,
                               /* --- */
                               MNU_ITEM_SEP, 0,
                               /* Level settings */
                               MNU_OPT_LABEL, "Level settings",
                               MNU_OPT_ALIGN, MNU_ALIGN_CENTER,
                               MNU_OPT_COLOR, font_color_gray, 0,
                               /* --- */
                               MNU_ITEM_SEP, 0, 0,
                               /* Critters */
                               MNU_ITEM_SUBMENU, MENU_ID_CRITTER,
                               MNU_OPT_LABEL, "Critter settings",
                               MNU_OPT_VALUE, MENU_ID_CRITTER, 0,
                               MNU_ITEM_SEP, 0, MNU_OPT_LABEL, "- - -", 0,
                               /* Base indestruct */
                               MNU_ITEM_TOGGLE, 1, MNU_OPT_LABEL,
                               "Bases are %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.ls.indstr_base,
                               MNU_OPT_TOGGLE_TEXTS, "indestructable",
                               "destructable", 0,
                               /* Base regen */
                               MNU_ITEM_TOGGLE,2,
                               MNU_OPT_LABEL, "Base regeneration is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.base_regen,0,
                               /* Jumpgates */
                               MNU_ITEM_VALUE, 3,
                               MNU_OPT_LABEL, "Jump gates: %d pairs",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ls.jumpgates,
                               MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE, 5, 0,
                               /* Turrets */
                               MNU_ITEM_VALUE, 4, MNU_OPT_LABEL,
                               "Turrets: %d", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.ls.turrets, MNU_OPT_MINVALUE, 0,
                               MNU_OPT_MAXVALUE, 15, 0,
                               /* Snowfall */
                               MNU_ITEM_TOGGLE, 5,
                               MNU_OPT_LABEL, "Snowfall is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ls.snowfall, 0,
                               /* Stars */
                               MNU_ITEM_TOGGLE, 6, MNU_OPT_LABEL,
                               "Stars are %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.ls.stars, 0, MNU_ITEM_RETURN,
                               MENU_ITEM_ID_RETURN, MNU_OPT_LABEL, "Ok",
                               MNU_OPT_COLOR, font_color_green, 0, 0);
    /* Build critter settings menu */
    menulist[6] = create_menu (MENU_ID_CRITTER,
                               MNU_ITEM_SEP, 0, MNU_OPT_LABEL,
                               "Critter settings", MNU_OPT_ALIGN,
                               MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                               font_color_gray, 0, MNU_ITEM_SEP, 0, 0,
                               MNU_ITEM_TOGGLE, 1, MNU_OPT_LABEL,
                               "Critters are %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.ls.critters, 0, MNU_ITEM_VALUE,
                               2, MNU_OPT_LABEL, "Cows: %d",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ls.cows,
                               MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE,
                               MAX_COWS, 0, MNU_ITEM_VALUE, 3, MNU_OPT_LABEL,
                               "Birds: %d", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.ls.birds, MNU_OPT_MINVALUE, 0,
                               MNU_OPT_MAXVALUE, MAX_BIRDS, 0, MNU_ITEM_VALUE,
                               4, MNU_OPT_LABEL, "Fish: %d",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ls.fish,
                               MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE,
                               MAX_FISH, 0, MNU_ITEM_VALUE, 5, MNU_OPT_LABEL,
                               "Bats: %d", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.ls.bats, MNU_OPT_MINVALUE, 0,
                               MNU_OPT_MAXVALUE, MAX_BATS, 0, MNU_ITEM_VALUE,
                               6, MNU_OPT_LABEL, "Infantry per player: %d",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ls.soldiers,
                               MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE,
                               MAX_INFANTRY, 0, MNU_ITEM_VALUE, 7,
                               MNU_OPT_LABEL, "Helicopters per player: %d",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.ls.helicopters,
                               MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE,
                               MAX_HELICOPTER, 0, MNU_ITEM_RETURN,
                               MENU_ITEM_ID_RETURN, MNU_OPT_LABEL, "Ok",
                               MNU_OPT_COLOR, font_color_green, 0, 0);
    /* Build audio menu */
    menulist[7] = create_menu (MENU_ID_AUDIO,
                               MNU_ITEM_SEP, 0,
                               MNU_OPT_LABEL, "Audio settings",
                               MNU_OPT_ALIGN,MNU_ALIGN_CENTER,
                               MNU_OPT_COLOR, font_color_gray, 0,

                               MNU_ITEM_SEP, 0, 0,

                               MNU_ITEM_TOGGLE, 1,
                               MNU_OPT_LABEL,"Sounds are %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.sounds, 0,

                               MNU_ITEM_TOGGLE, 2,
                               MNU_OPT_LABEL, "Music is %s",
                               MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                               MNU_OPT_VALUE, &game_settings.music, 0,

                               MNU_ITEM_TOGGLE, 3, MNU_OPT_LABEL,
                               "Playlist is %s", MNU_OPT_VALUE_TYPE,
                               MNU_TYP_INT, MNU_OPT_VALUE,
                               &game_settings.playlist, MNU_OPT_TOGGLE_TEXTS,
                               "shuffled", "ordered", 0,

                               MNU_ITEM_VALUE, MENU_ITEM_ID_SNDVOL,
                               MNU_OPT_LABELF,intro_label_function,
                               MNU_OPT_VALUE_TYPE,MNU_TYP_INT,
                               MNU_OPT_VALUE,&game_settings.sound_vol,
                               MNU_OPT_MINVALUE, 0,MNU_OPT_MAXVALUE, 128,
                               MNU_OPT_ACTION,update_sndvolume,
                               MNU_OPT_INCREMENT, 10, 0,

                               MNU_ITEM_VALUE, MENU_ITEM_ID_MUSVOL,
                               MNU_OPT_LABELF,intro_label_function,
                               MNU_OPT_VALUE_TYPE,MNU_TYP_INT,
                               MNU_OPT_VALUE,&game_settings.music_vol,
                               MNU_OPT_MINVALUE, 0,MNU_OPT_MAXVALUE, 128,
                               MNU_OPT_INCREMENT, 10, 0,

                               MNU_ITEM_RETURN, MENU_ITEM_ID_RETURN,
                               MNU_OPT_LABEL, "Ok", MNU_OPT_COLOR,
                               font_color_green, 0, 0);
    /* Create player input settings menus */
    for (p = 0; p < 4; p++) {
        menulist[8 + p] = create_menu (MENU_ID_INPUT1 + p,
                                       MNU_ITEM_SEP, 0, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ALIGN,
                                       MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                                       font_color_gray, 0, MNU_ITEM_SEP, 0, 0,
                                       MNU_ITEM_VALUE,
                                       MENU_ITEM_ID_CONTROLLER,
                                       MNU_OPT_LABELF, intro_label_function,
                                       MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                                       MNU_OPT_VALUE,
                                       &game_settings.controller[p],
                                       MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE,
                                       Controllers - 1, 0, MNU_ITEM_LABEL,
                                       MENU_ITEM_ID_THRUST, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ACTION,
                                       set_player_key, 0, MNU_ITEM_LABEL,
                                       MENU_ITEM_ID_DOWN, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ACTION,
                                       set_player_key, 0, MNU_ITEM_LABEL,
                                       MENU_ITEM_ID_LEFT, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ACTION,
                                       set_player_key, 0, MNU_ITEM_LABEL,
                                       MENU_ITEM_ID_RIGHT, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ACTION,
                                       set_player_key, 0, MNU_ITEM_LABEL,
                                       MENU_ITEM_ID_FIRE, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ACTION,
                                       set_player_key, 0, MNU_ITEM_LABEL,
                                       MENU_ITEM_ID_SPECIAL, MNU_OPT_LABELF,
                                       intro_label_function, MNU_OPT_ACTION,
                                       set_player_key, 0, MNU_ITEM_RETURN,
                                       MENU_ITEM_ID_RETURN, MNU_OPT_LABEL,
                                       "Ok", MNU_OPT_COLOR,
                                       font_color_green, 0, 0);
    }
    /* Create startup settings menu */
    menulist[12] = create_menu (MENU_ID_STARTUP,
                                MNU_ITEM_SEP, 0, MNU_OPT_LABEL,
                                "Default startup settings", MNU_OPT_ALIGN,
                                MNU_ALIGN_CENTER, MNU_OPT_COLOR,
                                font_color_gray, 0, MNU_ITEM_SEP, 0, 0,
                                MNU_ITEM_TOGGLE, 1, MNU_OPT_LABEL,
                                "Start in %s mode", MNU_OPT_VALUE_TYPE,
                                MNU_TYP_INT, MNU_OPT_VALUE,
                                &luola_options.fullscreen,
                                MNU_OPT_TOGGLE_TEXTS, "fullscreen",
                                "windowed", 0, MNU_ITEM_TOGGLE, 2,
                                MNU_OPT_LABEL, "Mouse pointer is %s",
                                MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                                MNU_OPT_VALUE, &luola_options.hidemouse,
                                MNU_OPT_TOGGLE_TEXTS, "hidden", "shown",
                                0, MNU_ITEM_TOGGLE, 3, MNU_OPT_LABEL,
                                "Gamepad support is %s",
                                MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                                MNU_OPT_VALUE, &luola_options.joystick, 0,
                                MNU_ITEM_TOGGLE, 3, MNU_OPT_LABEL,
                                "Sound support is %s", MNU_OPT_VALUE_TYPE,
                                MNU_TYP_INT, MNU_OPT_VALUE,
                                &luola_options.sounds, 0,
#if HAVE_LIBSDL_TTF
                                MNU_ITEM_TOGGLE, 4, MNU_OPT_LABEL,
                                "Font engine: %s", MNU_OPT_VALUE_TYPE,
                                MNU_TYP_INT, MNU_OPT_VALUE,
                                &luola_options.sfont, MNU_OPT_TOGGLE_TEXTS,
                                "SFont", "SDL_ttf", 0,
#endif
                                MNU_ITEM_TOGGLE, 5, MNU_OPT_LABEL,
                                "Menu background animations are %s",
                                MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                                MNU_OPT_VALUE, &luola_options.mbg_anim, 0,
                                MNU_ITEM_VALUE, 6,
                                MNU_OPT_LABELF, intro_label_function,
                                MNU_OPT_VALUE_TYPE, MNU_TYP_INT,
                                MNU_OPT_VALUE, &luola_options.videomode,
                                MNU_OPT_MINVALUE, 0, MNU_OPT_MAXVALUE, 2, 0,
                                MNU_ITEM_SEP, 0, MNU_OPT_LABEL, "- - -", 0,
                                MNU_ITEM_LABEL, 10, MNU_OPT_LABEL,
                                "Save startup settings", MNU_OPT_COLOR,
                                font_color_red, MNU_OPT_ACTION,
                                save_startup_settings, 0, MNU_ITEM_RETURN,
                                MENU_ITEM_ID_RETURN, MNU_OPT_LABEL,
                                "Return", MNU_OPT_COLOR, font_color_green,
                                0, 0);
    /* Make the toplevel menu */
    intro_menu = create_toplevel_menu (13, menulist);
    intro_menu->escvalue = INTRO_RVAL_EXIT;
    /* Misc. variable initialization */
    intr_message.show = 0;
    intr_message.text = NULL;
    intr_message.setkey = NULL;
    intr_message.framecolor = map_rgba(200,80,80,220);
    intr_message.fillcolor = map_rgba(0,0,0,220);
    ldat_free (datafile);
    return 0;
}

/* The intro and settings screens */
int game_menu_screen (void) {
    draw_intro_screen ();
    return intro_event_loop ();
}

/* Internal. */
/* Intro screen eventloop */
static Uint8 intro_event_loop (void) {
    Uint8 needupdate, rval, isevent;
    SDL_Event Event;
    Sint8 command;
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
                if (intr_message.show) {
                    intr_message.show = 0;
                    if (intr_message.setkey) {
                        *intr_message.setkey = Event.key.keysym.sym;
                        intr_message.setkey = NULL;
                        return 0;
                    }
                    needupdate = 1;
                } else {
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
                    }
                    else if (Event.key.keysym.sym == SDLK_ESCAPE)
                        command = MenuESC;
                }
                if (command >= 0) {
                    if (command != MenuEnter && command != MenuESC
                        && command != MenuToggleLock)
                        playwave (WAV_BLIP);
                    else
                        playwave (WAV_BLIP2);
                    rval = menu_control (intro_menu, command);
                    if (rval == INTRO_RVAL_STARTGAME
                        || rval == INTRO_RVAL_EXIT)
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
                draw_intro_screen ();
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

/* Internal. */
/* Redraw the intro screen */
void draw_intro_screen (void)
{
    SDL_FillRect (screen, NULL, 0);
    draw_starfield ();
    draw_menu (screen, intro_menu);
    if (intr_message.show)
        intro_draw_message ();
    SDL_UpdateRect (screen, 0, 0, 0, 0);
}

/* Called by a menu callback */
/* Draw the luola logo */
void draw_logo (int menu_id)
{
    if (menu_id != MENU_ID_TOPLEVEL) {
        printf ("Warning! draw_logo(%d) was called from a wrong menu\n",
                (int) menu_id);
        return;
    }
    if (intr_logo)
        SDL_BlitSurface (intr_logo, NULL, screen, &intr_logo_rect);
}

/* Called by a menu callback */
/* Save settings */
Uint8 save_settings (MenuCommand cmd, int menu_id, MenuItem * item)
{
    if (menu_id != MENU_ID_SETTINGS) {
        printf
            ("Warning! save_settings(%d,%d,%x) was not called from %d\n",
             cmd, menu_id, MENU_ID_SETTINGS, (int) item);
        return 0;
    }
    if (cmd != MenuEnter)
        return 0;
    save_game_config ();
    intro_message ("Game settings were saved");
    return 0;
}

/* Called by a menu callback */
/* Save startup settings */
Uint8 save_startup_settings (MenuCommand cmd, int menu_id, MenuItem * item)
{
    if (menu_id != MENU_ID_STARTUP) {
        printf ("save_startup_settings(%d,%d,%x) was not called from %d\n",
                cmd, menu_id, MENU_ID_STARTUP, (int) item);
        return 0;
    }
    if (cmd != MenuEnter)
        return 0;
    save_startup_config ();
    intro_message ("Game startup defaults were saved");
    return 0;
}

/* Called by a menu callback */
/* Return the proper label for a menu item */
char *intro_label_function (int id, MenuItem * item)
{
    static char buf[256] = "???";
    if (id == MENU_ID_GAME) {
        switch (item->ID) {
        case MENU_ITEM_ID_JUMPLIFE:
            sprintf (buf, "Jump-point life: %s",
                     (game_settings.jumplife ==
                      0) ? "short" : (game_settings.jumplife ==
                                         1) ? "medium" : "long");
            break;
        }
    } else if (id == MENU_ID_AUDIO) {
        switch (item->ID) {
            case MENU_ITEM_ID_SNDVOL:
                sprintf(buf, "Effect volume: %d%%",
                        (int) ((game_settings.sound_vol / 128.0) * 100.0));
                break;
            case MENU_ITEM_ID_MUSVOL:
                sprintf (buf, "Music volume: %d%%",
                         (int) ((game_settings.music_vol / 128.0) * 100.0));
                break;
        }
    } else if (id == MENU_ID_WEAPON) {
        char sizestr[32] = "foo";
        switch (game_settings.holesize) {
        case 0:
            strcpy (sizestr, "Tiny");
            break;
        case 1:
            strcpy (sizestr, "Small");
            break;
        case 2:
            strcpy (sizestr, "Normal");
            break;
        case 3:
            strcpy (sizestr, "Big");
            break;
        }
        sprintf (buf, "Explosion hole size: %s", sizestr);
        set_hole_size (game_settings.holesize);
    } else if (id == MENU_ID_STARTUP) {
        char vidstr[32] = "foo";
        switch(luola_options.videomode) {
            case VID_640: strcpy(vidstr,"640x480"); break;
            case VID_800: strcpy(vidstr,"800x600"); break;
            case VID_1024: strcpy(vidstr,"1024x768"); break;
        }
        sprintf (buf, "Video mode: %s", vidstr);
    } else if (id >= MENU_ID_INPUT1 && id <= MENU_ID_INPUT4) {
        Uint8 p;
        p = id - MENU_ID_INPUT1;
        switch (item->ID) {
        case 0:
            sprintf (buf, "Player %d controls", p + 1);
            break;
        case MENU_ITEM_ID_CONTROLLER:
            sprintf (buf, "Controller: %s",
                     controller_name (*(ControllerType *) item->value));
            break;
        case MENU_ITEM_ID_THRUST:
            sprintf (buf, "Thrust - %s",
                     SDL_GetKeyName (game_settings.buttons[p][0]));
            break;
        case MENU_ITEM_ID_DOWN:
            sprintf (buf, "Down - %s",
                     SDL_GetKeyName (game_settings.buttons[p][1]));
            break;
        case MENU_ITEM_ID_LEFT:
            sprintf (buf, "Left - %s",
                     SDL_GetKeyName (game_settings.buttons[p][2]));
            break;
        case MENU_ITEM_ID_RIGHT:
            sprintf (buf, "Right - %s",
                     SDL_GetKeyName (game_settings.buttons[p][3]));
            break;
        case MENU_ITEM_ID_FIRE:
            sprintf (buf, "Fire normal - %s",
                     SDL_GetKeyName (game_settings.buttons[p][4]));
            break;
        case MENU_ITEM_ID_SPECIAL:
            sprintf (buf, "Fire special - %s",
                     SDL_GetKeyName (game_settings.buttons[p][5]));
            break;
        }
    }
    return buf;
}

/* Called by a menu callback (this is exported in intro.h as well) */
/* Draw input icons */
int draw_input_icon (int x, int y, Uint8 align, int mid, MenuItem * item)
{
    Uint8 id;
    SDL_Rect rect;
    id = item->ID;
    rect.x = x;
    rect.y = y;
    if (mid == MENU_ID_INPUT) {
        if (id >= MENU_ID_INPUT1 && id <= MENU_ID_INPUT4)
            id -= MENU_ID_INPUT1;
    } else if (mid == MENU_ID_HOTSEAT) {
        id--;
    }
    if (id > 3) {
        printf ("Warning! Didn't find the icon for menu %d item %d\n", mid,
                id);
        return 0;
    }
    if (game_settings.controller[id] != Keyboard) {
        if (pad_icon[id] == NULL)
            return 0;
        rect.w = pad_icon[id]->w;
        if (align == MNU_ALIGN_LEFT)
            rect.x -= rect.w + 7;
        SDL_BlitSurface (pad_icon[id], NULL, screen, &rect);
        rect.x += rect.w + 2;
        rect.y += rect.h - 5;
        for (x = 0; x < 5; x++)
            for (y = 0; y < 5; y++)
                if (number
                    [LUOLA_MIN
                     (game_settings.controller[id] - 1, NUMBERS - 1)][y][x])
                    putpixel (screen, rect.x + x, rect.y + y, col_red);
    } else {
        if (keyb_icon[id] == NULL)
            return 0;
        rect.w = keyb_icon[id]->w;
        if (align == MNU_ALIGN_LEFT)
            rect.x -= rect.w + 7;
        SDL_BlitSurface (keyb_icon[id], NULL, screen, &rect);
    }
    return rect.w + 7;
}

/* Called by a menu callback (this is exported in intro.h as well) */
/* Draw a team icon */
int draw_team_icon (int x, int y, Uint8 align, int mid, MenuItem * item)
{
    Uint8 id, px, py;
    Uint32 color;
    id = item->ID;
    if (align == MNU_ALIGN_LEFT)
        x -= 12;
    y += 5;
    if (mid == MENU_ID_HOTSEAT) {
        id--;
    }
    if (id > 3) {
        printf ("Warning! Didn't find the icon for menu %d item %d\n", mid,
                id);
        return 0;
    }
    if (player_teams[id] > 1)
        color = col_black;
    else
        color = col_white;
    draw_line (screen, x, y, x, y + 15, col_gray);
    for (px = 1; px < 11; px++)
        for (py = px / 2; py < 10 - (px / 2); py++)
            putpixel (screen, x + px, y + py, col_plrs[player_teams[id]]);
    for (px = 0; px < 5; px++)
        for (py = 0; py < 5; py++) {
            if (number[player_teams[id]][py][px])
                putpixel (screen, x + 1 + px, y + 2 + py, color);
        }
    return 12;
}

/* Called by a menu callback */
/* Set sound volume */
Uint8 update_sndvolume (MenuCommand cmd, int menu_id, MenuItem * item)
{
    audio_setsndvolume(game_settings.sound_vol);
    return 1;
}

/* Called by a menu callback */
/* Ask a new key */
Uint8 set_player_key (MenuCommand cmd, int menu_id, MenuItem * item)
{
    Uint8 p, k;
    if (cmd != MenuEnter)
        return 0;
    p = menu_id - MENU_ID_INPUT1;
    k = item->ID - MENU_ITEM_ID_THRUST;
    intro_message ("Press a key");
    intr_message.setkey = &game_settings.buttons[p][k];
    draw_intro_screen ();
    intro_event_loop ();
    return 1;
}

/* Internal */
/* Create the messagebox */
static void intro_message (char *msg)
{
    intr_message.show = 1;
    /* Make the messagebox */
    if (intr_message.text)
        SDL_FreeSurface (intr_message.text);
    intr_message.text = renderstring (Bigfont, msg, font_color_white);
    intr_message.x = screen->w / 2 - intr_message.text->w / 2 - 25;
    intr_message.y = screen->h / 2 - intr_message.text->h / 2 - 10;
    intr_message.w = intr_message.text->w + 50;
    intr_message.h = intr_message.text->h + 20;
    intr_message.textrect.x =
        intr_message.x + intr_message.w / 2 - intr_message.text->w / 2;
    intr_message.textrect.y =
        intr_message.y + intr_message.h / 2 - font_height (Bigfont) / 2;
}

/* Internal. */
/* Draw the intro messagebox */
static void intro_draw_message (void)
{
    fill_box(screen,intr_message.x+2,intr_message.y+2,intr_message.w-4,intr_message.h-4,intr_message.fillcolor);

    draw_box (intr_message.x, intr_message.y, intr_message.w, intr_message.h,
              2, intr_message.framecolor);
    SDL_BlitSurface (intr_message.text, NULL, screen, &intr_message.textrect);
}

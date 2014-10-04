/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
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

#include "console.h"
#include "intro.h"
#include "player.h"
#include "game.h"
#include "fs.h"
#include "font.h"
#include "menu.h"
#include "startup.h"
#include "demo.h"

#include "audio.h"

#include "number.h"

/* Internally used globals */
static struct Menu *intro_menu;
static SDL_Surface *intr_logo;
static SDL_Surface *keyb_icon, *pad_icon;
static SDL_Rect intr_logo_rect;

static struct Message {
    int show;
    Uint32 framecolor, fillcolor;
    int x, y, w, h;
    SDL_Surface *text;
    SDL_Rect textrect;
    SDLKey *setkey;
} intr_message;

/* Internally used functions */
static int intro_event_loop (void);
static void draw_intro_screen (void);
static void intro_draw_message (void);

/* Draw the luola logo (called by a menu callback) */
static void draw_logo (struct Menu *menu)
{
    if (intr_logo)
        SDL_BlitSurface (intr_logo, NULL, screen, &intr_logo_rect);
}

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

/* Draw the intro messagebox */
static void intro_draw_message (void)
{
    fill_box(screen,intr_message.x+2,intr_message.y+2,intr_message.w-4,
            intr_message.h-4,intr_message.fillcolor);

    draw_box (intr_message.x, intr_message.y, intr_message.w, intr_message.h,
              2, intr_message.framecolor);
    SDL_BlitSurface (intr_message.text, NULL, screen, &intr_message.textrect);
}

/* Add a caption to a menu */
static void add_caption(struct Menu *menu, const char *caption) {
    struct MenuText txt = menu_txt_label(caption);
    txt.color = font_color_gray;
    txt.align = MNU_ALIGN_CENTER;
    add_menu_item(menu,MNU_ITEM_SEP,0,txt,MnuNullValue);
    add_menu_item(menu,MNU_ITEM_SEP,0,menu_txt_label(NULL),MnuNullValue);
}

/* Add an Ok button */
static void add_ok(struct Menu *menu) {
    struct MenuText ok = menu_txt_label("Ok");
    ok.color = font_color_green;
    add_menu_item(menu,MNU_ITEM_PARENT,0,ok,MnuNullValue);
}

/* Return a string containing the player key label */
static const char *player_key_label(struct MenuItem *item) {
    static const char *keys[]={"Thrust","Down","Left","Right","Fire normal", "Fire special"};
    static char str[64];
    int plr = item->parent->ID;
    if(item->ID==1) {
        if(*(int *) item->value.value==0)
            strcpy(str,"Controller: Keyboard");
        else {
            const char *name = SDL_JoystickName(*(int *) item->value.value-1);
            if(name==NULL)
                name = "(Joypad not connected)";
            sprintf(str,"Controller: %s",name);
        }
    } else if(item->ID>1 && item->ID<8) {
        sprintf(str, "%s - %s", keys[item->ID-2],
                SDL_GetKeyName(game_settings.controller[plr].keys[item->ID-2]));
    } else {
        strcpy(str,"???");
    }
    return str;
}

/* Ask a new key */
static int set_player_key (MenuCommand cmd, struct MenuItem * item)
{
    if (cmd != MNU_ENTER)
        return 0;
    intro_message ("Press a key");
    intr_message.setkey = &game_settings.controller[item->parent->ID].keys[item->ID-2];
    draw_intro_screen ();
    intro_event_loop ();
    return 1;
}

/* Create the controller configuration menu */
static struct Menu *make_controller_menu(struct Menu *parent,int plr,
        const char *caption)
{
    struct Menu *m;
    struct MenuText label = menu_func_label(player_key_label);
    struct MenuValue val = {0,0,0,1};
    int r;

    m = create_menu(plr,parent,NULL,NULL,NULL,0);
    add_caption(m,caption);

    val.value = &game_settings.controller[plr].number;
    val.max = SDL_NumJoysticks();
    val.min = 0;
    add_menu_item(m,MNU_ITEM_VALUE,1,label,val);
    for(r=2;r<8;r++)
        add_menu_item(m,MNU_ITEM_LABEL,r,label,MnuNullValue)->action = set_player_key;

    add_ok(m);
    return m;
}

/* Create the input settings menu */
static struct Menu *make_input_menu(struct Menu *parent) {
    static const char *names[]={"Player 1 controller","Player 2 controller",
        "Player 3 controller", "Player 4 controller"};
    struct Menu *m;
    int r;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Input settings");

    for(r=0;r<4;r++) {
        struct MenuValue sub;
        struct MenuItem *item;
        struct MenuIcon *icon;
        sub.value = (int*)make_controller_menu(m,r,names[r]);
        item=add_menu_item(m,MNU_ITEM_SUBMENU,r+1,menu_txt_label(names[r]),sub);
        icon = menu_icon_draw(draw_input_icon);
        item->icons = dllist_append(NULL,icon);
    }

    add_ok(m);

    return m;

}

/* Create the weapon settings menu */
static struct Menu *make_weapon_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuValue val = {NULL,0,0,1};
    struct MenuItem *item;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Weapon settings");

    val.value = &game_settings.large_bullets;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,3,
            menu_txt_label("Projectiles are drawn %s"),val);
    item->text_enabled="large"; item->text_disabled="normal";

    val.value = &game_settings.weapon_switch;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,4,
            menu_txt_label("Ingame weapon switching is %s"),val);

    val.value = &game_settings.explosions;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,5,
            menu_txt_label("Explosion animation is %s"),val);

    val.value = &game_settings.criticals;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,7,
            menu_txt_label("Critical hits are %s"),val);

    val.value = &game_settings.soldiers;
    val.max = 50;
    item = add_menu_item(m,MNU_ITEM_VALUE,8,
            menu_txt_label("Max infantry per player: %d"),val);

    val.value = &game_settings.helicopters;
    item = add_menu_item(m,MNU_ITEM_VALUE,9,
            menu_txt_label("Max helicopters per player: %d"),val);

    add_ok(m);

    return m;
}

/* Create the ship settings menu */
static struct Menu *make_ship_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuValue val;
    struct MenuItem *item;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Ship settings");

    val.value = &game_settings.ship_collisions;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Ship collisions are %s"),val);

    val.value = &game_settings.coll_damage;
    add_menu_item(m,MNU_ITEM_TOGGLE,2,
            menu_txt_label("Collision damage is %s"),val);

    val.value = &game_settings.eject;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,3,
            menu_txt_label("Pilot ejection is %s"),val);

    val.value = &game_settings.recall;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,4,
            menu_txt_label("Ship recall is %s"),val);

    add_ok(m);

    return m;

}

/* Menu callback to get the label for jumppoint duration option */
static const char *jumplife_label(struct MenuItem *item) {
    switch(*(Jumplife*)item->value.value) {
        case JLIFE_SHORT: return "Jump-point life: short";
        case JLIFE_MEDIUM: return "Jump-point life: medium";
        case JLIFE_LONG: return "Jump-point life: long";
    }
    return "Jump-point life: ???";
}

/* Create the game settings menu */
static struct Menu *make_game_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuValue val = {NULL,0,2,1};
    struct MenuItem *item;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Game settings");

    add_submenu(m,1,"Weapon settings",make_weapon_menu(m));
    add_submenu(m,1,"Ship settings",make_ship_menu(m));
    add_menu_item(m,0,MNU_ITEM_SEP,menu_txt_label("- - -"),MnuNullValue);

    val.value = (int*)&game_settings.jumplife;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_func_label(jumplife_label),val);

    val.value = &game_settings.onewayjp;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,2,
            menu_txt_label("Jump points are %s-way"),val);
    item->text_enabled="one";
    item->text_disabled="two";

    val.value = &game_settings.enable_smoke;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,3,
            menu_txt_label("Smoke is %s"),val);

    val.value = &game_settings.endmode;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,4,
            menu_txt_label("Endmode: %s"),val);
    item->text_enabled="last player lands on a base";
    item->text_disabled = "one player survives";

    val.value = &game_settings.bigscreens;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,5,
            menu_txt_label("Player screen size: %s"),val);
    item->text_enabled="maximum";
    item->text_disabled = "quarter";

    add_ok(m);

    return m;
}

/* Create the critter settings menu */
static struct Menu *make_critter_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuValue val = {NULL,0,0,1};
    struct MenuItem *item;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Critter settings");

    val.value = &game_settings.ls.critters;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Random critters are %s"),val);

    val.max = 50;
    val.value = &game_settings.ls.cows;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_txt_label("Cows: %d"),val);

    val.value = &game_settings.ls.birds;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_txt_label("Birds: %d"),val);

    val.value = &game_settings.ls.fish;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_txt_label("Fish: %d"),val);

    val.value = &game_settings.ls.bats;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_txt_label("Bats: %d"),val);

    add_ok(m);

    return m;

}

/* Create the level settings menu */
static struct Menu *make_level_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuValue val;
    struct MenuItem *item;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Level settings");

    add_submenu(m,1,"Critter settings",make_critter_menu(m));
    add_menu_item(m,0,MNU_ITEM_SEP,menu_txt_label("- - -"),MnuNullValue);

    val.value = &game_settings.ls.indstr_base;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Bases are %s"),val);
    item->text_enabled = "indestructable";
    item->text_disabled = "destructable";

    val.value = &game_settings.base_regen;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Base regeneration is %s"),val);

    val.max = 5;
    val.min = 0;
    val.inc = 1;
    val.value = &game_settings.ls.jumpgates;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_txt_label("Jump gates: %d pairs"),val);

    val.max = 15;
    val.value = &game_settings.ls.turrets;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_txt_label("Turrets: %d"),val);


    val.value = &game_settings.ls.snowfall;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,2,
            menu_txt_label("Snowfall is %s"),val);

    val.value = &game_settings.ls.stars;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,2,
            menu_txt_label("Stars are %s"),val);

    add_ok(m);

    return m;
}

/* Sound/music volume menu item label */
static const char *volume_label(struct MenuItem *item) {
    static char buf[22];
    if(item->ID==0) {
        sprintf(buf,"Effect volume: %d%%%%",
                (int)((game_settings.sound_vol / 128.0) * 100.0));
    } else {
        sprintf(buf,"Music volume: %d%%%%",
                (int)((game_settings.music_vol / 128.0) * 100.0));
    }
    return buf;
}

/* Set sound volume callback */
int update_sndvolume (MenuCommand cmd, struct MenuItem * item) {
    audio_setsndvolume(game_settings.sound_vol);
    return 0;
}

/* Create the audio settings menu */
static struct Menu *make_audio_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuValue val;
    struct MenuItem *item;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Audio settings");

    val.value = &game_settings.sounds;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Sound effects are %s"),val);

    val.value = &game_settings.music;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Music is %s"),val);

    val.value = (int*)&game_settings.playlist;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Playlist is %s"),val);
    item->text_enabled="random"; item->text_disabled="ordered";

    val.max = 128;
    val.min = 0;
    val.inc = 10;
    val.value = &game_settings.sound_vol;
    item = add_menu_item(m,MNU_ITEM_VALUE,0,
            menu_func_label(volume_label),val);
    item->action = update_sndvolume;

    val.value = &game_settings.music_vol;
    item = add_menu_item(m,MNU_ITEM_VALUE,1,
            menu_func_label(volume_label),val);

    add_ok(m);

    return m;
}

/* Called by a menu callback */
/* Save startup settings */
static int save_startup_settings (MenuCommand cmd, struct MenuItem * item)
{
    if (cmd != MNU_ENTER)
        return 0;
    save_startup_config ();
    intro_message ("Game startup defaults were saved");
    return 0;
}

/* Menu callback to get the label for video mode */
static const char *vmode_label(struct MenuItem *item) {
    switch(*(Videomode*)item->value.value) {
        case VID_640: return "Video mode: 640x480";
        case VID_800: return "Video mode: 800x600";
        case VID_1024: return "Video mode: 1024x768";
    }
    return "Video mode: ???";
}

/* Create the startup settings menu */
static struct Menu *make_startup_menu(struct Menu *parent) {
    struct Menu *m;
    struct MenuText save,back;
    struct MenuValue val;
    struct MenuItem *item;

    /* Create some special menu item labels */
    save = menu_txt_label("Save settings");
    save.color = font_color_red;
    back = menu_txt_label("Return");
    back.color = font_color_green;

    /* Create menu */
    m = create_menu(0,parent,NULL,NULL,NULL,0);
    add_caption(m,"Default startup settings");

    val.value = &luola_options.fullscreen;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Start in %s mode"),val);
    item->text_enabled="fullscreen";
    item->text_disabled="windowed";

    val.value = &luola_options.hidemouse;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Mouse pointer is %s"),val);
    item->text_enabled="hidden";
    item->text_disabled="shown";

    val.value = &luola_options.joystick;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Gamepad support is %s"),val);

    val.value = &luola_options.sounds;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Sound support is %s"),val);

    val.value = &luola_options.sfont;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Font engine: %s"),val);
    item->text_enabled="SFont";
    item->text_disabled="SDL_ttf";

    val.value = &luola_options.mbg_anim;
    item = add_menu_item(m,MNU_ITEM_TOGGLE,1,
            menu_txt_label("Menu background animations are %s"),val);

    val.max = 2;
    val.min = 0;
    val.inc = 1;
    val.value = (int*)&luola_options.videomode;
    item = add_menu_item(m,MNU_ITEM_VALUE,0,
            menu_func_label(vmode_label),val);

    add_menu_item(m,MNU_ITEM_SEP,0,menu_txt_label("- - -"),MnuNullValue);
    add_menu_item(m,MNU_ITEM_LABEL,0,save,MnuNullValue)->action =
        save_startup_settings;
    add_menu_item(m,MNU_ITEM_PARENT,0,back,MnuNullValue);

    return m;
}
/* Save settings */
static int save_settings (MenuCommand cmd, struct MenuItem * item)
{
    if (cmd != MNU_ENTER)
        return 0;
    save_game_config ();
    intro_message ("Game settings were saved");
    return 0;
}

/* Create the options menu */
static struct Menu *make_options_menu(struct Menu *parent,struct MenuDrawingOptions *options) {
    struct MenuText save,back;
    struct Menu *opts;
    struct MenuItem *audiomenu;
    /* Create some special menu item labels */
    save = menu_txt_label("Save settings");
    save.color = font_color_red;
    back = menu_txt_label("Return");
    back.color = font_color_green;
    /* Create menu */
    opts = create_menu(0,parent,options,NULL,NULL,0);

    add_caption(opts,"Luola configuration");
    add_submenu(opts,1,"Input settings", make_input_menu(opts));
    add_submenu(opts,2,"Game settings", make_game_menu(opts));
    add_submenu(opts,3,"Level settings", make_level_menu(opts));
    audiomenu = add_submenu(opts,4,"Audio settings", make_audio_menu(opts));
    add_submenu(opts,5,"Startup settings", make_startup_menu(opts));
    add_menu_item(opts,MNU_ITEM_SEP,0,menu_txt_label("- - -"),MnuNullValue);
    add_menu_item(opts,MNU_ITEM_LABEL,0,save,MnuNullValue)->action = save_settings;
    add_menu_item(opts,MNU_ITEM_PARENT,0,back,MnuNullValue);

    if(SDL_WasInit(SDL_INIT_AUDIO)==0)
        audiomenu->sensitive = 0;

    return opts;
}

/* Load some graphics and create menus */
void init_intro (LDAT *miscfile) {
    struct MenuDrawingOptions mainmenu, optionsmenu;
    struct MenuValue optsval;
    int logoy=10;
    int yoffset=250;
    int optionsy=80;
    /* Load the input device icons */
    keyb_icon = load_image_ldat (miscfile, 1, T_ALPHA, "KEYBOARD", 0);
    pad_icon = load_image_ldat (miscfile, 1, T_ALPHA, "PAD", 0);

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
    mainmenu.area.x = 0;
    mainmenu.area.y = yoffset;
    mainmenu.area.w = screen->w;
    mainmenu.area.h = screen->h-yoffset;
    mainmenu.spacing = MENU_SPACING;
    mainmenu.selection_color = map_rgba(0,128,255,255);
    mainmenu.left_offset = screen->h/2;

    optionsmenu = mainmenu;
    optionsmenu.area.y = optionsy;
    optionsmenu.left_offset = 130;

    /* Load graphics */
    intr_logo = load_image_ldat (miscfile, 1, 2, "LOGO", 0);
    if (intr_logo) {
        intr_logo_rect.x = screen->w / 2 - intr_logo->w / 2;
        intr_logo_rect.y = logoy;
        centered_string (intr_logo, Smallfont, intr_logo->h - 20, VERSION,
                         font_color_red);
    }

    /* Build main menu */
    intro_menu = create_menu(0,NULL,&mainmenu,
            "enabled","disabled",INTRO_RVAL_EXIT);
    add_menu_item(intro_menu,MNU_ITEM_RETURN,INTRO_RVAL_STARTGAME,
            menu_txt_label("Start game"),MnuNullValue);
    optsval.value = (int*)make_options_menu(intro_menu,&optionsmenu);
    add_menu_item(intro_menu,MNU_ITEM_SUBMENU,0,
            menu_txt_label("Settings"),optsval);
    add_menu_item(intro_menu,MNU_ITEM_RETURN,INTRO_RVAL_EXIT,
            menu_txt_label("Exit"),MnuNullValue);

    intro_menu->predraw = draw_logo;

    /* Misc. variable initialization */
    intr_message.show = 0;
    intr_message.text = NULL;
    intr_message.setkey = NULL;
    intr_message.framecolor = map_rgba(200,80,80,220);
    intr_message.fillcolor = map_rgba(0,0,0,220);
}

/* The intro and settings screens */
int game_menu_screen (void) {
    draw_intro_screen ();
    return intro_event_loop ();
}

/* Intro screen eventloop */
static int intro_event_loop (void) {
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
                    }
                    else if (Event.key.keysym.sym == SDLK_ESCAPE)
                        command = MNU_BACK;
                }
                if (command >= 0) {
                    if (command != MNU_ENTER && command != MNU_BACK)
                        playwave (WAV_BLIP);
                    else
                        playwave (WAV_BLIP2);
                    rval = menu_control (&intro_menu, command);
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
            case SDL_QUIT:
                exit(0);
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

/* Called by a menu callback (this is exported in intro.h as well) */
/* Draw input icons */
int draw_input_icon (int x, int y, MenuAlign align, struct MenuItem * item)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    if (item->ID > 4) {
        fprintf (stderr,"Bug: no input icon %d\n",item->ID);
        return 0;
    }
    if (game_settings.controller[item->ID-1].number != 0) {
        if (pad_icon == NULL) return 0;
        rect.w = pad_icon->w;
        if (align == MNU_ALIGN_LEFT)
            rect.x -= rect.w + 7;
        SDL_BlitSurface (pad_icon, NULL, screen, &rect);
        draw_number(screen, rect.x+rect.w - 2,rect.y + rect.h - NUMBER_H - 2,
                game_settings.controller[item->ID-1].number,col_red);
    } else {
        if (keyb_icon == NULL) return 0;
        rect.w = keyb_icon->w;
        if (align == MNU_ALIGN_LEFT)
            rect.x -= rect.w + 7;
        SDL_BlitSurface (keyb_icon, NULL, screen, &rect);
    }
    return rect.w + 7;
}


/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : player.h
 * Description : Player information and animation
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

#ifndef PLAYER_H
#define PLAYER_H

#include "SDL.h"
#include "intro.h"
#include "font.h"
#include "weapon.h"
#include "pilot.h"

/* This structure represents the state of the game controller */
typedef struct {
    signed char axis[2];        /* Vertical, Horizontal */
    unsigned char weapon1;
    unsigned char weapon2;
} GameController;

typedef struct {
    struct Ship *ship;
    Pilot pilot;
    GameController controller;
    enum {INACTIVE=0,ALIVE,DEAD,BURIED} state;
    int recall_cooloff;
    int weapon_select;
    /* So the game remembers which weapons the player selected */
    WeaponType specialWeapon;
    SWeaponType standardWeapon;
} Player;

/* Initialization */
extern int init_players (void);
extern void reinit_players (void);

/* Animation */
extern void draw_player_hud (void);
extern void draw_radar (SDL_Rect rect, int plr);
extern void animate_players ();

/* Handling */
extern int hearme (int x, int y);
extern int find_player (struct Ship * ship);
extern void buryplayer (int plr);
extern void eject_pilot (int plr);
extern void set_player_message (int plr, FontSize size,
                                SDL_Color color, int dur, const char *msg, ...);
extern void player_cleanup (void);
extern void recall_ship (int plr);

/* Input */
extern void player_keyhandler (SDL_KeyboardEvent * event, Uint8 type);
extern void player_joybuttonhandler (SDL_JoyButtonEvent * button);
extern void player_joyaxishandler (SDL_JoyAxisEvent * axis);

/* Globals */
extern Player players[4];
extern int player_teams[4];
extern int radars_visible;
extern int plr_teams_left;
extern SDL_Surface *plr_messages[4];

/* Shh.. Quiet ! */
extern void cheatcode (SDLKey sym);
extern char cheat, cheat1;

#endif

/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
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
    struct Pilot pilot;
    GameController controller;
    enum {INACTIVE=0,ALIVE,DEAD,BURIED} state;
    int recall_cooloff;
    int weapon_select;
    /* So the game remembers which weapons the player selected */
    int standardWeapon;
    int specialWeapon;
} Player;

/* Load player related datafiles */
extern void init_players (LDAT *misc);

/* Reset players for a new game */
extern void reset_players (void);

/* Prepare players for a new round */
extern void reinit_players (void);

/* Draw statusbar */
extern void draw_player_hud (void);

/* Draw radars */
extern void draw_radar (SDL_Rect rect, int plr);

/* Animation */
extern void animate_players ();

/* Return the player (in ship or a pilot) nearest to the coordinates */
extern int hearme (int x, int y);

/* Find the player to whom ship belongs */
extern int find_player (struct Ship * ship);

/* Mark player as killed. Screen will fade out and player will be marked
 * as buried */
extern void kill_player (int plr);

/* Eject pilot from the ship */
extern void eject_pilot (int plr);

/* Display a message on player viewport */
extern void set_player_message (int plr, FontSize size,
                                SDL_Color color, int dur, const char *msg, ...);

/* Teleport a ship to its pilot */
extern void recall_ship (int plr);

/* Get the number of active players in the game */
extern int active_players(void);

/* Which team does a player belong to */
extern int get_team(int plr);

/* Set which team a player belongs to */
extern void set_team(int plr, int team);

/* Check if two players are on the same team */
extern int same_team(int plr1, int plr2);

/* Input */
extern void player_keyhandler (SDL_KeyboardEvent * event, Uint8 type);
extern void player_joybuttonhandler (SDL_JoyButtonEvent * button);
extern void player_joyaxishandler (SDL_JoyAxisEvent * axis);

/* Globals */
extern Player players[4];
extern int radars_visible;
extern int plr_teams_left;
extern SDL_Surface *plr_messages[4];

#endif

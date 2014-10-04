/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : player.c
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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "SDL.h"

#include "fs.h"
#include "console.h"
#include "particle.h"
#include "animation.h"
#include "level.h"
#include "weapon.h"
#include "player.h"
#include "game.h"
#include "special.h"
#include "critter.h"
#include "ship.h"

#include "audio.h"

/* Internally used globals */
static SDL_Surface *plr_weaponsel_bg;
static SDL_Surface *plr_weapons[4];
/*static SDL_Surface *plr_criticals;*/
static signed int player_message[4];
static int plr_teamc[4];
static Uint32 plr_healthbar_col, plr_healthbar_col2, plr_healthbar_col3,
    plr_energybar_col, plr_blankbar_col;

/* Exported globals */
SDL_Surface *plr_messages[4];
int player_teams[4];
int plr_teams_left;
int radars_visible;
Player players[4];

/* Internally used functions */
static void player_key_update(unsigned char plr);

/* Initialize players */
void init_players (LDAT *misc) {
    int p;
    plr_healthbar_col = SDL_MapRGB (screen->format, 80, 98, 186);
    plr_healthbar_col2 = SDL_MapRGB (screen->format, 200, 200, 0);
    plr_healthbar_col3 = SDL_MapRGB (screen->format, 200, 0, 0);
    plr_energybar_col = SDL_MapRGB (screen->format, 186, 195, 195);
    plr_blankbar_col = SDL_MapRGB (screen->format, 106, 44, 123);
    for (p = 0; p < 4; p++) {
        players[p].specialWeapon = WGrenade;
        players[p].standardWeapon = SShot;
        /* Initialize message surface */
        plr_messages[p] = NULL;
        plr_weapons[p] = NULL;
    }
    radars_visible = 0;
    /* Load graphics */
    plr_weaponsel_bg = load_image_ldat (misc, 1, T_COLORKEY, "WEAPONSEL", 0);
    /* TODO: Load critical icons */
}

/* Return the number of active players */
int active_players(void) {
    int p,a=0;
    for(p=0;p<4;p++)
        if(players[p].state != INACTIVE) a++;
    return a;
}

/* Prepare players for a new round */
void reinit_players (void) {
    int p, unlucky = -1;
    endgame = -1;
    if (active_players() == 0) {
        fprintf(stderr, "Bug! reinit_players(): no players selected!\n");
        exit(1);
    }
    if (game_settings.playmode == OutsideShip1) {
        do
            unlucky = rand () % 4;
        while (players[unlucky].state == INACTIVE);
    }
    for (p = 0; p < 4; p++) {
        players[p].ship=NULL;
        if (players[p].state == INACTIVE)
            continue;

        players[p].state = ALIVE;
        players[p].recall_cooloff = 0;
        init_pilot(&players[p].pilot,p);
        memset (&players[p].controller, 0, sizeof (GameController));
        if (plr_weapons[p]) {
            SDL_FreeSurface (plr_weapons[p]);
            plr_weapons[p] = NULL;
        }
        if(game_settings.playmode == RndWeapon) {
            players[p].standardWeapon = rand()%SWeaponCount;
            players[p].specialWeapon = rand()%(WeaponCount-1)+1;
        }
        players[p].weapon_select = 0;
        if (game_settings.playmode == OutsideShip1) {
            if (p != unlucky)
                players[p].ship =
                    create_ship (Grey, players[p].standardWeapon,
                                 players[p].specialWeapon);
            else
                players[p].ship = NULL;
        } else {
            players[p].ship =
                create_ship (Red + p, players[p].standardWeapon,
                             players[p].specialWeapon);
        }
        if (game_settings.playmode == RndCritical) {
            /* Randomize criticals */
            int c, cc;
            cc = rand () % CRITICAL_COUNT * 2;
            for (c = 0; c < cc; c++)
                ship_critical (players[p].ship, 0);
        }
        if (p != unlucky) {
            players[p].ship->x = lev_level.player_def_x[0][p];
            players[p].ship->y = lev_level.player_def_y[0][p];
        }
        if (game_settings.playmode == OutsideShip
            || game_settings.playmode == OutsideShip1) {
            eject_pilot (p);
            players[p].pilot.x = lev_level.player_def_x[1][p];
            players[p].pilot.y = lev_level.player_def_y[1][p];
        }
        player_message[p] = 0;
        if (plr_messages[p]) {
            SDL_FreeSurface (plr_messages[p]);
            plr_messages[p] = NULL;
        }
    }
    /* Count the number of teams and the number of players in each of them */
    memset(plr_teamc,0,sizeof(int)*4);
    for (p = 0; p < 4; p++)
        if (players[p].state!=INACTIVE)
            plr_teamc[player_teams[p]]++;       /* Number of players per team */
    plr_teams_left = 0;
    for (p = 0; p < 4; p++)
        if (plr_teamc[p])
            plr_teams_left++;
    if (plr_teams_left == 1) {
        plr_teams_left++;
    }

    /* Initialize player screen geometry */
    reinit_animation ();
    recalc_geometry ();
    rearrange_animation ();
}

/* Draw the player statusbars */
static void draw_player_statusbar (int p) {
    SDL_Rect outline,health,energy;
    Uint32 healthcol;
    outline.x = lev_rects[p].x + 19;
    outline.y = lev_rects[p].y + cam_rects[p].h - 7;
    outline.w = cam_rects[p].w - 38;
    outline.h = 7;

    health = outline;
    health.x++; health.w -= 2;
    health.y++; health.h = 2;

    energy = health;
    energy.y += 3;
    
    SDL_FillRect (screen, &outline, 0);
    SDL_FillRect (screen, &health, plr_blankbar_col);
    SDL_FillRect (screen, &energy, plr_blankbar_col);

    if (players[p].ship->health > 0.5)
        healthcol = plr_healthbar_col;
    else if (players[p].ship->health > 0.2)
        healthcol = plr_healthbar_col2;
    else
        healthcol = plr_healthbar_col3;

    health.w *= players[p].ship->health;
    energy.w *= players[p].ship->energy;
    SDL_FillRect (screen, &health, healthcol);
    SDL_FillRect (screen, &energy, plr_energybar_col);
}

/* Draw the weapon selection screen for player */
static void draw_player_weaponselection (int plr) {
    SDL_Rect rect = lev_rects[plr];
    SDL_BlitSurface (plr_weaponsel_bg, NULL, screen, &rect);
    /* Update the string surface if necessary */
    if (players[plr].specialWeapon != players[plr].ship->special
        || plr_weapons[plr] == NULL) {
        players[plr].specialWeapon = players[plr].ship->special;
        SDL_FreeSurface (plr_weapons[plr]);
        plr_weapons[plr] =
            renderstring (Smallfont, weap2str (players[plr].specialWeapon),
                          font_color_green);
    }
    /* Blit text to screen */
    rect.x += cam_rects[plr].w/2 - plr_weapons[plr]->w / 2;
    rect.y += cam_rects[plr].h/2 - plr_weapons[plr]->h / 2;
    SDL_BlitSurface (plr_weapons[plr], NULL, screen, &rect);
}

/* Draw player messages */
static void draw_player_message (int plr)
{
    SDL_Rect rect;
    rect.x = lev_rects[plr].x + cam_rects[plr].w/2 - plr_messages[plr]->w/2;
    rect.y = lev_rects[plr].y + cam_rects[plr].h/2 - plr_messages[plr]->h/2;
    SDL_BlitSurface (plr_messages[plr], NULL, screen, &rect);
}

#if 0 /* TODO: Activate this when you have the critical icons */
static  void draw_player_criticals(int plr) {
  SDL_Rect rect=lev_rects[plr];
  sDL_Rect src;
  int c;
  rect.y+=10;
  src.x=0;
  src.y=0;
  src.w=32;
  src.h=32;
  for(c=0;c<CRITICAL_COUNT;c++) {
    rect.x+=32;
    src.x+=32;
    if((players[plr].ship->criticals&(1<<c))
      SDL_BlitSurface(plr_criticals,&src,screen,&rect);
  }
}
#endif

/* All sorts of information for the player */
void draw_player_hud(void) {
    int p;
    for (p = 0; p < 4; p++) {
        if (players[p].state==ALIVE || players[p].state==DEAD) {
            if(players[p].ship) {
                draw_player_statusbar(p);
#if 0
                if(players[p].ship->criticals) draw_player_criticals(p);
#endif
            }
            if (players[p].weapon_select)
                draw_player_weaponselection (p);
            if (player_message[p])
                draw_player_message (p);
        }
    }
}

void draw_radar (SDL_Rect rect, int plr) {
    double d;
    int p, dx, dy, dx2, dy2;
    rect.x += 8;
    rect.y += 8;
    if (players[plr].ship == NULL)
        return;
    for (p = 0; p < 4; p++)
        if (p != plr && players[p].ship && players[p].ship->visible) {
            d = atan2 (players[plr].ship->x - players[p].ship->x,
                       players[plr].ship->y - players[p].ship->y);
            dx = sin (d) * 7;
            dy = cos (d) * 7;
            dx2 = sin (d) * 17;
            dy2 = cos (d) * 17;
            draw_line (screen, rect.x - dx, rect.y - dy, rect.x - dx2,
                       rect.y - dy2, col_plrs[p]);
        }
}

int find_player (struct Ship * ship) {
    int p;
    for (p = 0; p < 4; p++)
        if (players[p].ship == ship)
            return p;
    return -1;
}

void player_cleanup (void) {
    clean_ships ();
}

/*** PLAYER ANIMATION ***/
void animate_players ()
{
    struct dllist *ships;
    int n,newx,newy;
    if (plr_teams_left == 0 && endgame == -1)
        endgame = 30;           /* In case of tie game, end it */
    for (n = 0; n < 4; n++) {
        if (players[n].state!=ALIVE)
            continue;
        /* Counters */
        game_status.lifetime[n]++;
        if (player_message[n] > 0)
            player_message[n]--;
        if(players[n].recall_cooloff>0)
            players[n].recall_cooloff--;
        /* Weapon selection */
        if (players[n].weapon_select) {
            if (players[n].ship->onbase == 0) {
                players[n].weapon_select = 0;
            } else {
                if (players[n].controller.axis[1] > 0
                    && players[n].ship->cooloff == 0) {
                    players[n].ship->special--;
                    if (players[n].ship->special < 1)
                        players[n].ship->special = WeaponCount - 1;
                    players[n].ship->cooloff = 5;       /* We borrow the weapon cooloff */
                    players[n].ship->energy = 0;
                } else if (players[n].controller.axis[1] < 0
                           && players[n].ship->cooloff == 0) {
                    players[n].ship->special++;
                    if (players[n].ship->special == WeaponCount)
                        players[n].ship->special = 1;
                    players[n].ship->cooloff = 5;
                    players[n].ship->energy = 0;
                }
            }
        }
        if (players[n].ship == NULL) {
            /* End of game ? */
            if (endgame == -1 && game_settings.endmode
                && players[n].pilot.onbase) {
                if (plr_teams_left < 2)
                    endgame = 30;
            }
            /* Jump into an available ship */
            ships = ship_list;
            while (ships) {
                struct Ship *ship=ships->data;
                if (players[n].pilot.x >= ship->x - 8
                    && players[n].pilot.x <= ship->x + 8)
                    if (players[n].pilot.y +
                        players[n].pilot.sprite[0]->h >= ship->y - 8
                        && players[n].pilot.y +
                        players[n].pilot.sprite[0]->h - 2 <=
                        ship->y + 8)
                        if (find_player (ship) < 0
                            && ship->eject_cooloff == 0) {
                            players[n].ship = ship;
                            players[n].recall_cooloff = 0;
                            if (players[n].ship->color == Grey)
                                claim_ship (players[n].ship, n);
                        }
                ships = ships->next;
            }
        } else {
            /* End of game ? */
            if (endgame == -1 && game_settings.endmode
                && players[n].ship->onbase) {
                if (plr_teams_left < 2)
                    endgame = 30;
            }
        }
        /* Update camera position */
        if (players[n].ship) {
            newx = Round(players[n].ship->x);
            newy = Round(players[n].ship->y);
        } else {
            newx = Round(players[n].pilot.x);
            newy = Round(players[n].pilot.y);
            if (players[n].pilot.parachuting) {
                newy +=
                    players[n].pilot.sprite[2]->h -
                    players[n].pilot.sprite[0]->h;
                newx +=
                    players[n].pilot.sprite[2]->w / 2 -
                    players[n].pilot.sprite[0]->w / 2;
            }
        }
        /* If viewport width or height is greater than that of the */
        /* level, x or y will be set to a negative value and luola */
        /* will crash later. (Most likely in draw_stars) */
        cam_rects[n].x = newx - cam_rects[n].w/2;
        cam_rects[n].y = newy - cam_rects[n].h/2;
        if (cam_rects[n].x < 0)
            cam_rects[n].x = 0;
        else if (cam_rects[n].x > lev_level.width - cam_rects[n].w)
            cam_rects[n].x = lev_level.width - cam_rects[n].w;
        if (cam_rects[n].y < 0)
            cam_rects[n].y = 0;
        else if (cam_rects[n].y > lev_level.height - cam_rects[n].h)
            cam_rects[n].y = lev_level.height - cam_rects[n].h;
    }
}

/* Keyboard handling */
void player_keyhandler (SDL_KeyboardEvent * event, Uint8 type)
{
    int p, b;
    SDLKey key = event->keysym.sym;
    for (p = 0; p < 4; p++) {   /* Check all 4 players */
        if (players[p].state==ALIVE&&game_settings.controller[p].number==0) {
            for (b = 0; b < 6; b++) { /* Each player has 6 buttons */
                if (game_settings.controller[p].keys[b] == key) {
                    switch (b) {
                    case 0:
                        players[p].controller.axis[0] = (type == SDL_KEYDOWN);
                        break;
                    case 1:
                        players[p].controller.axis[0] = -(type == SDL_KEYDOWN);
                        break;
                    case 2:
                        players[p].controller.axis[1] = (type == SDL_KEYDOWN);
                        break;
                    case 3:
                        players[p].controller.axis[1] = -(type == SDL_KEYDOWN);
                        break;
                    case 4:
                        players[p].controller.weapon1 = (type == SDL_KEYDOWN);
                        break;
                    case 5:
                        players[p].controller.weapon2 = (type == SDL_KEYDOWN);
                        break;
                    }
                    player_key_update (p);
                    break;
                }
            }
        }
    }
}

/* Joystick axis handling */
void player_joyaxishandler (SDL_JoyAxisEvent * axis)
{
    int p;
    signed char state;
    for (p = 0; p < 4; p++) {
        if (players[p].state!=ALIVE||game_settings.controller[p].number==0)
            continue;
        if (game_settings.controller[p].number - 1 != axis->which)
            continue;
        state = abs (axis->value) > 16384;
        if (axis->axis == 0)    /* left/right */
            players[p].controller.axis[1] =
                (axis->value > 0) ? -state : state;
        else if (axis->axis == 1)       /* up/down */
            players[p].controller.axis[0] =
                (axis->value > 0) ? -state : state;
        player_key_update (p);
        break;
    }
}

/* Joystick button handling */
void player_joybuttonhandler (SDL_JoyButtonEvent * button)
{
    int p;
    for (p = 0; p < 4; p++) {
        if (players[p].state!=ALIVE||game_settings.controller[p].number==0)
            continue;
        if (game_settings.controller[p].number - 1 != button->which)
            continue;
        switch (button->button) {
        case 1:
        case 2:
            players[p].controller.weapon2 = button->state == SDL_PRESSED;
            break;
        default:
            players[p].controller.weapon1 = button->state == SDL_PRESSED;
            break;
        }
        player_key_update (p);
    }
}

/* Generic input handling (this is where the real magic happens) */
void player_key_update (unsigned char plr) {
    struct Ship *ship;
    if(players[plr].state!=ALIVE) return;
    if (players[plr].ship) {
        if (players[plr].ship->dead) {
            if (players[plr].controller.weapon2)
                eject_pilot (plr);
            return;
        }
        if (players[plr].ship->frozen)
            return;
        /* Weapon selection */
        if (players[plr].ship->onbase && players[plr].controller.axis[0] < 0) {
            players[plr].weapon_select = 1;
        } else
            players[plr].weapon_select = 0;
        if (players[plr].weapon_select) {
        }
        /* Eject pilot */
        if (players[plr].controller.axis[0] < 0
            && players[plr].controller.weapon1 &&
            game_settings.eject) {
            eject_pilot (plr);
            return;
        }
        if (players[plr].ship->darting)
            return;
        /* Remote control */
        if (players[plr].ship->remote_control) {
            if ((int) players[plr].ship->remote_control > 1)
                ship = players[plr].ship->remote_control;
            else
                return; /* If you are being remote controlled, controls are disabled */
        } else
            ship = players[plr].ship;
        if (players[plr].controller.weapon2 && ship != players[plr].ship) {
            /* Disengage remote control */
            ship_fire (players[plr].ship, SpecialWeapon);
            return;
        }
        /* Normal controls */
        ship->thrust = players[plr].controller.axis[0] > 0;
        ship->turn = players[plr].controller.axis[1] * TURN_SPEED;
        if ((ship->criticals & CRITICAL_LTHRUSTER) && ship->turn < 0)
            ship->turn = 0;
        else if ((ship->criticals & CRITICAL_RTHRUSTER) && ship->turn > 0)
            ship->turn = 0;
        ship->fire_weapon = players[plr].controller.weapon1
            && !(ship->criticals & CRITICAL_STDWEAPON);
        if (ship->fire_special_weapon != players[plr].controller.weapon2
            && !(ship->criticals & CRITICAL_SPECIAL)) {
            switch (ship->special) {
            /* Some weapons need to be fired only once */
            case WCloak:
            case WShield:
            case WGhostShip:
            case WAfterBurner:
            case WRepair:
            case WRemote:
            case WAntigrav:
                ship_fire (ship, SpecialWeapon);
                ship->fire_special_weapon = 0;
                players[plr].controller.weapon2 = 0;
                break;
            default:
                ship->fire_special_weapon = players[plr].controller.weapon2;
                break;
            }
        }
    } else {
        control_pilot (plr);
    }
}

/* If a grenade explodes in the woods and there is nobody around to hear it,
 * does it really make a sound ? */
int hearme (int x, int y) {
    double dist = 9999, d;
    int nearest=-1;
    int p;
    for (p = 0; p < 4; p++)
        if (players[p].state==ALIVE) {
            if (players[p].ship)
                d = hypot (players[p].ship->x - x, players[p].ship->y - y);
            else
                d = hypot (players[p].pilot.x - x, players[p].pilot.y - y);
            if (d < dist) {
                nearest = p;
                dist = d;
            }
        }
    if (dist < HEARINGRANGE)
        return nearest;
    return -1;
}

void set_player_message (int plr, FontSize size, SDL_Color color,
                         int dur, const char *msg, ...)
{
    char buf[256];
    va_list ap;
    va_start (ap, msg);
    vsprintf (buf, msg, ap);
    va_end (ap);
    if (plr_messages[plr]) {
        SDL_FreeSurface (plr_messages[plr]);
        plr_messages[plr] = NULL;
    }
    if (strlen (buf) && dur) {
        plr_messages[plr] = renderstring (size, buf, color);
        player_message[plr] = dur;
    } else
        player_message[plr] = 0;
}

/* Finish off a player (set state to dead, remove ship...) */
/* This doesn't actually set player state to BURIED. That is set */
/* in animation.c code after the player has been dead for FADE_STEP frames */
/* so the fadeout animation can be drawn. */
void buryplayer (int plr) {
    if(players[plr].state!=ALIVE) {
        printf("Bug! buryplayer(%d): Player is already dead or inactive!\n",plr);
    } else {
        players[plr].state=DEAD;
        players[plr].ship=NULL;
        plr_teamc[player_teams[plr]]--;
        if (plr_teamc[player_teams[plr]] == 0)
            plr_teams_left--;
        set_player_message (plr, Bigfont, font_color_red, -1, "Dead");
        kill_plr_screen (plr);
    }
}

/* Eject the pilot */
void eject_pilot (int plr) {
    pilot_any_ejected++;
    if (players[plr].ship) {
        players[plr].ship->fire_weapon = 0;
        players[plr].ship->thrust = 0;
        if (players[plr].ship->dead == 0)
            players[plr].ship->turn = 0;
        if (players[plr].ship->afterburn > 1) {
            players[plr].ship->maxspeed = MAXSPEED;
            players[plr].ship->afterburn = 1;
        }
        players[plr].pilot.x = players[plr].ship->x;   /* If you eject a player without a ship */
        players[plr].pilot.y = players[plr].ship->y - players[plr].pilot.hy;  /* you'd better set the coordinates yourself... */
        players[plr].ship->eject_cooloff = 70;
        players[plr].ship = NULL;
    }
    memset (&players[plr].controller, 0, sizeof (GameController));
    players[plr].weapon_select = 0;
    players[plr].pilot.vector = makeVector (0, 3);
    players[plr].pilot.rope = 0;
    players[plr].pilot.walking = 0;
    players[plr].pilot.updown = 0;
    players[plr].pilot.lock = 0;
    players[plr].pilot.parachuting = 0;
    players[plr].pilot.maxspeed = MAXSPEED;
    players[plr].pilot.hx = players[plr].pilot.sprite[0]->w / 2;
    players[plr].pilot.hy = players[plr].pilot.sprite[0]->h;
}

/*** Recall the ship to the pilot ***/
/* If the ship has been destroyed, create a new one */
void recall_ship (int plr) {
    struct Ship *ship = NULL;
    struct dllist *ships;
    int taken = 0;
    int tx, ty;

    if(players[plr].recall_cooloff>0) return;

    /* First, attempt to locate the ship */
    ships = ship_list;
    while (ships) {
        if (((struct Ship*)ships->data)->color == Red + plr) {
            if (find_player (ships->data)<0)
                ship=ships->data;
            else
                taken = 1;
            break;
        }
        ships = ships->next;
    }
    if (taken) {              /* If the ship is taken, it cannot be recalled */
        set_player_message (plr, Smallfont, font_color_green, 25,
                            "Ship taken, cannot recall");
        return;
    }
    /* Find good coordinates */
    tx = players[plr].pilot.x;
    ty = players[plr].pilot.y;
    while (ty > players[plr].pilot.y - 20) {
        if (ty <= 0
            || (lev_level.solid[tx][ty] != TER_FREE
                && lev_level.solid[tx][ty] != TER_TUNNEL)) {
            ty++;
            break;
        }
        ty--;
    }
    /* Recall the ship */
    if (ship) {     /* Teleport the existing ship */
        drop_jumppoint (ship->x, ship->y, plr + 20);
        drop_jumppoint (tx, ty, plr + 20);
        ship->vector.x = 0;
        ship->vector.y = 0;
    } else {        /* Ship probably destroyed, create a new one */
        SpecialObj *jump;
        ship =
            create_ship (Red + plr, players[plr].standardWeapon,
                         players[plr].specialWeapon);
        ship->x = tx;
        ship->y = ty;
        ship->energy = 0.3;
        ship->health = 0.5;
        jump = malloc (sizeof (SpecialObj));
        jump->x = tx - 16;
        jump->y = ty - 16;
        jump->owner = -1;
        jump->frame = 0;
        jump->timer = 0;
        jump->age = JPSHORTLIFE;
        jump->type = WarpExit;
        jump->link = NULL;
        addspecial (jump);
    }
    players[plr].recall_cooloff=JPLONGLIFE*2;
}


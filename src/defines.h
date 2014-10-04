/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : defines.h
 * Description : Miscallenous definitions
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

#ifndef DEFINES_H
#define DEFINES_H

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

/* Startup defaults */
#define FULLSCREEN	0       /* By default, we don't use fullscreen (Note, when compiling under win32, this defaults to 1) */
#define HIDEMOUSE	1       /* The mouse is only in the way when it's over the window */
#define JOYSTICK	1       /* Pad controller is very handy (especially when there are more than two players...) */
#define SOUNDS		1       /* Are sounds enabled by default */
#define MUSIC		1       /* Is music enabled by default */

/* Truetype fonts. These are used only if SDL_ttf is enabled */
#define FONT_CFG_FILE "fonts.cfg"

/* Filenames */
#define CONF_FILE "luola.cfg"
#define PLAYLIST_FILE "battle.pls"
#define STARTUP_FILE "startup.cfg"

/*#define VWING_STYLE_SMOKE*/     /* This enables the V-Wing style smoketrail. */
#define HEARINGRANGE	350.0
#define MAX_WIND_TIME	400     /* Maximium time in frames that a breeze can last */
#define STARCOUNT	15      /* Number of stars drawn */
#define JSTRESHOLD	16384   /* Joystick axis treshold */

/* User changeable limits */
/* Note that you can change these from inside the game as well, these 
 * limits are the absolute maximiums */
#define MAX_INFANTRY	50  /* Maximium number of ground troops per player */
#define MAX_HELICOPTER	50  /* Maximium number of helicopters per player */
/* How many neutral critters are created in the beginning of the level */
#define MAX_COWS	15
#define MAX_BIRDS	15
#define MAX_FISH	15
#define MAX_BATS	30

/* Speeds */
#define GAME_SPEED          30   /* Speed of the game. (1000/FPS) */
#define GRAVITY             0.136
#define WEAP_GRAVITY        0.11
#define THRUST              0.4
#define MAXSPEED            2.8
#define TURN_SPEED          0.15
#define PROJ_UW_MAXSPEED    3.15 /* Max speed of a projectile underwater */
#define PROJ_MAXSPEED       8.5  /* The generic maximium speed for a projectile */

#define PILOT_TOOFAST       10  /* For how long can a pilot fall too fast without dieing when hitting ground */

#define JPLONGLIFE	    400     /* How long does a jump-point last */
#define JPMEDIUMLIFE    100
#define JPSHORTLIFE     25

#define EMBER_LIFE	33      /* How long does an ember last before splitting */
#define MIRV_LIFE	26      /* How long does a MIRV missile last before splitting */
#define SHIP_WHITE_DUR	4   /* After receiving damage, for how many frames the ship appears white */

#define CRITTER_ANGRY	35  /* How long does a critter stay angry */

#define MAX_ROPE_LEN	120 /* Maximium length of the pilots rope in pixels */
#define ROPE_SPEED      5   /* How fast does the rope unwind */

#define BASE_REGEN_SPEED        9   /* Delay between each regenerated pixel */
#define SNOWFLAKE_INTERVAL      20
#define DIVIDINGMINE_INTERVAL   213
#define DIVIDINGMINE_RAND       40
#define EXPLOSION_CLUSTER_SPEED 5  /* How soon the explosion sends out shrapnel */

#define FADE_STEP               35 /* How many steps in fade animation */

/* Critical hits */
#define CRITICAL_COUNT          8
#define CRITICAL_ENGINE         0x01
#define CRITICAL_FUELCONTROL    0x02
#define CRITICAL_LTHRUSTER      0x04
#define CRITICAL_RTHRUSTER      0x08
#define CRITICAL_CARGO          0x10
#define CRITICAL_STDWEAPON      0x20
#define CRITICAL_SPECIAL        0x40
#define CRITICAL_POWER          0x80

/* Graph (change these if the numer of frames in sprites change) */
#define SHIP_POSES      36
#define REMOTE_FRAMES   11
#define EXPL_FRAMES     7
#define WARP_FRAMES     21
#define WARP_LOOP       10
#define FIRE_FRAMES     26
#define FIRE_SPREAD     17
#define FIRE_RANDOM     17
#define TURRET_FRAMES   15

#define BAT_FRAMES          3
#define BIRD_FRAMES         4
#define COW_FRAMES          6
#define FISH_FRAMES         2
#define INFANTRY_FRAMES	    1
#define HELICOPTER_FRAMES   2

/** You shouldn't need to change anything below this line */
/* Landscape */
#define TER_FREE        0
#define TER_GROUND      1
#define TER_UNDERWATER  2
#define TER_INDESTRUCT  3
#define TER_WATER       4
#define TER_BASE        5
#define TER_EXPLOSIVE   6
#define TER_EXPLOSIVE2  7
#define TER_WATERFU     8
#define TER_WATERFR     9
#define TER_WATERFD     10
#define TER_WATERFL     11
#define TER_COMBUSTABLE 12
#define TER_COMBUSTABL2 13
#define TER_SNOW        14
#define TER_ICE         15
#define TER_BASEMAT     16
#define TER_TUNNEL      17
#define TER_WALKWAY     18
#define LAST_TER        18

/* Screen depth (bits per pixel) */
#define SCREEN_DEPTH 32

/* Win32 compatability */
#ifdef WIN32

/* Windows users like their games to start in fullscreen mode */
#undef FULLSCREEN
#define FULLSCREEN 1

#define M_PI		3.14159265358979323846  /* pi */
#define M_PI_2		1.57079632679489661923  /* pi/2 */
#define M_PI_4		0.78539816339744830962  /* pi/4 */

#define inline __inline

#define strcasecmp stricmp
#define strncasecmp strnicmp

#endif

/* Inlines instead of macros for type safety and to prevent against ++args */
/* Function names start with uppercase letter to prevent name clash */
/* with C99 round functions */
static inline int Round (double a)
{
    return (a < 0 ? a - 0.5 : a + 0.5);
}
static inline int Roundplus (double a)
{
    return (a < 0 ? 0 : a + 0.5);
}

/*static inline int LUOLA_MAX(int a, int b) { return (a<b ? b : a); }*/
static inline int LUOLA_MIN (int a, int b)
{
    return (a > b ? b : a);
}

#endif


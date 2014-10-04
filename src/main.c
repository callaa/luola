/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2001-2005 Calle Laakkonen
 *
 * File        : main.c
 * Description : Main module
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
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "fs.h"
#include "console.h"
#include "intro.h"
#include "game.h"
#include "levelfile.h"
#include "hotseat.h"
#include "level.h"
#include "player.h"
#include "particle.h"
#include "weapon.h"
#include "animation.h"
#include "special.h"
#include "critter.h"
#include "weather.h"
#include "ship.h"
#include "font.h"
#include "demo.h"
#include "startup.h"

#if HAVE_LIBSDL_MIXER
#include "audio.h"
#endif

static void show_version (void) {
    const SDL_version *sdld;
    sdld = SDL_Linked_Version ();
    printf ("%s version %s (Stable branch)\n", PACKAGE, VERSION);
    printf ("Compiled with SDL version %d.%d.%d\n", SDL_MAJOR_VERSION,
            SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    printf ("Dynamically linked SDL version is %d.%d.%d\n", sdld->major,
            sdld->minor, sdld->patch);
#ifndef WIN32
    printf ("Data directory is \"%s\"\n", PACKAGE_DATA_DIR);
#endif
    printf ("\nCompiled in features:\n");
#if HAVE_LIBSDL_MIXER
    printf ("SDL_Mixer (sounds): enabled\n");
#else
    printf ("SDL_Mixer (sounds): disabled\n");
#endif
#if HAVE_LIBSDL_GFX
    printf ("SDL_gfx (eyecandy): enabled\n");
#else
    printf ("SDL_gfx (eyecandy): disabled\n");
#endif
#if HAVE_LIBSDL_TTF
    printf ("SDL_ttf (Truetype fonts): enabled\n");
#else
    printf ("SDL_ttf (Truetype fonts): disabled\n");
#endif
    exit (0);
}

int main (int argc, char *argv[]) {
    int rval, r;
    /* Parse command line arguments */
    init_startup_options ();
    if (argc > 1) {
        for (r = 1; r < argc; r++) {
            if (strcmp (argv[r], "--help") == 0) {
                print_help ();
                return 0;
            } else if (strcmp (argv[r], "--version") == 0)
                show_version ();
#ifdef CHEAT_POSSIBLE
            else if (strcmp (argv[r], "--cheat") == 0) {
                cheat = 1;
                cheat1 = 0;
                printf ("Cheat er.. Debug mode enabled\n");
            }
#endif
            else if ((r = parse_argument (r, argc, argv)) == 0)
                return 0;
        }
    }
    /* Check if luola's home directory exists */
    check_homedir ();
    /* Seed the random number generator */
    srand (time (NULL));
    /* Initialize */
    init_sdl ();
    init_video ();
#if HAVE_LIBSDL_MIXER
    if (luola_options.sounds)
        init_audio ();
#endif
    if(init_font()) return 1;
    init_level();
    init_particles();
    if(init_intro()) return 1;
    if(init_game()) return 1;
    if(init_weapons()) return 1;
    if(init_players()) return 1;
    if(init_ships()) return 1;
    if(init_pilots()) return 1;
    if(init_hotseat()) return 1;
    if(init_specials()) return 1;
    if(init_critters()) return 1;
    init_weather();
    if (luola_options.mbg_anim)
        init_demos ();
    scan_levels(0);
    scan_levels(1);
    if (game_settings.levels == NULL)
        no_levels_found ();
    load_game_config ();

#if HAVE_LIBSDL_MIXER
    /* Set sound effect volume */
    audio_setsndvolume(game_settings.sound_vol);
#endif
    /* Enable key repeat (useful in menus. Will be disabled during game) */
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

    /* Enter game loop */
    while (1) {
        /* Intro screen */
        rval = game_menu_screen ();
        if (rval == INTRO_RVAL_EXIT)
            return 0;
        /* Play ! */
        if (rval == INTRO_RVAL_STARTGAME)
            hotseat_game ();
    }
    return 0;
}


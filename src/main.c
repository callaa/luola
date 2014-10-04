/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2001-2006 Calle Laakkonen
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

#include "fs.h"
#include "console.h"
#include "intro.h"
#include "game.h"
#include "levelfile.h"
#include "hotseat.h"
#include "level.h"
#include "player.h"
#include "projectile.h"
#include "animation.h"
#include "special.h"
#include "critter.h"
#include "ship.h"
#include "font.h"
#include "demo.h"
#include "selection.h"
#include "startup.h"
#include "audio.h"

/* Show version info */
static void show_version (void) {
    const SDL_version *sdld;
    sdld = SDL_Linked_Version ();
    printf ("%s version %s (Development branch)\n", PACKAGE, VERSION);
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

/* Do initializations that require datafiles */
static int load_data(void) {
    LDAT *graphics;
    graphics = ldat_open_file(getfullpath(DATA_DIRECTORY,"gfx.ldat"));
    if(graphics==NULL)
        return 1;

    init_critters(graphics);
    init_intro(graphics);
    init_game(graphics);
    init_selection(graphics);
    init_players(graphics);
    init_pilots(graphics);
    init_ships(graphics);
    init_specials(graphics);
    init_projectiles(graphics);

    ldat_free(graphics);

    return 0;
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
            else if ((r = parse_argument (r, argc, argv)) == 0)
                return 0;
        }
    }
    /* Check if luola's home directory exists and create it if necessary */
    check_homedir ();

    /* Seed the random number generator */
    srand (time (NULL));

    /* Initialize */
    init_sdl ();
    init_video ();

    if (luola_options.sounds)
        init_audio ();

    if(init_font()) return 1;
    if(load_data()) return 1;

    scan_levels(0);
    scan_levels(1);
    if (game_settings.levels == NULL)
        no_levels_found ();

    init_level();
    init_hotseat();
    if (luola_options.mbg_anim)
        init_demos ();

    /* Set sound effect volume */
    audio_setsndvolume(game_settings.sound_vol);

    /* Enable key repeat (useful in menus. Will be disabled during game) */
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

    /* Enter game loop */
    while (1) {
        /* Intro screen */
        rval = game_menu_screen ();
        if (rval == INTRO_RVAL_EXIT)
            break;
        /* Play ! */
        if (rval == INTRO_RVAL_STARTGAME)
            hotseat_game ();
    }
    return 0;
}


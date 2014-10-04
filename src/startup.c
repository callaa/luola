/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : startup.c
 * Description : Startup options
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "fs.h"
#include "parser.h"

#if HAVE_LIBSDL_MIXER
#include "SDL/SDL_mixer.h"
#else
#define MIX_DEFAULT_FREQUENCY 0
#endif

#include "startup.h"

/* The exported options structure */
StartupOptions luola_options;

/* Set default values */
void init_startup_options (void) {
    struct dllist *config;

    /* Built in values */
    luola_options.fullscreen = 0;
    luola_options.hidemouse = 1;
    luola_options.joystick = 1;
#if HAVE_LIBSDL_MIXER
    luola_options.sounds = 1;
#else
    luola_options.sounds = 0;
#endif
    luola_options.audio_rate = MIX_DEFAULT_FREQUENCY;
    luola_options.audio_chunks = 256;
    luola_options.sfont = 0;
    luola_options.mbg_anim = 1;
    luola_options.videomode = VID_640;

    /* Load configuration file (if exists) */
    config = read_config_file(getfullpath (HOME_DIRECTORY, "startup.cfg"),1);
    if(config) {
        struct ConfigBlock *block = config->data;
        struct Translate tr[] = {
            {"fullscreen", CFG_INT, &luola_options.fullscreen},
            {"hidemouse", CFG_INT, &luola_options.hidemouse},
            {"joystick", CFG_INT, &luola_options.joystick},
            {"sounds", CFG_INT, &luola_options.sounds},
            {"audiorate", CFG_INT, &luola_options.audio_rate},
            {"audiochunks", CFG_INT, &luola_options.audio_chunks},
            {"sfont", CFG_INT, &luola_options.sfont},
            {"mbg_anim", CFG_INT, &luola_options.mbg_anim},
            {"videomode", CFG_INT, &luola_options.videomode},
            {0,0,0}
        };
        translate_config(block->values,tr,0);

        dllist_free(config,free_config_file);
    }
}

void print_help (void) {
    printf ("Luola command line options:\n");
    printf ("  --fullscreen               Full screen mode\n");
    printf ("  --window                   Windowed mode\n");
    printf ("  --videomode <size>         Screen size (640x480,800x600,1024x768)\n");      
    printf ("  --hidemouse                Hide the mouse cursor when it's over the window\n");
    printf ("  --showmouse                Show the mouse cursor when it's over the window\n");
    printf ("  --pad                      Enable joypad\n");
    printf ("  --nopad                    Disable joypad\n");
#if HAVE_LIBSDL_MIXER
    printf ("  --sounds                   Enable sounds\n");
    printf ("  --nosounds                 Disable sounds\n");
#endif
    printf ("  --sfont                    Force SFont to be used\n");
    printf ("  --ttf                      Attempt to use SDL_ttf for font rendering\n");
    printf ("  --menu-animation           Enable menu background animation\n");
    printf ("  --no-menu-animation        Disable menu background animation\n");
    printf ("  --audiorate <rate>         Set audio sampling frequency\n");
    printf ("  --audiochunks <chunks>     Set audio chunks\n");
    printf ("  --help                     Show this message\n");
    printf ("  --version                  Show version information\n\n");
}

/* Parse a command argument */
int parse_argument (int r, int argc, char **argv) {
    if (strcmp (argv[r], "--fullscreen") == 0)
        luola_options.fullscreen = 1;
    else if (strcmp (argv[r], "--window") == 0)
        luola_options.fullscreen = 0;
    else if (strcmp (argv[r], "--hidemouse") == 0)
        luola_options.hidemouse = 1;
    else if (strcmp (argv[r], "--showmouse") == 0)
        luola_options.hidemouse = 0;
    else if (strcmp (argv[r], "--pad") == 0)
        luola_options.joystick = 1;
    else if (strcmp (argv[r], "--nopad") == 0)
        luola_options.joystick = 0;
#if HAVE_LIBSDL_MIXER
    else if (strcmp (argv[r], "--sounds") == 0)
        luola_options.sounds = 1;
    else if (strcmp (argv[r], "--nosounds") == 0)
        luola_options.sounds = 0;
#endif
    else if (strcmp (argv[r], "--sfont") == 0)
        luola_options.sfont = 1;
    else if (strcmp (argv[r], "--ttf") == 0)
        luola_options.sfont = 0;
    else if (strcmp (argv[r], "--menu-animation") == 0)
        luola_options.mbg_anim = 1;
    else if (strcmp (argv[r], "--no-menu-animation") == 0)
        luola_options.mbg_anim = 0;
    else if (strcmp (argv[r], "--audiorate") == 0) {
        if (r + 1 < argc) {
            r++;
            luola_options.audio_rate = atoi (argv[r]);
        } else {
            printf ("You did not specify the audio rate\n");
            return 0;
        }
    } else if (strcmp (argv[r], "--audiochunks") == 0) {
        if (r + 1 < argc) {
            r++;
            luola_options.audio_chunks = atoi (argv[r]);
        } else {
            printf ("You did not specify number of chunks\n");
            return 0;
        }
    } else if (strcmp (argv[r], "--videomode") == 0) {
        if (r + 1 < argc) {
            r++;
            if(strcmp(argv[r],"640x480")==0)
                luola_options.videomode=VID_640;
            else if(strcmp(argv[r],"800x600")==0)
                luola_options.videomode=VID_800;
            else if(strcmp(argv[r],"1024x768")==0)
                luola_options.videomode=VID_1024;
            else {
                printf ("Unknown video mode %s. Supported modes are 640x480,800x600 and 1024x768",argv[r]);
                return 0;
            }
        } else {
            printf ("You did not specify the video mode\n");
            return 0;
        }
    } else {
        printf ("Unrecognized argument: %s\n", argv[r]);
        return 0;
    }
    return r;
}

int save_startup_config (void) {
    FILE *fp;
    const char *filename;
    filename = getfullpath (HOME_DIRECTORY, "startup.cfg");
    fp = fopen (filename, "w");
    if (!fp)
        return 1;
    /* Save data */
    fprintf(fp,"fullscreen=%d\n",luola_options.fullscreen);
    fprintf(fp,"hidemouse=%d\n",luola_options.hidemouse);
    fprintf(fp,"joystick=%d\n",luola_options.joystick);
    fprintf(fp,"sounds=%d\n",luola_options.sounds);
    fprintf(fp, "audiorate=%d\n",luola_options.audio_rate);
    fprintf(fp, "audiochunks=%d\n",luola_options.audio_chunks);
    fprintf(fp, "sfont=%d\n",luola_options.sfont);
    fprintf(fp, "mbg_anim=%d\n",luola_options.mbg_anim);
    fprintf(fp, "videomode=%d\n",luola_options.videomode);
    /* Done. */
    fclose (fp);
    return 0;
}


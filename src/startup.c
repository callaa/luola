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

#include "defines.h"
#include "fs.h"
#include "stringutil.h"

#if HAVE_LIBSDL_MIXER
#include <SDL/SDL_mixer.h>
#else
#define MIX_DEFAULT_FREQUENCY 0
#define MIX_DEFAULT_CHANNELS 0
#endif

#include "startup.h"

/* The exported options structure */
StartupOptions luola_options;

/* Set default values */
void init_startup_options (void) {
    char tmps[256], *line = NULL;
    FILE *fp;
    /* Built in values */
    luola_options.fullscreen = FULLSCREEN;
    luola_options.hidemouse = HIDEMOUSE;
    luola_options.joystick = JOYSTICK;
#if HAVE_LIBSDL_MIXER
    luola_options.sounds = SOUNDS;
#else
    luola_options.sounds = 0;
#endif
    luola_options.audio_rate = MIX_DEFAULT_FREQUENCY;
    luola_options.audio_channels = MIX_DEFAULT_CHANNELS;
    luola_options.audio_chunks = 256;
    luola_options.sfont = 0;
    luola_options.mbg_anim = 1;
    luola_options.videomode = VID_640;

    /* Load configuration file (if exists) */
    fp = fopen (getfullpath (HOME_DIRECTORY, STARTUP_FILE), "r");
    if (!fp)
        return;                 /* No configuration file. We don't complain, just stick with the defaults */
    for (; fgets (tmps, sizeof (tmps) - 1, fp); free (line)) {
        line = strip_white_space (tmps);
        if (line == NULL)
            continue;
        if (strcmp (line, "fullscreen") == 0)
            luola_options.fullscreen = 1;
        else if (strcmp (line, "window") == 0)
            luola_options.fullscreen = 0;
        else if (strcmp (line, "hidemouse") == 0)
            luola_options.hidemouse = 1;
        else if (strcmp (line, "showmouse") == 0)
            luola_options.hidemouse = 0;
        else if (strcmp (line, "pad") == 0)
            luola_options.joystick = 1;
        else if (strcmp (line, "nopad") == 0)
            luola_options.joystick = 0;
#if HAVE_LIBSDL_MIXER
        else if (strcmp (line, "sounds") == 0)
            luola_options.sounds = 1;
        else if (strcmp (line, "nosounds") == 0)
            luola_options.sounds = 0;
#endif
        else if (strcmp (line, "sfont") == 0)
            luola_options.sfont = 1;
        else if (strcmp (line, "ttf") == 0)
            luola_options.sfont = 0;
        else if (strcmp (line, "menu-animation") == 0)
            luola_options.mbg_anim = 1;
        else if (strcmp (line, "no-menu-animation") == 0)
            luola_options.mbg_anim = 0;
        else if (strcmp (line, "audiorate") == 0) {
            fgets (tmps, sizeof (tmps) - 1, fp);
            luola_options.audio_rate = atoi (tmps);
        } else if (strcmp (line, "audiochannels") == 0) {
            fgets (tmps, sizeof (tmps) - 1, fp);
            luola_options.audio_channels = atoi (tmps);
        } else if (strcmp (line, "audiochunks") == 0) {
            fgets (tmps, sizeof (tmps) - 1, fp);
            luola_options.audio_chunks = atoi (tmps);
        } else if (strcmp (line, "videomode") == 0) {
            fgets (tmps, sizeof (tmps) - 1, fp);
            luola_options.videomode = atoi (tmps);
            if(luola_options.videomode<VID_640)
                luola_options.videomode=VID_640;
            else if(luola_options.videomode>VID_1024)
                luola_options.videomode=VID_1024;
        }
    }
    fclose (fp);
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
    printf ("  --audiochannels <channels> Set audio channels (1 or 2)\n");
    printf ("  --audiochunks <chunks>     Set audio chunks\n");
    printf ("  --help                     Show this message\n");
    printf ("  --version                  Show version information\n\n");
}

/* Parse a command tmps argument */
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
    } else if (strcmp (argv[r], "--audiochannels") == 0) {
        if (r + 1 < argc) {
            r++;
            luola_options.audio_channels = atoi (argv[r]);
        } else {
            printf ("You did not specify number of channels\n");
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

char save_startup_config (void) {
    FILE *fp;
    const char *filename;
    filename = getfullpath (HOME_DIRECTORY, STARTUP_FILE);
    fp = fopen (filename, "w");
    if (!fp)
        return 1;
    /* Save data */
    if (luola_options.fullscreen)
        fprintf (fp, "fullscreen\n");
    else
        fprintf (fp, "window\n");
    if (luola_options.hidemouse)
        fprintf (fp, "hidemouse\n");
    else
        fprintf (fp, "showmouse\n");
    if (luola_options.joystick)
        fprintf (fp, "pad\n");
    else
        fprintf (fp, "nopad\n");
    if (luola_options.sounds)
        fprintf (fp, "sounds\n");
    else
        fprintf (fp, "nosounds\n");
    fprintf (fp, "audiorate\n%d\n", luola_options.audio_rate);
    fprintf (fp, "audiochannels\n%d\n", luola_options.audio_channels);
    fprintf (fp, "audiochunks\n%d\n", luola_options.audio_chunks);
    if (luola_options.sfont)
        fprintf (fp, "sfont\n");
    else
        fprintf (fp, "ttf\n");
    if (luola_options.mbg_anim)
        fprintf (fp, "menu-animation\n");
    else
        fprintf (fp, "no-menu-animation\n");
    fprintf(fp, "videomode\n%d\n",luola_options.videomode);
    /* Done. */
    fclose (fp);
    return 0;
}


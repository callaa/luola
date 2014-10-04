/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : startup.h
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

#ifndef STARTUP_H
#define STARTUP_H

typedef enum {VID_640,VID_800,VID_1024} Videomode;

/* All startup options */
typedef struct {
    int fullscreen;
    int hidemouse;
    int joystick;
    int sounds;
    int audio_rate, audio_chunks;
    int sfont;
    int mbg_anim;
    Videomode videomode;
} StartupOptions;

/* The structure used by everyone */
extern StartupOptions luola_options;

/* Initialize the startup options structure.		*/
/* Sets the builtin values and tries to see if a 	*/
/* configuration file is available.			*/
extern void init_startup_options (void);

/* Print the list of accepted command line arguments	*/
extern void print_help (void);

/* Parse a command line argument */
extern int parse_argument (int r, int argc, char **argv);

/* Save the startup configuration to file.		*/
/* Returns non-zero on error.				*/
extern int save_startup_config (void);

#endif

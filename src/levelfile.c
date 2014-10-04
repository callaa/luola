/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : levelfile.c
 * Description : Level loading
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

#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>

#include "defines.h"
#include "console.h"
#include "levelfile.h"
#include "fs.h"
#include "list.h"
#include "game.h"
#include "font.h"

/* Get the type of the level file */
static LevelFormat get_level_format (const char *filename) {
    const char *ptr;
    ptr = strrchr(filename,'.');
    if(ptr && strcasecmp(ptr,".lev")==0) {
        if(filename<ptr-8 && strcasecmp(ptr-8,".compact.lev")==0) {
            return LEV_COMPACT;
        }
        return LEV_NORMAL;
    }
    return LEV_UNKNOWN;
}

/* Load level settings */
static int level_load_settings (struct LevelFile *level) {
    if (level->type==LEV_COMPACT) {
        LDAT *ldat;
        ldat =
            ldat_open_file (getfullpath
                            (level->
                             user ? USERLEVEL_DIRECTORY :
                             LEVEL_DIRECTORY, level->filename));
        if(!ldat) return 1;
        level->settings =
        load_level_config_rw (ldat_get_item
                             (ldat, "SETTINGS", 0),
                              ldat_get_item_length (ldat,
                              "SETTINGS", 0));
        ldat_free (ldat);
    } else {
        level->settings =
            load_level_config (getfullpath(level-> user ?
                        USERLEVEL_DIRECTORY : LEVEL_DIRECTORY,
                        level->filename));
    }
    if(level->settings==NULL)
        return 1;

    if (level->settings->mainblock) {
        if (level->settings->mainblock->name==NULL) {
            level->settings->mainblock->name = strdup(level->filename);
            fprintf(stderr,"Warning! Level configuration file \"%s\" has no level name!\n",level->filename);
        }
        if(level->type==LEV_NORMAL
            && level->settings->mainblock->artwork==NULL
            && level->settings->mainblock->collmap==NULL) {
            fprintf(stderr,"Warning! Level configuration file \"%s\" has no artwork or collisionmap filename!\n",level->filename);
            return 1;
        }
    } else {
        fprintf(stderr,
            "Warning! Level configuration file \"%s\" doesn't contain a mainblock!\n",
             level->filename);
        return 1;
    }
    return 0;
}

/* Scan the level directory and make a list of levels */
int scan_levels (int user) {
    struct LevelFile *newentry;
    struct dirent *next;
    const char *dirname;
    LevelFormat format;
    DIR *reading;

    /* Which directory to read */
    if (user)
        dirname = getfullpath (USERLEVEL_DIRECTORY, "");
    else
        dirname = getfullpath (LEVEL_DIRECTORY, "");
    /* Open the directory */
    reading = opendir (dirname);
    if (!reading) {
        printf ("Error! Cannot open directory \"%s\" !\n", dirname);
        return 1;
    }
    /* Loop thru the directory */
    while ((next = readdir (reading)) != NULL) {
        /* Check level type and add it */
        format = get_level_format (next->d_name);
        if (format == LEV_UNKNOWN) continue;

        /*newentry = newlevel (level_name(next->d_name));*/
        newentry = malloc(sizeof(struct LevelFile));
        newentry->filename = strdup(next->d_name);
        newentry->type = format;
        newentry->user = user;
        if(level_load_settings(newentry)) {
            free(newentry->filename);
            free(newentry);
        } else {
            game_settings.levels = dllist_append(game_settings.levels,newentry);
            game_settings.levelcount++;
        }
    }
    closedir(reading);
    /* Set first and last levels */
    if(game_settings.levels) {
        while(game_settings.levels->next)
            game_settings.levels=game_settings.levels->next;
        game_settings.last_level = game_settings.levels;
        while(game_settings.levels->prev)
            game_settings.levels=game_settings.levels->prev;
        game_settings.first_level = game_settings.levels;
    }
    return 0;
}

/* Loads the level artwork file from a file */
/* The surface returned is in the same format as the screen */
SDL_Surface *load_level_art (struct LevelFile *level) {
    SDL_Surface *art, *tmpart;
    const char *filename;
    if (level->type == LEV_NORMAL) {  /* Level files are seperate */
        if (level->settings->mainblock->artwork)
            filename = level->settings->mainblock->artwork;
        else
            filename = level->settings->mainblock->collmap;

        tmpart =
            load_image ((level->user) ? USERLEVEL_DIRECTORY : LEVEL_DIRECTORY,
                        filename, 0, 0);
    } else { /* Level files are stored in an LDAT file */
        LDAT *lf;
        SDL_RWops *rw;
        lf = ldat_open_file (getfullpath
                             (level->
                              user ? USERLEVEL_DIRECTORY : LEVEL_DIRECTORY,
                              level->filename));
        if(!lf) return NULL;
        if (ldat_find_item (lf, "ARTWORK", 0))
            rw = ldat_get_item (lf, "ARTWORK", 0);
        else /* If the artwork is not present, get the collisionmap */
            rw = ldat_get_item (lf, "COLLISION", 0);
        tmpart = load_image_rw (rw, 0, 0);
        ldat_free (lf);
    }
    if (level->settings->mainblock->aspect != 1.0 ||
            level->settings->mainblock->zoom != 1.0) {
        art = zoom_surface(tmpart, level->settings->mainblock->aspect,
                    level->settings->mainblock->zoom);
        SDL_FreeSurface (tmpart);
    } else {
        art = tmpart;
    }
    return art;
}

/* Loads the level collisionmap from a file */
/* The surface is returned as it is, that is 8bit */
/* If the image is not 8 bit, NULL is returned */
SDL_Surface *load_level_coll (struct LevelFile * level) {
    SDL_Surface *coll, *tmpcoll;
    const char *filename;
    if (level->type == LEV_NORMAL) {  /* Level files are seperate */
        if (level->settings->mainblock->collmap)
            filename = level->settings->mainblock->collmap;
        else
            filename = level->settings->mainblock->artwork;
        tmpcoll =
            load_image ((level->user) ? USERLEVEL_DIRECTORY : LEVEL_DIRECTORY,
                        filename, 0, -1);
    } else {                    /* Level files are stored in an LDAT file */
        LDAT *lf;
        SDL_RWops *rw;
        filename=level->filename;
        lf = ldat_open_file (getfullpath
                             (level->
                              user ? USERLEVEL_DIRECTORY : LEVEL_DIRECTORY,
                              filename));
        if(!lf) return NULL;
        if (ldat_find_item (lf, "COLLISION", 0))
            rw = ldat_get_item (lf, "COLLISION", 0);
        else /* If the collisionmap is not present, get the artwork */
            rw = ldat_get_item (lf, "ARTWORK", 0);
        tmpcoll = load_image_rw (rw, 0, -1);
        ldat_free (lf);
    }
    if(tmpcoll->format->palette==NULL) {
        printf("Error! Level collisionmap image \"%s\" doesn't have a palette!\n",filename);
        SDL_FreeSurface(tmpcoll);
        return NULL;
    }
    if (level->settings->mainblock->aspect != 1 ||
            level->settings->mainblock->zoom != 1) {
        coll = zoom_surface(tmpcoll,level->settings->mainblock->aspect,
                     level->settings->mainblock->zoom);
        SDL_FreeSurface (tmpcoll);
    } else
        coll = tmpcoll;
    return coll;
}


#ifdef WIN32
#undef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "data"
#endif

/* Display this message when no levels are found */
void no_levels_found (void)
{
    SDL_Rect r1, r2;
    SDL_Event event;
    char tmps[256];
    r1.x = 10;
    r1.w = screen->w - 20;
    r1.y = 10;
    r1.h = screen->h - 20;
    r2.x = r1.x + 2;
    r2.y = r1.y + 2;
    r2.w = r1.w - 4;
    r2.h = r1.h - 4;
    SDL_FillRect (screen, &r1, SDL_MapRGB (screen->format, 255, 0, 0));
    SDL_FillRect (screen, &r2, SDL_MapRGB (screen->format, 0, 0, 0));
    centered_string (screen, Bigfont, r2.y + 10, "No levels found",
                     font_color_red);
    centered_string (screen, Bigfont, r2.y + 50,
                     "No levels were found in the data directory",
                     font_color_white);
    sprintf (tmps, "%s/levels/", PACKAGE_DATA_DIR);
    centered_string (screen, Bigfont, r2.y + 80, tmps, font_color_white);
    centered_string (screen, Bigfont, r2.y + 120,
                     "Please download the level pack from the Luola homepage",
                     font_color_white);
    centered_string (screen, Bigfont, r2.y + 150,
                     "http://www.luolamies.org/software/luola/",
                     font_color_white);
    centered_string (screen, Bigfont, r2.y + 180,
                     "and unpack it into the level directory",
                     font_color_white);
    centered_string (screen, Bigfont, r2.y + r2.h - 50, "Press enter to exit",
                     font_color_red);
    SDL_UpdateRect (screen, r1.x, r1.y, r1.w, r1.h);
    while (1) {
        SDL_WaitEvent (&event);
        if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_RETURN ||
                    event.key.keysym.sym == SDLK_KP_ENTER))
            break;
        if (event.type == SDL_JOYBUTTONDOWN)
            break;
    }
    exit (0);
}


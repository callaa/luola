/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
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

#include "SDL.h"

#include "console.h"
#include "levelfile.h"
#include "fs.h"
#include "list.h"
#include "game.h"
#include "font.h"

/* Open level for reading */
int open_level(struct LevelFile *level) {
    if(level->ldat) {
        fprintf(stderr,"open_level: level file %s already opened!\n",
                level->filename);
        return 0;
    }

    if(level->type == LEV_COMPACT) {
        level->ldat = ldat_open_file (level->filename);
        if(level->ldat==NULL)
            return 1;
    }
    return 0;
}

/* Close level */
void close_level(struct LevelFile *level) {
    if(level->type==LEV_COMPACT) {
        if(level->ldat==NULL) {
            fprintf(stderr,"close_level: level file %s already closed!\n",
                    level->filename);
        } else {
            ldat_free(level->ldat);
            level->ldat = NULL;
        }
    }
}

/* Load level settings and thumbnail */
static int level_load_settings (struct LevelFile *level) {
    if (level->ldat) {
        const char *config;
        /* TODO: Use only CONFIG in the next stable release */
        if(ldat_find_item(level->ldat,"CONFIG",level->index))
            config = "CONFIG";
        else
            config = "SOURCE";
        level->settings =
        load_level_config_rw (ldat_get_item
                             (level->ldat, config, level->index),
                              ldat_get_item_length (level->ldat,
                              config, level->index),
                              level->filename);
        if(level->settings)
            level->settings->thumbnail =
                load_image_ldat(level->ldat,1,T_OPAQUE,"THUMBNAIL",level->index);
    } else {
        level->settings =
            load_level_config (level->filename);
        if(level->settings && level->settings->mainblock.thumbnail) {
            level->settings->thumbnail = load_image(samepath(level->filename,
                        level->settings->mainblock.thumbnail),1,T_OPAQUE);
        }
    }
    if(level->settings==NULL)
        return 1;

    if (level->settings->mainblock.name==NULL) {
        level->settings->mainblock.name = strdup(level->filename);
        fprintf(stderr,"Warning! Level configuration file \"%s\" has no level name!\n",level->filename);
    }
    if(level->type==LEV_NORMAL
        && level->settings->mainblock.collmap==NULL) {
        fprintf(stderr,"Warning! Level configuration file \"%s\" has no collisionmap filename!\n",level->filename);
        return 1;
    }
    return 0;
}

/* Get the type of the level file */
static LevelFormat get_level_format (const char *filename) {
    const char *ptr;
    ptr = strrchr(filename,'.');
    if(ptr && strcasecmp(ptr,".lev")==0) {
        if(is_ldat(filename))
            return LEV_COMPACT;
        return LEV_NORMAL;
    }
    return LEV_UNKNOWN;
}

/* Add a normal level */
static void add_level(const char *filename) {
    struct LevelFile *newentry = malloc(sizeof(struct LevelFile));
    newentry->filename = strdup(filename);
    newentry->ldat = NULL;
    newentry->type = LEV_NORMAL;
    if(level_load_settings(newentry)==0) {
        game_settings.levels = dllist_append(game_settings.levels,newentry);
    } else {
        free(newentry->filename);
        free(newentry);
    }
}

/* Add a compact level file. Might contain multiple levels */
static void add_compact_level(const char *filename) {
    LDAT *ldat;
    int count,r;
    ldat = ldat_open_file(filename);
    if(!ldat) return;
    count = ldat_get_item_count(ldat,"CONFIG");
    /* TODO: Use only CONFIG in the next stable release */
    if(count==0)
        count = ldat_get_item_count(ldat,"SOURCE");

    for(r=0;r<count;r++) {
        struct LevelFile *newentry = malloc(sizeof(struct LevelFile));
        newentry->filename = strdup(filename);
        newentry->ldat = ldat;
        newentry->index= r;
        newentry->type = LEV_COMPACT;
        if(level_load_settings(newentry)==0) {
            game_settings.levels = dllist_append(game_settings.levels,newentry);
            newentry->ldat = NULL;
        } else {
            free(newentry->filename);
            free(newentry);
        }
    }

    ldat_free(ldat);
}

/* Scan the level directory and make a list of levels */
int scan_levels (int user) {
    struct dirent *next;
    char fullpath[PATH_MAX];
    int pathlen;
    LevelFormat format;
    DIR *reading;

    /* Which directory to read */
    if (user)
        strcpy(fullpath, getfullpath (USERLEVEL_DIRECTORY, ""));
    else
        strcpy(fullpath, getfullpath (LEVEL_DIRECTORY, ""));
    /* Open the directory */
    reading = opendir (fullpath);
    if (!reading) {
        perror(fullpath);
        return 1;
    }
    pathlen = strlen(fullpath);
    /* Loop thru the directory */
    while ((next = readdir (reading)) != NULL) {
        /* Check level type and add it */
        strcpy(fullpath+pathlen,next->d_name);
        format = get_level_format (fullpath);
        switch(format) {
            case LEV_UNKNOWN: continue;
            case LEV_NORMAL: add_level(fullpath); break;
            case LEV_COMPACT: add_compact_level(fullpath); break;
        }
    }
    closedir(reading);
    return 0;
}

/* Loads the level artwork file from a file */
/* The surface returned is in the same format as the screen */
SDL_Surface *load_level_art (struct LevelFile *level) {
    SDL_Surface *art, *tmpart;
    if (level->ldat==NULL) {  /* Level files are seperate */
        const char *filename;
        if (level->settings->mainblock.artwork)
            filename = level->settings->mainblock.artwork;
        else /* Use collisionmap if artwork is not available */
            filename = level->settings->mainblock.collmap;

        tmpart = load_image (samepath(level->filename,filename), 0, T_OPAQUE);
    } else { /* Level files are stored in an LDAT file */
        SDL_RWops *rw;
        if (ldat_find_item (level->ldat, "ARTWORK", level->index))
            rw = ldat_get_item (level->ldat, "ARTWORK", level->index);
        else /* If the artwork is not present, get the collisionmap */
            rw = ldat_get_item (level->ldat, "COLLISION", level->index);
        tmpart = load_image_rw (rw, 0, T_OPAQUE);
    }
    if (level->settings->mainblock.aspect != 1.0 ||
            level->settings->mainblock.zoom != 1.0) {
        art = zoom_surface(tmpart, level->settings->mainblock.aspect,
                    level->settings->mainblock.zoom);
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
    if (level->ldat==NULL) {  /* Level files are seperate */
        const char *filename;
        if (level->settings->mainblock.collmap) {
            filename = level->settings->mainblock.collmap;
        } else {
            fprintf(stderr,"%s: no collisionmap present\n",level->filename);
            return NULL;
        }
        tmpcoll = load_image (samepath(level->filename,filename), 0, T_NONE);
    } else {                    /* Level files are stored in an LDAT file */
        SDL_RWops *rw;
        rw = ldat_get_item (level->ldat, "COLLISION", level->index);
        if(!rw) {
            fprintf(stderr,"%s: no collisionmap present\n",level->filename);
            return NULL;
        }
        tmpcoll = load_image_rw (rw, 0, T_NONE);
    }
    if(tmpcoll->format->palette==NULL) {
        fprintf(stderr, "%s: collisionmap doesn't have a palette\n",level->filename);
        SDL_FreeSurface(tmpcoll);
        return NULL;
    }
    if (level->settings->mainblock.aspect != 1.0 ||
            level->settings->mainblock.zoom != 1.0) {
        coll = zoom_surface(tmpcoll,level->settings->mainblock.aspect,
                     level->settings->mainblock.zoom);
        SDL_FreeSurface (tmpcoll);
    } else
        coll = tmpcoll;
    return coll;
}


#ifdef WIN32
#undef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "data"
#define HOMELEVELS "home/levels/"
#else
#define HOMELEVELS "~/.luola/levels"
#endif

/* Display this message when no levels are found */
void no_levels_found (void)
{
    const char *msg[] = {
        "No levels were found in the data directory",
        PACKAGE_DATA_DIR "/levels/",
        "or",
        HOMELEVELS,
        "Please download a level pack from the Luola homepage",
        "http://www.luolamies.org/software/luola/",
        "and unpack it into the level directory.",NULL};

    error_screen("No levels found","Press enter to exit",msg);
    exit (1);
}


/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : fs.c
 * Description : File system calls and filepath abstraction
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
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "SDL.h"
#include "SDL_image.h"

#include "console.h"
#include "fs.h"
#include "lcmap.h"

/* Paths */
#ifndef WIN32
#define LEVEL_PATH "/levels/"
#define FONT_PATH "/font/"
#else
#undef PACKAGE_DATA_DIR
#define DATA_PATH "data/"
#define LEVEL_PATH "data/levels/"
#define FONT_PATH "data/font/"
#endif

#define HOME_PATH "luola/"

/* Get the full path of a filename */
const char *getfullpath (DataDir dir,const char *filename)
{
    /* static buffer for the filename */
    static char fullpath[1024];
    int datadirlen = 0;
#ifdef PACKAGE_DATA_DIR
    datadirlen = strlen (PACKAGE_DATA_DIR);
#endif
    switch (dir) {
    case DATA_DIRECTORY:
        if ((strlen (filename) + datadirlen + 1) >
            sizeof (fullpath)) {
            fprintf (stderr,"Error! full pathname too long");
            return NULL;
        }
#ifdef PACKAGE_DATA_DIR
        strcpy (fullpath, PACKAGE_DATA_DIR "/");
#else
        strcpy (fullpath, DATA_PATH);
#endif
        break;
    case LEVEL_DIRECTORY:
        if ((strlen (filename) + datadirlen + strlen (LEVEL_PATH) + 1) >
            sizeof (fullpath)) {
            fprintf (stderr,"Error! full pathname too long");
            return NULL;
        }
#ifdef PACKAGE_DATA_DIR
        strcpy (fullpath, PACKAGE_DATA_DIR);
        strcat (fullpath, LEVEL_PATH);
#else
        strcpy (fullpath, LEVEL_PATH);
#endif
        break;
    case FONT_DIRECTORY:
        if ((strlen (filename) + datadirlen + strlen (LEVEL_PATH) + 1) >
            sizeof (fullpath)) {
            fprintf (stderr,"Error! full pathname too long");
            return NULL;
        }
#ifdef PACKAGE_DATA_DIR
        strcpy (fullpath, PACKAGE_DATA_DIR);
        strcat (fullpath, FONT_PATH);
#else
        strcpy (fullpath, FONT_PATH);
#endif
        break;
    case USERLEVEL_DIRECTORY:
    case HOME_DIRECTORY:{
#if WIN32
            if (dir == USERLEVEL_DIRECTORY)
                strcpy (fullpath, "home/levels/");
            else
                strcpy (fullpath, "home/");
            break;
#else
            char *home_path, *levelpath;
            if (dir == USERLEVEL_DIRECTORY)
                levelpath = "levels/";
            else
                levelpath = "";
            home_path = getenv ("HOME");
            if (home_path == NULL) {    /* HOME environment variable not set */
                printf ("Error ! Unable to get home directory ($HOME)\n");
                exit (1);
            }
            /* Enough space for a slash (/) and the appended file name */
            if ((strlen (filename) + strlen (home_path) + strlen (HOME_PATH) +
                 strlen (levelpath) + 3) > sizeof (fullpath)) {
                printf ("Error ! full pathname too long");
                return NULL;
            }
            if (dir == USERLEVEL_DIRECTORY)
                sprintf (fullpath, "%s/.%s%s", home_path, HOME_PATH,
                         levelpath);
            else
                sprintf (fullpath, "%s/.%s", home_path, HOME_PATH);
            break;
#endif
        }
    }
    strcat (fullpath, filename);
    return fullpath;
}

/* Return file2 with file1's path */
const char *samepath(const char *file1, const char *file2)
{
    static char fullpath[PATH_MAX];
    char *ptr;
    ptr = strrchr(file1,'/')+1;
    strncpy(fullpath,file1,ptr-file1);
    strcpy(fullpath+(ptr-file1),file2);
    return fullpath;
}

/* Create the home directory if it doesn't exist */
void check_homedir (void)
{
    DIR *dir;
    const char *path;
    path = getfullpath (HOME_DIRECTORY, "");
    dir = opendir (path);
    if (dir) {
        closedir (dir);         /* We're OK */
        path = getfullpath (USERLEVEL_DIRECTORY, "");
        dir = opendir (path);
        if (!dir) {
#ifndef WIN32
            printf ("User level subdirectory doesn't exist. Creating...\n");
            if (mkdir (path, 0777)) {
                perror (path);
            }
#endif
        } else {
            closedir(dir);
        }
        return;
    }
#ifndef WIN32
    printf ("Luola home directory \"%s\" doesn't exist. Creating...\n", path);
    if (mkdir (path, 0777)) {
        perror (path);
        exit (1);
    }
    path = getfullpath (USERLEVEL_DIRECTORY, "");
    if (mkdir (path, 0777)) {
        perror (path);
    }
#endif
}

/* Load an imagefile */
SDL_Surface *load_image (const char *filename, int allownull,
                         Transparency transparency)
{
    SDL_Surface *image;
    SDL_RWops *rw;
    rw = SDL_RWFromFile (filename, "rb");
    if (rw == NULL) {
        if (allownull == 0) {
            fprintf (stderr,"Unable to open image \"%s\"\n%s:\n",
                    filename, SDL_GetError ());
            exit (1);
        }
        return NULL;
    }
    image = load_image_rw (rw, allownull, transparency);
    SDL_FreeRW (rw);
    return image;
}

/* This function loads an image not supported by SDL_Image */
/* Returns NULL if Luola doesn't recognize the file type */
/* Currently, only LMAP is supported */
static SDL_Surface *load_luola_image_rw (SDL_RWops * rw)
{
    SDL_Surface *image = NULL;
    char magic[5];
    int start;
    start = SDL_RWtell (rw);
    if (SDL_RWread (rw, magic, 1, 5)) {
        SDL_RWseek (rw, start, SEEK_SET);
        if (strncmp (magic, "LCMAP", 5) == 0) { /* image is an LCMAP */
            image = lcmap_to_surface_rw (rw);
        }
    }
    return image;
}

/* Load an image from SDL_RWops */
SDL_Surface *load_image_rw (SDL_RWops * rw, int allownull,
                            Transparency transparency)
{
    SDL_Surface *tmp, *conv = NULL;

    /* Check if Luola can load the image itself (LCMAP ?) */
    tmp = load_luola_image_rw (rw);
    if (tmp == NULL) /* If not, then use SDL_image */
        tmp = IMG_Load_RW (rw, 0);
    if (tmp == NULL) {
        if (allownull == 0) {
            fprintf (stderr,"Unable to load image from SDL_RWops\n%s\n",
                    SDL_GetError ());
            exit (1);
        }
        return NULL;
    }

    /* Set transparency options */
    if (transparency == T_COLORKEY) {
        Uint32 ck;
        switch (tmp->format->BytesPerPixel) {
        case 1:
            ck = *((Uint8 *) tmp->pixels);
            break;
        case 2:
            ck = *((Uint16 *) tmp->pixels);
            break;
        case 3:{
                Uint8 r, g, b;
                r = *(((Uint8 *) tmp->pixels) + tmp->format->Rshift / 8);
                g = *(((Uint8 *) tmp->pixels) + tmp->format->Gshift / 8);
                b = *(((Uint8 *) tmp->pixels) + tmp->format->Bshift / 8);
                ck = SDL_MapRGB (tmp->format, r, g, b);
            }
            break;
        case 4:
            ck = *((Uint32 *) tmp->pixels);
            break;
        default:
            ck = 0;
        }
        SDL_SetColorKey (tmp, SDL_SRCCOLORKEY | SDL_RLEACCEL, ck);
    }
    if (transparency == T_ALPHA)
        conv = SDL_DisplayFormatAlpha (tmp);
    else if(transparency != T_NONE)
        conv = SDL_DisplayFormat (tmp);

    if (conv == NULL)
        conv = tmp;
    else
        SDL_FreeSurface (tmp);

    if (tmp == NULL) {
        fprintf (stderr,"Could not convert image to screen format! %s\n",
                SDL_GetError ());
        exit (1);
    }
    return conv;
}

/* Load an array of images with sequential index numbers from an LDAT file */
/* count is set to the number of images read. */
SDL_Surface **load_image_array (LDAT * datafile, int allownull,
                                Transparency transparency, const char *id,
                                int *count)
{
    SDL_Surface **surfaces;
    SDL_RWops *data;
    int r;
    *count = ldat_get_item_count(datafile, id);
    if(*count==0) {
        if(allownull==0) {
            fprintf (stderr,"No %s images in LDAT file\n",id);
            exit(1);
        }
        return NULL;
    }
    surfaces = malloc (sizeof (SDL_Surface *) * (*count));
    for (r=0;r<*count;r++) {
        data = ldat_get_item (datafile, id, r);
        surfaces[r] = load_image_rw (data, allownull, transparency);
    }
    return surfaces;
}

/* This is a convenience function to load an image from a datafile */
SDL_Surface *load_image_ldat (LDAT * datafile, int allownull,
                              Transparency transparency, const char *id,
                              int index)
{
    SDL_RWops *data;
    data = ldat_get_item (datafile, id, index);
    if (data == NULL) {
        if (allownull)
            return NULL;
        exit (1);
    }
    return load_image_rw (data, allownull, transparency);
}

/* Take a screenshot and save it in the home directory */
void screenshot (void)
{
    const char *filename;
    char tmp[64];
    sprintf (tmp, "luola_%d.bmp", SDL_GetTicks ());
    filename = getfullpath (HOME_DIRECTORY, tmp);
    if (SDL_SaveBMP (screen, filename)) {
        fprintf (stderr,"Error saving screenshot to %s: %s\n", filename,
                SDL_GetError());
    }
}

/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003-2005 Calle Laakkonen
 *
 * File        : lconf.c
 * Description : Level configuration file parsing
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

#include "SDL_image.h"
#include "SDL_endian.h"

#include "stringutil.h"
#include "lconf.h"
#include "sbmp.h"

/* Internally used functions */
static char *read_line (FILE * fp);
static LSB_Main *readMainBlock (FILE * fp);
static LSB_Override *readOverrideBlock (FILE * fp);
static LSB_Objects *readObjectsBlock (FILE * fp);
static LSB_Palette *readPaletteBlock (FILE * fp);
static SDL_Surface *readIconBlock (FILE * fp);
static int readMainBlockRW (LevelSettings * settings, SDL_RWops * rw,
                            int len);
static int readOverrideBlockRW (LevelSettings * settings, SDL_RWops * rw,
                                int len);
static int readObjectsBlockRW (LevelSettings * settings, SDL_RWops * rw,
                               int len);
static int readPaletteBlockRW (LevelSettings * settings, SDL_RWops * rw,
                               int len);
static int readIconRW (LevelSettings * info, SDL_RWops * rw, int len);

/* Load the configuration file from file */
LevelSettings *load_level_config (const char *filename)
{
    FILE *fp;
    char *line;
    LevelSettings *settings;
    /* Initialize */
    settings = malloc (sizeof (LevelSettings));
    memset (settings, 0, sizeof (LevelSettings));
    /* Open file */
    fp = fopen (filename, "r");
    if (!fp) {
        printf ("Error! Could not open configuration file \"%s\"\n",
                filename);
        exit (1);
    }
    /* Read */
    while ((line = read_line (fp))) {
        if (line[0] == '\0')
            continue;
        if (strcmp (line, "[main]") == 0)
            settings->mainblock = readMainBlock (fp);
        else if (strcmp (line, "[override]") == 0)
            settings->override = readOverrideBlock (fp);
        else if (strcmp (line, "[objects]") == 0)
            settings->objects = readObjectsBlock (fp);
        else if (strcmp (line, "[palette]") == 0)
            settings->palette = readPaletteBlock (fp);
        else if (strcmp (line, "[icon]") == 0)
            settings->icon = readIconBlock (fp);
        free (line);
    }
    /* Finished */
    fclose (fp);
    return settings;
}

/* Load the (binary) configuration file from RWops */
LevelSettings *load_level_config_rw (SDL_RWops * rw, int len)
{
    LevelSettings *settings;
    int read = 0;
    Uint16 l;
    Uint8 type,ver;
    /* Initialize */
    settings = malloc (sizeof (LevelSettings));
    memset (settings, 0, sizeof (LevelSettings));
    /* Read */
    while (read < len) {
        read += SDL_RWread (rw, &l, 2, 1) * 2;
        read += SDL_RWread (rw, &type, 1, 1);
        l = SDL_SwapLE16(l);
        switch (type) {
        case 0x01:
            read += SDL_RWread(rw,&ver,1,1);
            if(ver!=LCONF_REVISION) {
                fprintf(stderr,"Error: incompatible level file\n");
                free(settings);
                return NULL;
            }
            break;
        case 0x02:
            read += readMainBlockRW (settings, rw, l);
            break;
        case 0x03:
            read += readOverrideBlockRW (settings, rw, l);
            break;
        case 0x04:
            read += readObjectsBlockRW (settings, rw, l);
            break;
        case 0x05:
            read += readPaletteBlockRW (settings, rw, l);
            break;
        case 0x06:
            read += readIconRW(settings,rw,l);
            break;
        default:
            fprintf(stderr,
                "Warning: Unrecognized toplevel block 0x%x ! (skipping %d bytes...)\n",
                 type, l);
            SDL_RWseek (rw, l, SEEK_CUR);
            read += l;
            break;
        }
    }
    if (read > len)
        fprintf(stderr,
            "Warning: read %d bytes past the limit in load_level_config_rw()!\n",
             read - len);
    /* Finished */
    return settings;
}

/* Read the icon from a textmode level settings file */
static SDL_Surface *readIconBlock (FILE * fp)
{
    SDL_Surface *icon;
    SDL_RWops *rw;
    rw = SDL_RWFromFP (fp, 0);
    icon = IMG_LoadTyped_RW (rw, 0, "XPM");
    if (icon == NULL)
        fprintf (stderr,"Error occured while tried to read level icon:\n%s\n",
                SDL_GetError ());
    SDL_FreeRW (rw);
    return icon;
}

/* Read the icon from a binary level settings file */
static int readIconRW (LevelSettings *settings, SDL_RWops * rw, int len)
{
    Uint32 colorkey;
    Uint8 hascolorkey;
    Uint8 *data;
    int read;
    data = malloc (len);
    read = SDL_RWread (rw, &hascolorkey, 1, 1);
    read += SDL_RWread (rw, &colorkey, 4, 1) * 4;
    read += SDL_RWread (rw, data, 1, len);
    len -= read;
    if (len)
        SDL_RWseek (rw, len, SEEK_CUR);
    settings->icon = sbmp_to_surface (data);
    if (hascolorkey)
        SDL_SetColorKey (settings->icon, SDL_SRCCOLORKEY, SDL_SwapLE32(colorkey));
    free (data);
    return read + len;
}

/* Read the main block */
static LSB_Main *readMainBlock (FILE * fp)
{
    LSB_Main *mblock;
    char *left, *right;
    char *line;
    /* Initialize main block */
    mblock = malloc (sizeof (LSB_Main));
    memset (mblock, 0, sizeof (LSB_Main));
    mblock->aspect = 1.0;
    mblock->zoom = 1.0;
    /* Read main block values */
    while ((line = read_line (fp))) {
        if (line[0] == '\0')
            continue;
        if (strcmp (line, "[end]") == 0) {
            free (line);
            break;
        }
        split_string (line, '=', &left, &right);
        if (left == NULL || right == NULL) {
            fprintf (stderr,"Warning: Malformed line:\n \"%s\"\n", line);
            continue;
        }
        if (strcmp (left, "collisionmap") == 0)
            mblock->collmap = right;
        else if (strcmp (left, "artwork") == 0)
            mblock->artwork = right;
        else if (strcmp (left, "name") == 0)
            mblock->name = right;
        else if (strcmp (left, "aspect") == 0) {
            mblock->aspect= atof (right);
            free (right);
        } else if (strcmp (left, "zoom") == 0) {
            mblock->zoom = atof (right);
            free (right);
        } else if (strcmp (left, "music") == 0) {
            LevelBgMusic *mentry = NULL;
            mentry = malloc (sizeof (LevelBgMusic));
            memset (mentry, 0, sizeof (LevelBgMusic));
            mentry->file = right;
            if (mblock->music == NULL)
                mblock->music = mentry;
            else {
                LevelBgMusic *mtmp;
                mtmp = mblock->music;
                while (mtmp->next)
                    mtmp = mtmp->next;
                mtmp->next = mentry;
            }
        } else {
            fprintf(stderr,"Warning: unknown option \"%s\"\n",left);
            free(right);
        }
        free (line);
        free (left);
    }
    return mblock;
}

/* Read the mainblock (binary) */
static int readMainBlockRW (LevelSettings * settings, SDL_RWops * rw, int len)
{
    LSB_Main *mblock;
    Uint32 aspect,zoom;
    int read = 0;
    Uint8 type;
    /* Initialize mainblock */
    mblock = malloc (sizeof (LSB_Main));
    memset (mblock, 0, sizeof (LSB_Main));
    mblock->aspect = 1.0;
    mblock->zoom = 1.0;
    /* Read mainblock */
    while (read < len) {
        read += SDL_RWread (rw, &type, 1, 1);
        switch (type) {
        case 0x01:{ /* Level name */
                char tmps[512], chr = 'a';
                int l = 0;
                while (chr != '\0' && l < 512) {
                    read += SDL_RWread (rw, &chr, 1, 1);
                    tmps[l] = chr;
                    l++;
                }
                mblock->name = malloc (l);
                strncpy (mblock->name, tmps, l);
            }
            break;
        case 0x02: /* Aspect ratio */
            read += SDL_RWread (rw, &aspect, 4, 1) * 4;
            mblock->aspect = SDL_SwapLE32(aspect)/1000.0;
            break;
        case 0x03: /* Zoom */
            read += SDL_RWread (rw, &zoom, 4, 1) * 4;
            mblock->zoom = SDL_SwapLE32(zoom)/1000.0;
            break;
        default:
            fprintf(stderr,
                "WARNING! Unknown main block item 0x%x! (skipping one byte...)\n",
                 type);
            SDL_RWseek (rw, 1, SEEK_CUR);
            read++;
            break;
        }
    }
    settings->mainblock = mblock;
    return read;
}

static void init_override_block(LSB_Override *block) {
    block->indstr_base = -1;
    block->critters = -1;
    block->stars = -1;
    block->snowfall = -1;
    block->turrets = -1;
    block->jumpgates = -1;
    block->cows = -1;
    block->fish = -1;
    block->birds = -1;
    block->bats = -1;
    block->soldiers = -1;
    block->helicopters = -1;
}

/* Read the override block */
static LSB_Override *readOverrideBlock (FILE * fp)
{
    LSB_Override *override;
    char *left, *right;
    char *line;
    override = malloc (sizeof (LSB_Override));
    init_override_block(override);
    while ((line = read_line (fp))) {
        if (line[0] == '\0')
            continue;
        if (strcmp (line, "[end]") == 0) {
            free (line);
            break;
        }
        split_string (line, '=', &left, &right);
        if (left == NULL || right == NULL) {
            fprintf (stderr,"Warning: Malformed line:\n \"%s\"\n", line);
            continue;
        }
        if (strcmp (left, "critters") == 0)
            override->critters = atoi (right);
        else if (strcmp (left, "bases_indestructable") == 0)
            override->indstr_base = atoi (right);
        else if(strcmp (left,"snowfall")==0)
            override->snowfall = atoi(right);
        else if(strcmp (left,"stars")==0)
            override->stars = atoi(right);
        else if (strcmp (left, "turrets") == 0)
            override->turrets = atoi (right);
        else if (strcmp (left, "jumpgates") == 0)
            override->jumpgates = atoi (right);
        else if (strcmp (left, "cows") == 0)
            override->cows = atoi (right);
        else if (strcmp (left, "fish") == 0)
            override->fish = atoi (right);
        else if (strcmp (left, "birds") == 0)
            override->birds = atoi (right);
        else if (strcmp (left, "bats") == 0)
            override->bats = atoi (right);
        else if (strcmp (left, "soldiers") == 0)
            override->soldiers = atoi (right);
        else if (strcmp (left, "helicopters") == 0)
            override->helicopters = atoi (right);
        else
            fprintf (stderr,"Unidentified option \"%s\" with parameter \"%s\"\n",
                    left, right);
        free (line);
        free (left);
        free (right);
    }
    return override;
}

/* Read override block (binary) */
static int readOverrideBlockRW (LevelSettings * settings, SDL_RWops * rw,
                                int len)
{
    LSB_Override *override;
    Uint8 type,byte;
    int read = 0;
    override = malloc (sizeof (LSB_Override));
    init_override_block(override);
    while (read < len) {
        read += SDL_RWread (rw, &type, 1, 1);
        read += SDL_RWread (rw, &byte, 1, 1);
        switch (type) {
        case 0x01:             /* Critters */
            override->critters=byte;
            break;
        case 0x02:             /* Bases indestructable */
            override->indstr_base=byte;
            break;
        case 0x03:             /* Stars */
            override->stars=byte;
            break;
        case 0x04:             /* Snowfall */
            override->snowfall=byte;
            break;
        case 0x05:             /* Turrets */
            override->turrets=byte;
            break;
        case 0x06:             /* Jumpgates */
            override->jumpgates=byte;
            break;
        case 0x07:             /* Cows */
            override->cows=byte;
            break;
        case 0x08:             /* Fish */
            override->fish=byte;
            break;
        case 0x09:             /* Birds */
            override->birds=byte;
            break;
        case 0x0a:             /* Bats */
            override->bats=byte;
            break;
        default:
            printf("Warning! Unknown override block item 0x%x=0x%x!\n",
                 type,byte);
            break;
        }
    }
    settings->override = override;
    return read;
}

/* Read the objects block */
static LSB_Objects *readObjectsBlock (FILE * fp)
{
    LSB_Objects *objects = NULL, *first = NULL, *prev = NULL;
    char *left, *right;
    char *line;
    while ((line = read_line (fp))) {
        if (line[0] == '\0')
            continue;
        if (strcmp (line, "[endsub]") == 0) {
            prev = objects;
            objects = NULL;
        } else if (strcmp (line, "[end]") == 0) {
            free (line);
            break;
        } else if (strcmp (line, "[object]") == 0) {
            free (line);
            objects = malloc (sizeof (LSB_Objects));
            if (objects == NULL) {
                perror("readObjectsBlock()");
                exit (1);
            }
            memset (objects, 0, sizeof (LSB_Objects));
            if (!first)
                first = objects;
            if (prev)
                prev->next = (struct LSB_Objects *) objects;
            continue;
        }
        if (!objects) {
            free (line);
            continue;
        }
        split_string (line, '=', &left, &right);
        if (left == NULL || right == NULL) {
            fprintf (stderr,"Warning: Malformed line:\n \"%s\"\n", line);
            continue;
        }
        if (strcmp (left, "type") == 0) {
            if (strcmp (right, "turret") == 0)
                objects->type = 1;
            else if (strcmp (right, "jumpgate") == 0)
                objects->type = 2;
            else if (strcmp (right, "cow") == 0)
                objects->type = 0x10;
            else if (strcmp (right, "fish") == 0)
                objects->type = 0x11;
            else if (strcmp (right, "bird") == 0)
                objects->type = 0x12;
            else if (strcmp (right, "bat") == 0)
                objects->type = 0x13;
            else if (strcmp (right, "ship") == 0)
                objects->type = 0x20;
            else
                fprintf (stderr,"Warning: Unidentified type \"%s\"\n", right);
        } else if (strcmp (left, "x") == 0)
            objects->x = atoi (right);
        else if (strcmp (left, "y") == 0)
            objects->y = atoi (right);
        else if (strcmp (left, "ceiling_attach") == 0)
            objects->ceiling_attach = atoi (right);
        else if (strcmp (left, "value") == 0)
            objects->value = atoi (right);
        else if (strcmp (left, "link") == 0)
            objects->link = atoi (right);
        else if (strcmp (left, "id") == 0)
            objects->id = atoi (right);
        else
            fprintf (stderr,"Unidenified option \"%s\" with parameter \"%s\"\n",
                    left, right);
        free (left);
        free (right);
        free (line);
    }
    return first;
}

static LSB_Palette *readPaletteBlock (FILE * fp) {
    LSB_Palette *palette;
    char *line;
    int mapto;

    palette = malloc (sizeof (LSB_Palette));
    memset (palette->entries, 0, 256);
    while ((line = read_line (fp))) {
        char *left=NULL,*right=NULL;
        if (line[0] == '\0') {
            continue;
        }
        if (strcmp (line, "[end]") == 0) {
            free (line);
            break;
        }
        split_string (line, '=', &left, &right);
        free(line);
        if (left == NULL || right == NULL) {
            fprintf (stderr,"Warning: Malformed line:\n \"%s\"\n", line);
            free(left);
            free(right);
            continue;
        }
        mapto = name2terrain(right);
        if(strchr(left,'-')) { /* Range */
            int from,to;
            sscanf(left,"%d - %d",&from,&to);
            if(from<0||from>255||to<0||to>255) {
                fprintf(stderr,"Error: palette map range %d-%d out of bounds (0-255!",
                    from,to);
                free(left);
                free(right);
                continue;
            } else if(to<from) {
                int tmp;
                tmp=to; to=from; from=tmp;
            }
            for(;from<=to;from++) palette->entries[from]=mapto;
        } else { /* Single value */
            int index;
            index=atoi(left);
            if(index<0 || index>255) {
                fprintf(stderr,"Palette map index %d out of bounds (0-255)!\n",
                        index);
            } else {
                palette->entries[index]=mapto;
            }
        }
        free(left);
        free(right);
    }
    return palette;
}

/* Read objects block (binary) */
static int readObjectsBlockRW (LevelSettings * settings, SDL_RWops * rw,
                               int len)
{
    LSB_Objects *objects = NULL, *first = NULL, *prev;
    int read = 0;
    Uint16 l;
    while (read < len) {
        prev = objects;
        objects = malloc (sizeof (LSB_Objects));
        if (objects == NULL) {
            perror("readObjectsBlockRW()");
            exit (1);
        }
        memset (objects, 0, sizeof (LSB_Objects));
        if (!first)
            first = objects;
        if (prev)
            prev->next = (struct LSB_Objects *) objects;
        read += SDL_RWread (rw, &l, 2, 1) * 2;
        l = SDL_SwapLE16(l);
        read += l;
        l -= SDL_RWread (rw, &objects->type, 1, 1);
        l -= SDL_RWread (rw, &objects->x, 4, 1) * 4;
        l -= SDL_RWread (rw, &objects->y, 4, 1) * 4;
        l -= SDL_RWread (rw, &objects->ceiling_attach, 1, 1);
        l -= SDL_RWread (rw, &objects->value, 1, 1);
        l -= SDL_RWread (rw, &objects->id, 4, 1) * 4;
        l -= SDL_RWread (rw, &objects->link, 4, 1) * 4;
        if (l)
            SDL_RWseek (rw, l, SEEK_CUR);

        objects->x = SDL_SwapLE32(objects->x);
        objects->y = SDL_SwapLE32(objects->y);
        objects->id = SDL_SwapLE32(objects->id);
        objects->link = SDL_SwapLE32(objects->link);
    }
    settings->objects = first;
    return read;
}

/* Read the palette block (binary) */
static int readPaletteBlockRW (LevelSettings * settings, SDL_RWops * rw,
                               int len)
{
    LSB_Palette *palette;
    int read;
    palette = malloc (sizeof (LSB_Palette));
    if (palette == NULL) {
        perror("readPaletteBlockRW()");
        exit (1);
    }
    if (len != 256)
        fprintf (stderr,"Warning: Palette block has strange length (%d)\n",len);
    read = SDL_RWread (rw, &palette->entries, 1, 256);
    if (read != 256)
        fprintf (stderr,"Warning! Tried to read 256 bytes but read only %d!\n",
                read);
    len -= read;
    if (len)
        SDL_RWseek (rw, len, SEEK_CUR);
    settings->palette = palette;
    return read + len;
}

/* Read a single line from a file, discarding whitespace and comment lines */
static char *read_line (FILE * fp)
{
    char tmps[512], *line;
    if (fgets (tmps, 512, fp) == NULL)
        return NULL;
    line = strip_white_space (tmps);
    if (line == NULL)
        return "\0";
    if (line[0] == '#') {
        free (line);
        return "\0";
    }
    return line;
}

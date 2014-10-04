/*
 * LDAT - Luola Datafile format archiver
 * Copyright (C) 2002-2005 Calle Laakkonen
 *
 * File        : archive.c
 * Description : Functions to manipulate LDAT files
 * Author(s)   : Calle Laakkonen
 *
 * LDAT is free software; you can redistribute it and/or modify
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#include "ldat.h"
#include "archive.h"

#include "parser.h"

struct LDATIndex {
    char filename[512];
    struct dllist *files;
};

/* Make a new Filename */
struct Filename *make_file(const char *name,const char *id,int index) {
    struct Filename *file;
    struct stat finfo;
    if (stat(name, &finfo)) {
        perror(name);
        return NULL;
    }

    file = malloc(sizeof(struct Filename));
    if(!file) {
        perror("make_file");
        return NULL;
    }

    strcpy(file->filename,name);
    strcpy(file->id,id);
    file->index = index;
    file->len = finfo.st_size;

    return file;
}

/* Read a list of files to extract/archive */
static int read_packfile(struct LDATIndex *index, const char *filename,int getlen) {
    char tmps[512],*line=NULL;
    struct Filename fn;
    FILE *fp;

    fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        return 1;
    }
    for (; fgets(tmps, sizeof(tmps) - 1, fp); free(line)) {
        line = strip_white_space(tmps);
        if (line == NULL || strlen(line) == 0 || line[0]=='#')
            continue;
        if (isdigit(line[0])) {	/* Data file entry */
            struct Filename *newfile;
            struct stat finfo;

            sscanf(line, "%d %s %s\n", &fn.index, &fn.id, &fn.filename);
            if(getlen) {
                if (stat(fn.filename, &finfo)) {
                    perror(fn.filename);
                    continue;
                }
                fn.len = finfo.st_size;
            }
            newfile = malloc(sizeof(struct Filename));
            memcpy(newfile,&fn,sizeof(struct Filename));
            if(index->files)
                dllist_append(index->files,newfile);
            else
                index->files = dllist_append(NULL,newfile);
        } else if(sscanf(line,"ldat: %s",tmps)==1) {
            strcpy(index->filename,tmps);
	    } else {
            fprintf(stderr,"%s: unhandled line\n",line);
        }
	}
    fclose(fp);
    return 0;
}

static const char *align_space(char *str, int len)
{
    static char buffer[128];
    int l;
    l = strlen(str);
    l = len - l;
    memset(buffer, ' ', l);
    buffer[l] = '\0';
    return buffer;
}

/* Write data from an SDL_RWops to file */
static int save_rw_to_file(const char *file, SDL_RWops *rw, size_t len)
{
    size_t copied = 0, read, toread;
    Uint8 buffer[1024];
    FILE *fp;

    fp = fopen(file, "wb");
    if (fp == NULL) {
        perror(file);
        return 1;
    }
    while (copied < len) {
        if (len - copied < sizeof(buffer))
            toread = len - copied;
        else
            toread = sizeof(buffer);
        read = SDL_RWread(rw, buffer, 1, toread);
        if (read == 0) {
            fprintf(stderr,"save_rw_to_file: %s\n",SDL_GetError());
            return 1;
        }
        if(fwrite(buffer, 1, read, fp)!=read) {
            perror(file);
            return 1;
        }
        copied += read;
    }
    fclose(fp);
    return 0;
}

/* Gets the filename without the path component */
static char *getbasename(const char *fn)
{
    char *filename, *ptr;
    int len;
    ptr = strrchr(fn, '/');
    if (ptr == NULL)
        return strdup(fn);
    len = strlen(fn) - (ptr - fn);
    filename = malloc(len);
    strcpy(filename, ptr + 1);
    filename[len] = '\0';
    return filename;
}

/*
 * Print the list of items in an LDAT file to stdout 
 *
 * Returns the number of items printed 
 */
int print_ldat_catalog(LDAT * ldat, int verbose)
{
    LDAT_Block *item;
    int count = 0;
    item = ldat->catalog;
    while (item) {
        if (verbose)
            printf("%s%s%d\t\t%d\t%d\n", item->ID,
               align_space(item->ID, 30), item->index, item->size,
               item->pos);
        else
            printf("%s%s%d\n", item->ID, align_space(item->ID, 30),
               item->index);
        item = item->next;
        count++;
    }
    return count;
}

/*
 * Put the selected files into an LDAT file 
 */
int pack_ldat_files(LDAT * ldat, struct dllist *filenames,int verbose)
{

    while (filenames) {
        struct Filename *file = filenames->data;
        SDL_RWops *rw;
        rw = SDL_RWFromFile(file->filename, "rb");
        if(!rw) {
            fprintf(stderr,"%s: %s\n",file->filename,SDL_GetError());
            return 1;
        }

        ldat_put_item(ldat, strdup(file->id), file->index, rw, file->len);
        if(verbose)
            printf("Put item with ID \"%s\" and index %d, size %d\n",
                    file->id, file->index, file->len);
        
        filenames = filenames->next;
    }
    return 0;
}

/*
 * Unpack selected file from an LDAT file 
 */
int unpack_ldat_file(LDAT * ldat, const struct Filename *file,int verbose)
{
    SDL_RWops *item = ldat_get_item(ldat, file->id, file->index);
    if (item) {
        if(verbose)
            printf("Extracting file \"%s\" %d... ", file->id,file->index);
        if (!save_rw_to_file(file->filename, item,
                    ldat_get_item_length(ldat, file->id, file->index)))
        {
            if(verbose)
                printf("Ok.\n");
        } else
        {
            if(verbose)
                printf("Failed.\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Couldn't get item \"%s\" %d\n", file->id,file->index);
        return 1;
    }
    return 0;
}

/*
 * Pack an LDAT file according to the description in INDEX 
 */
/*
 * Returns the path for the output LDAT file 
 */
char *pack_ldat_index(LDAT * ldat, const char *indexfile, int packindex,int verbose)
{
    struct LDATIndex index;
    char *rval;

    index.filename[0] = '\0';
    index.files = NULL;

    if(read_packfile(&index,indexfile,1)) {
        return NULL;
    }

    if(index.files == NULL) {
        printf("Nothing to do!\n");
        return NULL;
    }

    if(packindex) {
        struct Filename *ifile = make_file(indexfile,"INDEX",0);
        if(ifile==NULL)
            return NULL;
        dllist_append(index.files,ifile);
    }

    if(pack_ldat_files(ldat,index.files,verbose))
        rval = NULL;
    else
        rval = strdup(index.filename);

    dllist_free(index.files,free);
    return rval;
}

/*
 * Unpack an LDAT file according to the description in INDEX 
 */
int unpack_ldat_index(const char *indexfile,int verbose)
{
    struct LDATIndex index;
    LDAT *ldat;
    int rval;

    index.filename[0] = '\0';
    index.files = NULL;

    if(read_packfile(&index,indexfile,0)) {
        return 1;
    }

    if(index.files == NULL) {
        printf("Nothing to do!\n");
        return 1;
    }

    ldat = ldat_open_file(index.filename);
    if(!ldat)
        rval = -1;
    else {
        struct dllist *ptr = index.files;
        while(ptr) {
            rval = unpack_ldat_file(ldat,ptr->data,verbose);
            if(rval!=0) break;
            ptr=ptr->next;
        }
        dllist_free(index.files,free);
        ldat_free(ldat);
    }
    return rval;
}

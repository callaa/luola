/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2004-2006 Calle Laakkonen
 *
 * File        : parser.c
 * Description : Generic configuration file parser
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
#include <string.h>
#include <stdio.h>

#include "SDL.h"
#include "SDL_rwops.h"

#include "parser.h"

/* actually strips all characters upto 0x20, \r, \n, \t... */
char *strip_white_space (const char *str)
{
    int len;
    char *newstr = NULL;
    const char *s1, *s2;
    s1 = str;
    s2 = str + strlen (str) - 1;
    while (s1 < s2 && (*s1 <= ' '))
        s1++;
    while (s2 >= s1 && (*s2 <= ' '))
        s2--;
    s2++;
    len = s2 - s1;
    if (len <= 0)
        return NULL;
    newstr = malloc (sizeof (char) * (len + 1));
    if (newstr == NULL) {
        printf
            ("Malloc error at strip_white_space ! (attempted to allocate %d bytes\n",
             len);
        exit (1);
    }
    strncpy (newstr, s1, len);
    newstr[len] = '\0';
    return newstr;
}

static int split_string (char *str, char delim, char **left, char **right)
{
    int l1;
    char *tl = NULL, *tr = NULL;
    tl = strchr (str, delim);
    if(tl) l1 = tl-str; else return 1;
    tl = malloc (l1 + 1);
    strncpy (tl, str, l1);
    tl[l1] = 0;
    tr = malloc (strlen (str) - l1);
    strcpy (tr, str + l1 + 1);

    *left = strip_white_space (tl);
    *right = strip_white_space (tr);
    free (tl);
    free (tr);
    return 0;
}

/*** Read a single line from file, removing comments and empty lines ***/
/* Line is read up to EOL, EOF or len. Number of bytes read is substracted
 * from len. */
static char *read_line(SDL_RWops *rw,size_t *len) {
    char tmps[512],*line;
    size_t limit=sizeof(tmps),read=0;

    if(len && *len<limit)
        limit = *len;

    while(read<limit) {
        if(SDL_RWread(rw,tmps+read,1,1)!=1) break;
        read++;
        if(tmps[read-1]=='\n') break;
    }
    if(read==0) return NULL;
    tmps[read]='\0';

    line=strip_white_space(tmps);
    if(line==NULL)
        line="\0";
    else if(line[0]=='#') {
        free(line);
        line="\0";
    }
    if(len)
        *len -= read;
    return line;
}

/*** Read and parse a configuration file ***/
struct dllist *read_config_file(const char *filename,int quiet) {
    SDL_RWops *rw = SDL_RWFromFile(filename,"r");
    struct dllist *config;

    if(!rw) {
        if(!quiet)
            fprintf(stderr,"%s: %s\n",filename,SDL_GetError());
        return NULL;
    }

    config = read_config_rw(rw,0,quiet);
    SDL_FreeRW(rw);

    return config;
}

/*** Read and parse a configuration file from an SDL_RWops ***/
struct dllist *read_config_rw(SDL_RWops *rw,size_t len,int quiet) {
	struct ConfigBlock *block=NULL;
	struct dllist *blocks=NULL;
    size_t *length;
	char *str;

    if(len>0)
        length=&len;
    else
        length=NULL;

	while((str=read_line(rw,length))) {
		struct KeyValue *pair;
		if(str[0]=='\0') continue;
		if(str[0]=='[') { /* New block */
			if(block && block->values)
				while(block->values->prev)
					block->values=block->values->prev;
			block=malloc(sizeof(struct ConfigBlock));
			block->title=strdup(str+1);
			block->title[strlen(block->title)-1]='\0';
			block->values=NULL;
			blocks=dllist_append(blocks,block);
            free(str);
			continue;
		} else if(block==NULL) { /* Default block */
			block=malloc(sizeof(struct ConfigBlock));
            block->title=NULL;
            block->values=NULL;
			blocks=dllist_append(blocks,block);
		}
		pair=malloc(sizeof(struct KeyValue));
        pair->key = NULL; pair->value = NULL;
		split_string(str,'=',&pair->key,&pair->value);
		block->values=dllist_append(block->values,pair);
        free(str);
	}
	if(block && block->values)
		while(block->values->prev)
			block->values=block->values->prev;
	if(blocks)
		while(blocks->prev) blocks=blocks->prev;
	return blocks;
}

/*** Set values ***/
void translate_config(struct dllist *values,const struct Translate tr[],int quiet) {
	int r;
	while(values) {
		struct KeyValue *pair=values->data;
        if(pair==NULL || pair->key==NULL || pair->value==NULL) {
            fprintf(stderr,"Unrecognized setting.\n");
            fprintf(stderr,"\"%s\" = \"%s\"\n",pair->key?pair->key:"(null)",
                    pair->value?pair->value:"(null)");
        } else {
            r=0;
            while(tr[r].key) {
                if(strcmp(tr[r].key,pair->key)==0) {
                    switch(tr[r].type) {
                        case CFG_INT: *((int*)tr[r].ptr)=atoi(pair->value); break;
                        case CFG_FLOAT: *((float*)tr[r].ptr)=atof(pair->value); break;
                        case CFG_DOUBLE: *((double*)tr[r].ptr)=atof(pair->value); break;
                        case CFG_STRING: *((char**)tr[r].ptr)=strdup(pair->value); break;
                        case CFG_MULTISTRING: {
                            struct dllist **list = tr[r].ptr;
                            if(*list)
                                dllist_append(*list,strdup(pair->value));
                            else
                                *list = dllist_append(NULL,strdup(pair->value));
                            } break;

                    }
                    break;
                }
                r++;
            }
            if(tr[r].key==NULL && quiet==0) {
                fprintf(stderr,"Unrecognized setting \"%s\"\n",pair->key);
            }
        }
		values=values->next;
	}
}

/* Used by dllist_free to free a configuration file buffer */
static void free_config_key_pair(void *data) {
    free(((struct KeyValue*)data)->key);
    free(((struct KeyValue*)data)->value);
	free(data);
}

void free_config_file(void *data) {
	struct ConfigBlock *block=data;
	free(block->title);
	dllist_free(block->values,free_config_key_pair);
	free(block);
}


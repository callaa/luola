/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2004-2005 Calle Laakkonen
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

#include "parser.h"

/*** Read a single line from file, removing comments and empty lines ***/
char *readLine(FILE *fp) {
  char tmps[512],*line;
  if(fgets(tmps,512,fp)==NULL) return NULL;
  line=stripWhiteSpace(tmps);
  if(line==NULL) return "\0";
  if(line[0]=='#') {free(line); return "\0";}
  return line;
}

/*** Read and parse a configuration file ***/
struct dllist *read_config_file(const char *filename,int quiet) {
	struct ConfigBlock *block=NULL;
	struct dllist *blocks=NULL;
	char *str;
	FILE *fp;

	fp=fopen(filename,"r");
	if(!fp) {
        if(!quiet) perror(filename);
		return NULL;
	}

	while((str=readLine(fp))) {
		struct KeyValue *pair;
		if(str[0]=='\0') continue;
		if(str[0]=='[') {			/* New block */
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
		} else if(block==NULL) {	/* Default block */
			block=malloc(sizeof(struct ConfigBlock));
            block->title=NULL;
            block->values=NULL;
			blocks=dllist_append(blocks,block);
		}
		pair=malloc(sizeof(struct KeyValue));
		splitString(str,'=',&pair->key,&pair->value);
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
void translate_config(struct dllist *values,int count,char **keys,CfgPtrType *types,void **pointers,int quiet) {
	int r;
	while(values) {
		struct KeyValue *pair=values->data;
        if(pair==NULL || pair->key==NULL || pair->value==NULL) {
            printf("Unrecognized setting.\n");
            printf("\"%s\" = \"%s\"\n",pair->key?pair->key:"(null)",
                    pair->value?pair->value:"(null)");
        } else {
            for(r=0;r<count;r++) {
                if(strcmp(keys[r],pair->key)==0) {
                    switch(types[r]) {
                        case CFG_INT: *((int*)pointers[r])=atoi(pair->value); break;
                        case CFG_FLOAT: *((float*)pointers[r])=atof(pair->value); break;
                        case CFG_DOUBLE: *((double*)pointers[r])=atof(pair->value); break;
                        case CFG_STRING: *((char**)pointers[r])=strdup(pair->value); break;

                    }
                    break;
                }
            }
            if(r==count && quiet==0) {
                printf("Unrecognized setting \"%s\"\n",pair->key);
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

/* actually strips all characters upto 0x20, \r, \n, \t... */
char *stripWhiteSpace(const char *str) {
	int len;
	char *newstr=NULL;
	const char *s1,*s2;
	s1=str;
	s2=str+strlen(str)-1;
	while(s1<s2 && (*s1<=' ')) s1++;
	while(s2>=s1 && (*s2<=' ')) s2--;
	s2++;
	len=s2-s1;
	if(len<=0) return NULL;
	newstr=malloc(len+1);
	if(newstr==NULL) {
		perror("malloc");
		return NULL;
	}
	strncpy(newstr,s1,len);
	newstr[len]='\0';
	return newstr;
}

int splitString(char *str,char delim,char **left,char **right) {
	int l1;
	char *tl=NULL,*tr=NULL;
	l1=strchr(str,delim)-str;
	if(l1<0) return 1;
	tl=malloc(l1+1);
	strncpy(tl,str,l1);
	tl[l1]=0;
	tr=malloc(strlen(str)-l1);
	strcpy(tr,str+l1+1);

	*left=stripWhiteSpace(tl);
	*right=stripWhiteSpace(tr);
	free(tl);
	free(tr);
	return 0;
}


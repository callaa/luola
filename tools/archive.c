/*
 * LDAT - Luola Datafile format archiver
 * Copyright (C) 2002 Calle Laakkonen
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
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#include "ldat.h"
#include "archive.h"
#include "stringutil.h"

static const char *align_space(char *str,int len) {
  static char buffer[128];
  int l;
  l=strlen(str);
  l=len-l;
  memset(buffer,' ',l);
  buffer[l]='\0';
  return buffer;
}

static int save_rw_to_file(char *file,SDL_RWops *rw,Uint32 len) {
  Uint8 *buffer;
  Uint32 copied=0,read,toread;
  FILE *fp;
  fp=fopen(file,"w");
  if(fp==NULL) {
    printf("Error ! Could not open file \"%s\" for writing !\n",file);
    return;
  }
  buffer=(Uint8*)malloc(1024); /* Buffer size */
  while(copied<len) {
    if(len-copied<1024) toread=len-copied; else toread=1024;
    read=SDL_RWread(rw,buffer,1,toread);
    if(read<toread) {
      printf("Error ! Read only %d bytes when we were supposed to read %d bytes !\n");
      return 1;
    }
    fwrite(buffer,1,read,fp);
    copied+=read;
  }
  free(buffer);
  fclose(fp);
  return 0;
}

static char *getfilename(char *fn) {
  char *filename,*ptr;
  int len;
  ptr=strrchr(fn,'/');
  if(ptr==NULL) return fn;
  len=strlen(fn)-(ptr-fn);
  filename=malloc(len);
  strcpy(filename,ptr+1);
  filename[len]='\0';
  return filename;
}

/* Print the list of items in an LDAT file to stdout */
/* Returns the number of items printed */
int print_ldat_catalog(LDAT *ldat,int verbose) {
  LDAT_Block *item;
  int count=0;
  item=ldat->catalog;
  while(item) {
    if(verbose)
      printf("%s%s%d\t\t%d\t%d\n",item->ID,align_space(item->ID,30),item->index,item->size,item->pos);
    else
      printf("%s%s%d\n",item->ID,align_space(item->ID,30),item->index);
    item=item->next;
    count++;
  }
  return count;
}

/* Put the selected files into an LDAT file */
int pack_ldat_files(LDAT *ldat,Filename *files) {
  struct stat finfo;
  Filename *next;
  SDL_RWops *rw;
  char *ID;
  int r,index;

  while(files) {
    next=files->next;
    index=0;
    if(next) {
      for(r=0;r<strlen(next->filename);r++)
        if(isdigit(next->filename[r])==0) {r=-1; break;}
      if(r>0) {
        index=atoi(next->filename);
        next=next->next;
      }
    }
    if(stat(files->filename,&finfo)) {
      perror(files->filename);
      files=next; continue;
    }
    ID=getfilename(files->filename);
    rw=SDL_RWFromFile(files->filename,"rb");
    ldat_put_item(ldat,ID,index,rw,finfo.st_size);
    printf("Put item with ID \"%s\" and index %d, size %d\n",ID,index,finfo.st_size);
    files=next;
  }
  return 0;
}

/* Unpack selected files from an LDAT file */
int unpack_ldat_files(LDAT *ldat,Filename *files) {
  SDL_RWops *item;
  Filename *next;
  int r,index;
  while(files) {
    next=files->next;
    index=0;
    if(next) {
      for(r=0;r<strlen(next->filename);r++)
        if(isdigit(next->filename[r])==0) {r=-1; break;}
      if(r>0) {
        index=atoi(next->filename);
        next=next->next;
      }
    }
    item=ldat_get_item(ldat,files->filename,index);
    if(item) {
      printf("Unpacking file \"%s\"... ",files->filename);
      if(!save_rw_to_file(files->filename,item,ldat_get_item_length(ldat,files->filename,index)))
        printf("Ok.\n");
    }
    files=next;
  }
  return 0;
}

/* Pack an LDAT file according to the description in INDEX */
/* Returns the path for the output LDAT file */
char *pack_ldat_index(LDAT *ldat,char *filename,int packindex) {
  char *line=NULL,*ldat_out="foo.ldat";
  char tmps[512],ID[128],fn[384];
  char *left,*right;
  struct stat finfo;
  int index;
  FILE *fp;
  if(packindex) { /* Automatically include the index file */
    SDL_RWops *indexrw;
    if(stat(filename,&finfo)) {
      printf("Error ! Could not stat file \"%s\"\n",filename);
      return NULL;
    }
    indexrw=SDL_RWFromFile(filename,"rb");
    ldat_put_item(ldat,"INDEX",0,indexrw,finfo.st_size);
    printf("Put INDEX item\n");
  }
  fp=fopen(filename,"r");
  if(!fp) {
    printf("Error ! Could not open file \"%s\"\n",filename);
    return NULL;
  }
  for (; fgets(tmps,sizeof(tmps)-1,fp); free(line)) {
    line=strip_white_space(tmps);
    if(line==NULL || strlen(line)==0) continue;
    if(line[0]=='#') continue;
    if(isdigit(line[0])) { /* Data file entry */
       SDL_RWops *rw;
       sscanf(line,"%d %s %s\n",&index,&ID,&fn);
       if(stat(fn,&finfo)) {
         perror(fn);
	 continue;
       }
       rw=SDL_RWFromFile(fn,"r");
       ldat_put_item(ldat,strdup(ID),index,rw,finfo.st_size);
       printf("Put item with ID \"%s\" and index %d, size %d\n",ID,index,finfo.st_size);
    } else { /* A setting */
      split_string(line,':',&left,&right);
      if(strcmp(left,"ldat")==0) {
        ldat_out=right;
      } else free(right);
      free(left);
    }
  }
  fclose(fp);
  return ldat_out;
}

/* Unpack an LDAT file according to the description in INDEX */
int unpack_ldat_index(char *filename) {
  char *line=NULL,*ldat_in="foo.ldat";
  char tmps[512],ID[128],fn[384];
  Filename *files=NULL,*prev;
  char *left,*right;
  SDL_RWops *item;
  LDAT *ldat;
  FILE *fp;
  fp=fopen(filename,"r");
  if(!fp) {
    printf("Error ! Could not open file \"%s\"\n",filename);
    return 1;
  }
  for (; fgets(tmps,sizeof(tmps)-1,fp); free(line)) {
    line=strip_white_space(tmps);
    if(line==NULL || strlen(line)==0) continue;
    if(line[0]=='#') continue;
    if(isdigit(line[0])) { /* Data file entry */
      Filename *newfile;
      newfile=(Filename*)malloc(sizeof(Filename));
      newfile->next=NULL;
      newfile->prev=files;
      if(files) files->next=newfile;
      files=newfile;
      sscanf(line,"%d %s %s\n",&newfile->index,&ID,&fn);
      newfile->id=strdup(ID);
      newfile->filename=strdup(fn);
    } else { /* A setting */
      split_string(line,':',&left,&right);
      if(strcmp(left,"ldat")==0) {
        ldat_in=right;
      } else free(right);
      free(left);
    }
  }
  fclose(fp);
  /* Data gathering complete, unpack files */
  ldat=ldat_open_file(ldat_in);
  if(ldat==NULL) return 1;
  while(files) {
    item=ldat_get_item(ldat,files->id,files->index);
    if(item) {
      printf("Unpacking file \"%s\"... ",files->filename);
      if(!save_rw_to_file(files->filename,item,ldat_get_item_length(ldat,files->id,files->index)))
        printf("Ok.\n");
    }
    free(files->id);
    free(files->filename);
    prev=files->prev;
    free(files);
    files=prev;
  }
  return 0;
}

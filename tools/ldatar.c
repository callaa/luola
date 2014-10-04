/*
 * LDAT - Luola Datafile format archiver
 * Copyright (C) 2002 Calle Laakkonen
 *
 * File        : ldatar.c
 * Description : A program to manipulate LDAT archives
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

#include "ldat.h"
#include "archive.h"

/* Print help */
static void print_help(void);

/*** MAIN ***/
int main(int argc, char *argv[]) {
  LDAT *ldatfile=NULL;
  Filename *files=NULL;
  int r,verbose=0,useindex=0,noindex=0;
  char *outfile;
  enum {ArchiveFoo,ArchiveList,ArchivePack,ArchiveExtract} archive_mode=ArchiveFoo;
  /* Parse command line arguments */
  if(argc==1) {
    print_help();
    return 0;
  }
  for(r=1;r<argc;r++) {
    if(strcmp(argv[r],"--help")==0) { print_help(); return 0; }
    if(strcmp(argv[r],"-l")==0 || strcmp(argv[r],"--list")==0) archive_mode=ArchiveList;
    else if(strcmp(argv[r],"-p")==0 || strcmp(argv[r],"--pack")==0) archive_mode=ArchivePack;
    else if(strcmp(argv[r],"-x")==0 || strcmp(argv[r],"--extract")==0) archive_mode=ArchiveExtract;
    else if(strcmp(argv[r],"-v")==0 || strcmp(argv[r],"--verbose")==0) verbose=1;
    else if(strcmp(argv[r],"-i")==0 || strcmp(argv[r],"--index")==0) useindex=1;
    else if(strcmp(argv[r],"--noindex")==0) noindex=1;
    else { /* No such argument, probably a filename */
      Filename *newfile;
      newfile=malloc(sizeof(Filename));
      newfile->filename=argv[r];
      newfile->prev=files;
      newfile->next=NULL;
      if(files) files->next=newfile;
      files=newfile;
    }
  }
  /* Done parsing command line arguments */
  /* Rewind filename list */
  if(files==NULL) {
    printf("No archive selected !\n");
    return 1;
  }
  while(files->prev) files=files->prev;
  /* Archive actions */
  switch(archive_mode) {
    case ArchiveFoo:	/* No action selected */
      printf("You did not select the archive action !\n");
      return 1;
    case ArchiveList:	/* List contents of an archive */
      ldatfile=ldat_open_file(files->filename);
      if(verbose) printf("Name                          Index\tSize\tOffset\n");
      print_ldat_catalog(ldatfile,verbose);
      ldat_free(ldatfile);
      break;
    case ArchivePack: /* Pack files into an archive */
      if(files->next==NULL && useindex==0) {
        printf("No files selected !\n");
	return 1;
      }
      ldatfile=ldat_create();
      if(useindex) {
        outfile=pack_ldat_index(ldatfile,files->filename,!noindex);
	if(outfile==NULL) return 1;
	printf("Creating file \"%s\"\n",outfile);
      } else {
        outfile=files->filename;
        pack_ldat_files(ldatfile,files->next);
      }
      if(ldat_save_file(ldatfile,outfile)) {
        printf("Error occured while saving !\n");
	return 1;
      }
      ldat_free(ldatfile);
      break;
    case ArchiveExtract:	/* Extract files from an archive */
      if(files->next==NULL && useindex==0) {
        printf("No files selected !\n");
	return 1;
      }
      if(useindex) {
        if(unpack_ldat_index(files->filename)) return 1;
      } else {
        ldatfile=ldat_open_file(files->filename);
	if(!ldatfile) return 1;
        unpack_ldat_files(ldatfile,files->next);
	ldat_free(ldatfile);
      }
      break;
    default: break;
  }

  return 0;
}

static void print_help(void) {
  printf("Usage: ldat <options> <ldat file> [filename1...]\nOptions:\n");
  printf("\t--help\t\t\tShow this help\n");
  printf("\t-l, --list\t\tList the contents of an archive\n");
  printf("\t-p, --pack\t\tPack files into an archive\n");
  printf("\t-x, --extract\t\tExtract files from an archive\n");
  printf("\t--index\t\t\tUse an index file to generate LDAT\n");
  printf("\t--noindex\t\t\tDo not automatically insert index file\n");
  printf("\t-v, --verbose\t\tVerbose mode\n");
}

/*
 * LDAT - Luola Datafile format archiver
 * Copyright (C) 2002 Calle Laakkonen
 *
 * File        : archive.h
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

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "list.h"

struct Filename {
  char filename[256];
  char id[256];
  int index;
  size_t len;
};

/* Make a new Filename, with proper length */
extern struct Filename *make_file(const char *name,const char *id,int index);

/* Print the list of items in an LDAT file to stdout */
/* Returns the number of items printed */
extern int print_ldat_catalog(LDAT *ldat,int verbose);

/* Put the selected files into an LDAT file */
/* filenames is a list of struct Filename */
extern int pack_ldat_files(LDAT *ldat,struct dllist *filenames,int verbose);

/* Unpack selected file from an LDAT archive */
extern int unpack_ldat_file(LDAT *ldat,const struct Filename *file,int verbose);

/* Pack an LDAT file according to the description in INDEX */
/* Returns the path for the output LDAT file */
extern char *pack_ldat_index(LDAT *ldat,const char *indexfile,int packindex,int verbose);

/* Unpack an LDAT file according to the description in INDEX */
extern int unpack_ldat_index(const char *indexfile,int verbose);

#endif

/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2004-2005 Calle Laakkonen
 *
 * File        : parser.h
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

#ifndef PARSER_H
#define PARSER_H

#include "list.h"

struct ConfigBlock {
	char *title;
	struct dllist *values;
};

struct KeyValue {
	char *key;
	char *value;
};

typedef enum {CFG_INT,CFG_FLOAT,CFG_DOUBLE,CFG_STRING} CfgPtrType;

extern struct dllist *read_config_file(const char *filename,int quiet);

extern void translate_config(struct dllist *values,int count,char **keys,CfgPtrType *types,void **pointers,int quiet);

extern void free_config_file(void *data);

/* Some useful functions */
/* need to export ? */
extern char *stripWhiteSpace(const char *str);
extern int splitString(char *str,char delim,char **left,char **right);
extern char *readLine(FILE *fp);

#endif


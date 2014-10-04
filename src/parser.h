/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2004-2006 Calle Laakkonen
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

struct Translate {
    char *key;
    enum {CFG_INT,CFG_FLOAT,CFG_DOUBLE,CFG_STRING,CFG_MULTISTRING} type;
    void *ptr;
};

/* Read a configuration file. Returns a list of ConfigBlocks */
extern struct dllist *read_config_file(const char *filename,int quiet);

/* Read a configuration file from an SDL_RWops. len is the length of the */
/* configuration file. If 0, RWops is read until EOF. */
extern struct dllist *read_config_rw(SDL_RWops *rw,size_t len,int quiet);

/* Extract data from a ConfigBlock */
extern void translate_config(struct dllist *values,const struct Translate tr[],int quiet);

/* Free a config block. Pass to dllist_free() */
extern void free_config_file(void *data);

/* Strip all whitespace characters from beginning and end */
extern char *strip_white_space (const char *str);

#endif


/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2004-2006 Calle Laakkonen
 *
 * File        : list.c
 * Description : Linked list functions
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

#include "list.h"

struct dllist *dllist_append(struct dllist *list, void *data) {
	struct dllist *newentry;
	newentry = malloc(sizeof(struct dllist));
	if(!newentry) {
		perror("malloc");
        return NULL;
	}
	newentry->next = NULL;
	newentry->data = data;
	if(list) {
		while(list->next)
			list = list->next;
        newentry->prev=list;
		list->next = newentry;
	} else {
        newentry->prev=NULL;
    }
	return newentry;
}

struct dllist *dllist_prepend(struct dllist *list, void *data) {
	struct dllist *newentry;
	newentry = malloc(sizeof(struct dllist));
	if(!newentry) {
		perror("malloc");
		return NULL;
	}
	if(list) {
		while(list->prev)
			list = list->prev;
		list->prev = newentry;
	}
	newentry->next = list;
	newentry->prev = NULL;
	newentry->data = data;
	return newentry;
}

struct dllist *dllist_remove(struct dllist *elem) {
    struct dllist *next;
    if(elem->prev) {
        next=NULL;
        elem->prev->next=elem->next;
    } else {
        next=elem->next;
    }
    if(elem->next) {
        elem->next->prev=elem->prev;
    }
    free(elem);

    return next;
}

int dllist_count(struct dllist *list) {
    int count=0;
    if(list) {
        struct dllist *ptr=list->prev;
        while(list) {
            ++count;
            list=list->next;
        }
        while(ptr) {
            ++count;
            ptr=ptr->prev;
        }
    }
    return count;
}

struct dllist *dllist_find(struct dllist *list, void *data) {
    while(list) {
        if(list->data==data) return list;
        list=list->next;
    }
    return NULL;
}

void dllist_free(struct dllist *list,void (*freefunction)(void *data)) {
    if(list) {
        struct dllist *next;
        while(list->prev) list=list->prev;
        while(list) {
            if(freefunction) freefunction(list->data);
            next=list->next;
            free(list);
            list=next;
        }
    }
}


/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2004-2006 Calle Laakkonen
 *
 * File        : list.h
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

#ifndef LIST_H
#define LIST_H

/* Doubly linked list */
struct dllist {
    void *data;
    struct dllist *next, *prev;
};

/* Append a new entry to a doubly linked     */
/* list. Returns a pointer to the new entry. */
extern struct dllist *dllist_append(struct dllist *list, void *data);

/* Insert a new entry to the beginning of a doubly  */
/* linked list. Returns a pointer to the new entry. */
extern struct dllist *dllist_prepend(struct dllist *list, void *data);

/* Remove an entry from a doubly linked list. If data is the */
/* first entry on the list, the next entry will be returned. */
/* You have to free the data contained in the entry yourself */
extern struct dllist *dllist_remove(struct dllist *elem);

/* Returns the number of items in a list */
extern int dllist_count(struct dllist *list);

/* Find an item from a list */
extern struct dllist *dllist_find(struct dllist *list, void *data);

/* Free a list. freefunction is used to free */
/* the data. If NULL, data is not freed.     */
extern void dllist_free(struct dllist *list,void (*freefunction)(void*));

#endif

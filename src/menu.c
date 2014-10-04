/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003 Calle Laakkonen
 *
 * File        : menu.c
 * Description : Menu library
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "menu.h"
#include "font.h"

/* Internally used functions */
static void update_menu_cache_item (MenuCache * cache, Menu * menu, int index,
                                    MenuItem * item);
static void menu_cache_ascend (ToplevelMenu * menu);
static void menu_cache_descend (ToplevelMenu * menu);
static void menu_inherit_values (Menu * menu);

/* Internally used datatypes */
typedef struct _Menuitemlist {
    MenuItem *item;
    struct _Menuitemlist *next;
} Menuitemlist;

/* Creates the toplevel menu				*/
ToplevelMenu *create_toplevel_menu (int menucount, Menu ** menus)
{
    ToplevelMenu *tl;
    int m, r, s;
    if (menucount <= 0)
        return NULL;
    /* First arrange the menus into a tree */
    for (m = 0; m < menucount; m++)
        for (r = 0; r < menus[m]->itemcount; r++)
            if (menus[m]->items[r]->type == MNU_ITEM_SUBMENU)
                for (s = 0; s < menucount; s++)
                    if (menus[s]->ID == (Uint32) menus[m]->items[r]->value) {
                        menus[m]->items[r]->submenu = menus[s];
                        menus[m]->items[r]->value = NULL;
                        if (menus[s]->parent)
                            printf
                                ("Warning ! Menu %d already has a parent!\n",
                                 menus[s]->ID);
                        menus[s]->parent = menus[m];
                    }
    /* Then check that all the menus are parented correctly */
    /* We don't actually do anything else but alert the user (which should be */
    /* the developer if this message appears...) */
    if (menus[0]->parent != NULL)
        printf ("Warning ! Toplevel menu %d has a parent!\n", menus[0]->ID);
    for (m = 1; m < menucount; m++)
        if (menus[m]->parent == NULL) {
            printf
                ("Warning ! Menu %d has no parent although it should have!\n",
                 menus[m]->ID);
            printf ("(this is a potential memory leak)\n");
        }
    /* The last round, inherit values */
    menu_inherit_values (menus[0]);
    /* Finally create the toplevel menu */
    tl = malloc (sizeof (ToplevelMenu));
    tl->menu = menus[0];
    tl->current_menu = tl->menu;
    tl->current_position = 0;
    while (tl->menu->items[tl->current_position]->disabled)
        tl->current_position++;

    tl->cache = NULL;
    menu_cache_descend (tl);
    return tl;
}

/* Creates a menu. Use this function to create		*/
/* all the menus you use. If your menu has submenus,	*/
/* you have to use the create_toplevel_menu function	*/
/* to actually make them work.				*/
Menu *create_menu (unsigned int id, ...)
{
    Menu *newmenu;
    Menuitemlist *items = NULL, *first = NULL;
    MenuItem *item;
    va_list ap;
    unsigned int type;
    int i;

    /* Create the menu structure */
    newmenu = malloc (sizeof (Menu));
    newmenu->itemcount = 0;
    newmenu->parent = NULL;
    newmenu->ID = id;
    newmenu->predraw = NULL;
    newmenu->drawopts.spacing = -1;
    newmenu->text_enabled = NULL;
    newmenu->text_disabled = NULL;
    /* Create menu items */
    va_start (ap, id);
    while (1) {
        type = va_arg (ap, unsigned int);
        if (type == 0x00)
            break;
        else if (type == MNU_PREDRAW_FUNC)
            newmenu->predraw = va_arg (ap, void *);
        else if (type == MNU_DRAWING_OPTIONS)
            newmenu->drawopts = va_arg (ap, MenuDrawingOptions);
        else if (type == MNU_TOGGLE_TEXTS) {
            newmenu->text_enabled = va_arg (ap, char *);
            newmenu->text_disabled = va_arg (ap, char *);
        } else {                /* Menu item */
            item = malloc (sizeof (MenuItem));
            item->type = type;
            if (items == NULL) {
                items = malloc (sizeof (Menuitemlist));
                items->next = NULL;
                first = items;
            } else {
                items->next = malloc (sizeof (Menuitemlist));
                items = items->next;
                items->next = NULL;
            }
            items->item = item;
            /* Set some defaults for this item */
            memset (item, 0, sizeof (MenuItem));
            item->inc.i = 1;
            if (type == MNU_ITEM_SEP)
                item->disabled = 1;
            item->color = font_color_white;
            item->align = MNU_ALIGN_LEFT;
            /* Get the options for this item */
            newmenu->itemcount++;
            item->type = type;
            /* Get the ID for this menu */
            item->ID = va_arg (ap, unsigned int);
            while ((type = va_arg (ap, unsigned int))) {        /* Item options */
                switch (type) {
                case MNU_OPT_LABEL:
                    item->label.text = va_arg (ap, char *);
                    item->label_type = 1;
                    break;
                case MNU_OPT_LABELF:
                    item->label.function = va_arg (ap, void *);
                    item->label_type = 2;
                    break;
                case MNU_OPT_VALUE_TYPE:
                    item->value_type = va_arg (ap, unsigned int);
                    break;
                case MNU_OPT_VALUE:
                    item->value = va_arg (ap, void *);
                    break;
                case MNU_OPT_MINVALUE:
                    item->min.i = va_arg (ap, int);
                    break;
                case MNU_OPT_MAXVALUE:
                    item->max.i = va_arg (ap, int);
                    break;
                case MNU_OPT_INCREMENT:
                    item->inc.i = va_arg (ap, int);
                    break;
                case MNU_OPT_LOCK:
                    item->lock = va_arg (ap, unsigned int *);
                    break;
                case MNU_OPT_ACTION:
                    item->action = va_arg (ap, void *);
                    break;
                case MNU_OPT_RVAL:
                    item->rval = va_arg (ap, unsigned int);
                    break;
                case MNU_OPT_COLOR:
                    item->color = va_arg (ap, SDL_Color);
                    break;
                case MNU_OPT_ALIGN:
                    item->align = va_arg (ap, unsigned int);
                    break;
                case MNU_OPT_DISABLED:
                    item->disabled = va_arg (ap, unsigned int);
                    break;
                case MNU_OPT_TOGGLE_TEXTS:
                    item->text_enabled = va_arg (ap, char *);
                    item->text_disabled = va_arg (ap, char *);
                    break;
                case MNU_OPT_ICON:
                case MNU_OPT_ICONF:
                case MNU_OPT_FICON:{
                        MenuIconList *newicon =
                            malloc (sizeof (MenuIconList));
                        newicon->type = type;
                        newicon->alignment = va_arg (ap, unsigned int);
                        newicon->icon.get_icon = va_arg (ap, void *);
                        newicon->next = NULL;
                        if (item->icon == NULL) {
                            item->icon = newicon;
                            item->last_icon = newicon;
                        } else {
                            item->last_icon->next = newicon;
                            item->last_icon = newicon;
                        }
                    }
                    break;
                default:
                    printf ("Warning ! Unidentified menu command 0x%x\n",
                            type);
                }
            }
        }
    }
    va_end (ap);
    /* Insert the menu items into the menu */
    newmenu->items = malloc (sizeof (MenuItem *) * newmenu->itemcount);
    if (!newmenu->items) {
        printf ("Malloc error while creating menu %d\n", id);
        perror ("malloc");
        exit (1);
    }
    items = first;
    i = 0;
    while (first) {
        items = first->next;
        newmenu->items[i] = first->item;
        i++;
        free (first);
        first = items;
    }
    return newmenu;
}

/* Create a basic MenuDrawingOptions structure.		*/
/* You should edit it afterwards, changing values for	*/
/* the left/right offsets and color			*/
MenuDrawingOptions make_menu_drawingoptions (int x, int y, int x2, int y2)
{
    MenuDrawingOptions opt;
    opt.area.x = x;
    opt.area.y = y;
    opt.area.w = x2 - x;
    opt.area.h = y2 - y;
    opt.spacing = 20;
    opt.left_offset = 0;
    opt.right_offset = 0;
    opt.selection_color = 0;
    return opt;
}


/* This function frees a toplevel menu, all its		*/
/* submenus and caches properly. None of the data	*/
/* contained in the items of menus are freed.		*/
void free_toplevel_menu (ToplevelMenu * menu)
{
    free_menu (menu->menu);
    while (menu->cache)
        menu_cache_ascend (menu);
    free (menu);
}

/* This function updates the cursor position and	*/
/* executes commands according to the settings in the	*/
/* selected item. Call this from the keyboard handler	*/
/* or whatever your are using.				*/
unsigned int menu_control (ToplevelMenu * menu, MenuCommand cmd)
{
    unsigned int rval = 0;
    MenuItem *item = NULL;
    switch (cmd) {
    case MenuUp:               /* Move cursor up */
        menu->current_position--;
        if (menu->current_position < 0)
            menu->current_position = menu->current_menu->itemcount - 1;
        if (menu->current_menu->items[menu->current_position]->disabled)
            menu_control (menu, MenuUp);
        break;
    case MenuDown:             /* Move cursor down */
        menu->current_position++;
        if (menu->current_position >= menu->current_menu->itemcount)
            menu->current_position = 0;
        if (menu->current_menu->items[menu->current_position]->disabled)
            menu_control (menu, MenuDown);
        break;
    case MenuLeft:             /* Decrease value */
        item = menu->current_menu->items[menu->current_position];
        rval = item->rval;
        if (item->type == MNU_ITEM_VALUE) {
            if (item->value_type == MNU_TYP_CHAR) {
                *(signed char *) item->value -= item->inc.c;
                if (*(signed char *) item->value < item->min.c)
                    *(signed char *) item->value = item->max.c;
            } else if (item->value_type == MNU_TYP_INT) {
                *(int *) item->value -= item->inc.i;
                if (*(int *) item->value < item->min.i)
                    *(int *) item->value = item->max.i;
            } else if (item->value_type == MNU_TYP_FLOAT
                       || item->value_type == MNU_TYP_DOUBLE) {
                *(double *) item->value -= item->inc.d;
                if (*(double *) item->value < item->min.d)
                    *(double *) item->value = item->max.d;
            }
        } else if (item->type == MNU_ITEM_TOGGLE) {
            menu_control (menu, MenuEnter);
        }
        update_menu_cache_item (menu->cache, menu->current_menu,
                                menu->current_position, item);
        break;
    case MenuRight:            /* Increase value */
        item = menu->current_menu->items[menu->current_position];
        rval = item->rval;
        if (item->type == MNU_ITEM_VALUE) {
            if (item->value_type == MNU_TYP_CHAR) {
                *(unsigned char *) item->value += item->inc.c;
                if (*(unsigned char *) item->value > item->max.c)
                    *(signed char *) item->value = item->min.c;
            } else if (item->value_type == MNU_TYP_INT) {
                *(int *) item->value += item->inc.i;
                if (*(int *) item->value > item->max.i)
                    *(int *) item->value = item->min.i;
            } else if (item->value_type == MNU_TYP_FLOAT
                       || item->value_type == MNU_TYP_DOUBLE) {
                *(double *) item->value += item->inc.d;
                if (*(double *) item->value > item->min.d)
                    *(double *) item->value = item->min.d;
            }
        } else if (item->type == MNU_ITEM_TOGGLE) {
            menu_control (menu, MenuEnter);
        }
        update_menu_cache_item (menu->cache, menu->current_menu,
                                menu->current_position, item);
        break;
    case MenuBack:             /* Return to previous menu */
        if (menu->current_menu->parent) {
            menu->current_menu = menu->current_menu->parent;
            menu->current_position = 0;
            if (menu->current_menu->items[0]->disabled)
                menu_control (menu, MenuDown);
            menu_cache_ascend (menu);
        } else
            return menu->current_menu->items[menu->current_position]->rval;
        break;
    case MenuESC:
        if (menu->current_menu->parent)
            menu_control (menu, MenuBack);
        else
            return menu->escvalue;
    case MenuEnter:            /* Execute selection */
        item = menu->current_menu->items[menu->current_position];
        rval = item->rval;
        if (item->type == MNU_ITEM_TOGGLE) {    /* Toggle value */
            switch (item->value_type) {
            case MNU_TYP_CHAR:
                *(unsigned char *) item->value =
                    !*(unsigned char *) item->value;
                break;
            default:
                *(unsigned int *) item->value =
                    !*(unsigned int *) item->value;
                break;
            }
            update_menu_cache_item (menu->cache, menu->current_menu,
                                    menu->current_position, item);
        } else if (item->type == MNU_ITEM_SUBMENU) {    /* Enter a submenu */
            menu->current_menu = item->submenu;
            menu->current_position = 0;
            if (menu->current_menu->items[0]->disabled)
                menu_control (menu, MenuDown);
            menu_cache_descend (menu);
        } else if (item->type == MNU_ITEM_RETURN) {
            menu_control (menu, MenuBack);
        }
        break;
    case MenuToggleLock:       /* Toggle the lock variable */
        item = menu->current_menu->items[menu->current_position];
        if (item->lock) {
            *item->lock = !*item->lock;
        }
        break;
    }
    // If the item has an action, execute it.
    if (item)
        if (item->action)
            if (item->
                action (cmd, menu->current_menu->ID,
                        (struct _MenuItemStruct *) item))
                update_menu_cache_item (menu->cache, menu->current_menu,
                                        menu->current_position, item);
    return rval;
}

/* This function updates the surface cache for the	*/
/* current menu. You only need to call this yourself	*/
/* when some of the values in the current menu or its	*/
/* parents have been changed outside menu_control() .	*/
void update_menu_cache (ToplevelMenu * menu)
{
    int i;
    for (i = 0; i < menu->current_menu->itemcount; i++)
        update_menu_cache_item (menu->cache, menu->current_menu, i,
                                menu->current_menu->items[i]);
}

/* Draw the menu onto the surface 'surface'.		*/
/* The menu is drawn into the area pointed by		*/
/* menu_are rectangle. (Remember to set it !)		*/
void draw_menu (SDL_Surface * surface, ToplevelMenu * menu)
{
    SDL_Rect targ, iconl, iconr;
    SDL_Surface *icon;
    MenuIconList *il;
    MenuDrawingOptions *opts;
    int i;
    /* Call the predraw function */
    if (menu->current_menu->predraw)
        menu->current_menu->predraw (menu->current_menu->ID);
    opts = &menu->current_menu->drawopts;
    targ.x = opts->area.x;
    targ.w = opts->area.w;
    targ.h = opts->spacing;
    targ.y = opts->area.y + menu->current_position * opts->spacing;
    SDL_FillRect (surface, &targ, opts->selection_color);
    for (i = 0; i < menu->current_menu->itemcount; i++) {
        if (menu->cache->items[i] == NULL)
            continue;
        targ.h = opts->spacing;
        targ.w = menu->cache->items[i]->w;
        targ.y = opts->area.y + i * opts->spacing;
        if (menu->cache->align[i] == MNU_ALIGN_LEFT)
            targ.x = opts->area.x + opts->left_offset;
        else if (menu->cache->align[i] == MNU_ALIGN_CENTER)
            targ.x = opts->area.x + opts->area.w / 2 - targ.w / 2;
        else
            targ.x =
                opts->area.x + opts->area.w - opts->right_offset - targ.w;
        SDL_BlitSurface (menu->cache->items[i], NULL, surface, &targ);
        if (menu->current_menu->items[i]->icon) {
            il = menu->current_menu->items[i]->icon;
            iconl.x = targ.x;
            iconl.y = targ.y;
            iconr.x = targ.x + targ.w + 2;
            iconr.y = targ.y;
            while (il) {
                if (il->type == MNU_OPT_ICON) {
                    icon = il->icon.surface;
                    if (!icon) {
                        il = il->next;
                        continue;
                    }
                } else if (il->type == MNU_OPT_ICONF) {
                    icon =
                        il->icon.get_icon (menu->current_menu->ID,
                                           (struct _MenuItemStruct *) menu->
                                           current_menu->items[i]);
                    if (!icon) {
                        il = il->next;
                        continue;
                    }
                } else {
                    icon = NULL;
                }
                if (il->alignment == MNU_ALIGN_LEFT) {
                    if (icon) {
                        iconl.x -= icon->w + 2;
                        SDL_BlitSurface (icon, NULL, surface, &iconl);
                    } else
                        iconl.x -=
                            il->icon.draw_icon (iconl.x, iconl.y,
                                                MNU_ALIGN_LEFT,
                                                menu->current_menu->ID,
                                                (struct _MenuItemStruct *)
                                                menu->current_menu->
                                                items[i]) + 2;
                } else {
                    if (icon) {
                        SDL_BlitSurface (icon, NULL, surface, &iconr);
                        iconr.x += icon->w + 2;
                    } else
                        iconr.x +=
                            il->icon.draw_icon (iconr.x, iconr.y,
                                                MNU_ALIGN_RIGHT,
                                                menu->current_menu->ID,
                                                (struct _MenuItemStruct *)
                                                menu->current_menu->
                                                items[i]) + 2;
                }
                il = il->next;
            }
        }
    }
}

/* Free a menu and all its submenus */
void free_menu (Menu * menu)
{
    int i;
    MenuIconList *tmp;
    for (i = 0; i < menu->itemcount; i++) {
        if (menu->items[i]->type == MNU_ITEM_SUBMENU)
            free_menu (menu->items[i]->submenu);
        while (menu->items[i]->icon) {
            tmp = menu->items[i]->icon->next;
            free (menu->items[i]->icon);
            menu->items[i]->icon = tmp;
        }
        free (menu->items[i]);
    }
    free (menu->items);
    free (menu);
}

/* Internal. */
/* Update a single item in the menu cache. */
static void update_menu_cache_item (MenuCache * cache, Menu * menu, int index,
                                    MenuItem * item)
{
    char value[10], *label = NULL;
    value[0] = '\0';
    /* First free the old surface */
    if (cache->items[index])
        SDL_FreeSurface (cache->items[index]);
    /* Check if the item has a value and if so, construct the proper label */
    if (item->label_type == 1) {
        if (item->type == MNU_ITEM_TOGGLE
            && (menu->text_enabled || item->text_enabled)
            && item->text_enabled != (char *) 1) {
            char *src;
            if (*(char *) item->value) {
                if (item->text_enabled)
                    src = item->text_enabled;
                else
                    src = menu->text_enabled;
            } else {
                if (item->text_disabled)
                    src = item->text_disabled;
                else
                    src = menu->text_disabled;
            }
            label = malloc (strlen (item->label.text) + strlen (src));
            sprintf (label, item->label.text, src);
            value[0] = '0';
        } else {
            if (item->value_type == MNU_TYP_CHAR) {
                sprintf (value, "%d", *(char *) item->value);
                label = malloc (strlen (item->label.text) + strlen (value));
                sprintf (label, item->label.text, *(char *) item->value);
            } else if (item->value_type == MNU_TYP_INT) {
                sprintf (value, "%d", *(int *) item->value);
                label = malloc (strlen (item->label.text) + strlen (value));
                sprintf (label, item->label.text, *(char *) item->value);
            } else if (item->value_type == MNU_TYP_DOUBLE
                       || item->value_type == MNU_TYP_FLOAT) {
                sprintf (value, "%.2f", *(double *) item->value);
                label = malloc (strlen (item->label.text) + strlen (value));
                sprintf (label, item->label.text, *(char *) item->value);
            } else
                label = item->label.text;
        }
    } else if (item->label_type == 2)
        label =
            item->label.function (menu->ID, (struct _MenuItemStruct *) item);
    /* Draw the new menu item */
    if (label) {
        cache->items[index] = renderstring (Bigfont, label, item->color);
        cache->align[index] = item->align;
    } else {
        cache->items[index] = NULL;
        cache->align[index] = 0;
    }
    if (item->label_type == 1 && value[0])
        free (label);
}

/* Internal. */
/* Return one level in the cache */
static void menu_cache_ascend (ToplevelMenu * menu)
{
    int i;
    MenuCache *prev;
    prev = menu->cache;
    menu->cache = prev->parent;
    for (i = 0; i < prev->itemcount; i++)
        SDL_FreeSurface (prev->items[i]);
    free (prev->items);
    free (prev->align);
    free (prev);
}

/* Internal. */
/* Create a new level in the cache */
static void menu_cache_descend (ToplevelMenu * menu)
{
    MenuCache *newcache;
    newcache = malloc (sizeof (MenuCache));
    newcache->parent = menu->cache;
    menu->cache = newcache;
    newcache->itemcount = menu->current_menu->itemcount;
    newcache->items = malloc (sizeof (SDL_Surface *) * newcache->itemcount);
    memset (newcache->items, 0, sizeof (SDL_Surface *) * newcache->itemcount);
    newcache->align = malloc (sizeof (unsigned int) * newcache->itemcount);
    update_menu_cache (menu);
}

/* Internal. */
/* Loop thru the menu tree and do inheritance */
static void menu_inherit_values (Menu * menu)
{
    int i;
    Menu *s;
    for (i = 0; i < menu->itemcount; i++)
        if (menu->items[i]->type == MNU_ITEM_SUBMENU) {
            if (menu->items[i]->submenu == NULL) {
                printf
                    ("Warning ! A submenu item with no submenu detected ! (%d/%d)\n",
                     menu->ID, menu->items[i]->ID);
                printf ("Changing type to label...\n");
                menu->items[i]->type = MNU_ITEM_LABEL;
                continue;
            }
            s = menu->items[i]->submenu;
            if (s->drawopts.spacing == -1)
                s->drawopts = menu->drawopts;
            if (!s->text_enabled)
                s->text_enabled = menu->text_enabled;
            if (!s->text_disabled)
                s->text_disabled = menu->text_disabled;
            menu_inherit_values (s);
        }
}

/* This function returns a pointer to a submenu	with 	*/
/* the ID 'id'. The first occurence of a menu with that	*/
/* ID will be returned.					*/
Menu *find_menu (Menu * menu, int id)
{
    Menu *rm;
    int i;
    if (menu->ID == id)
        return menu;
    for (i = 0; i < menu->itemcount; i++) {
        if (menu->items[i]->type == MNU_ITEM_SUBMENU) {
            if ((rm = find_menu (menu->items[i]->submenu, id)))
                return rm;
        }
    }
    return NULL;
}

/* This function returns the item with the ID 'id'	*/
/* from the menu 'menu'. The first occurence of an	*/
/* item with that ID will be returned.			*/
MenuItem *find_item (Menu * menu, int id)
{
    int i;
    for (i = 0; i < menu->itemcount; i++)
        if (menu->items[i]->ID == id)
            return menu->items[i];
    return NULL;
}

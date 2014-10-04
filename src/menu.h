/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
 *
 * File        : menu.h
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

#ifndef NMENU_H
#define NMENU_H

#include "SDL.h"

#include "list.h"

/* Menu item types */
typedef enum { MNU_ITEM_SEP, MNU_ITEM_LABEL, MNU_ITEM_TOGGLE, MNU_ITEM_VALUE,
    MNU_ITEM_SUBMENU, MNU_ITEM_PARENT, MNU_ITEM_RETURN} MenuItemType;

/* Menu commands */
typedef enum { MNU_UP, MNU_DOWN, MNU_LEFT, MNU_RIGHT, MNU_ENTER, MNU_BACK} MenuCommand;

/* Menu alignments */
typedef enum { MNU_ALIGN_LEFT, MNU_ALIGN_RIGHT, MNU_ALIGN_CENTER } MenuAlign;

/* Menu value types */
typedef enum { MNU_TYP_INT, MNU_TYP_DOUBLE, MNU_TYP_SUBMENU} MenuValueType;

/* Menu item label text */
struct MenuItem;
struct MenuText {
    union {
        const char *text;
        const char *(*function) (struct MenuItem* item);
    } txt;
    enum {MNU_TXT_STR,MNU_TXT_FUNC} type;
    SDL_Color color;
    MenuAlign align;
    SDL_Surface *cache;
};

/* Menu item icon (need here?) */
struct MenuIcon {
    union {
        SDL_Surface *surface;
        SDL_Surface *(*get_icon) (struct MenuItem * item);
        int (*draw_icon) (int x, int y, MenuAlign align,struct MenuItem* item);
    } icon;
    enum {MNU_ICON_SURF,MNU_ICON_GET,MNU_ICON_DRAW} type;
    MenuAlign align;
    int margin;
};

/* Menu item value */
struct MenuValue {
    int *value; /* This is actually struct Menu* in case of submenus */
    int min;
    int max;
    int inc;
};

static const struct MenuValue MnuNullValue = {NULL,0,0,0};

/* Menu item */
struct Menu;
struct MenuItem {
    struct Menu *parent;
    MenuItemType type;
    int ID;
    int sensitive;
    struct MenuText label;
    struct dllist *icons;
    struct MenuValue value;
    const char *text_enabled, *text_disabled;
    /* Function to execute when any command is executed on this item.
     * Should return a positive value if the calling menuitem needs to be
     * refreshed */
    int (*action) (MenuCommand cmd, struct MenuItem * item);
};

/* Menu drawing style */
struct MenuDrawingOptions {
    SDL_Rect area;
    int spacing;
    Uint32 selection_color;
    int left_offset, right_offset;
};

/* Menu */
struct Menu {
    int ID;
    struct Menu *parent;
    struct dllist *children;

    struct MenuDrawingOptions options;
    void (*predraw) (struct Menu *menu);

    int selection;
    
    /* Value to return when leaving the menu */
    int escvalue;

    /* Default, inherited by children whose text_* is NULL */
    const char *text_enabled, *text_disabled;

};

/* Create a new menu */
extern struct Menu *create_menu(int id,struct Menu *parent,
        struct MenuDrawingOptions *options, char *text_enabled,
        char *text_disabled,int escvalue);

/* Add a new menu item to a menu */
extern struct MenuItem *add_menu_item(struct Menu *menu,MenuItemType type,
        int id, struct MenuText text, struct MenuValue value);

/* Free a menu and its children */
extern void free_menu(struct Menu *menu);

/* Free all cached surfaces from a menu and its children */
extern void flush_menu(struct Menu *menu);

/* Control a menu. If a submenu is entered, the menu pointer will be
 * set accordingly. */
extern int menu_control(struct Menu **menu, MenuCommand cmd);

/* Draw the menu on a surface */
extern void draw_menu(SDL_Surface *surface, struct Menu *menu);

/* Get an menu item by index number */
extern struct MenuItem *get_menu_item(struct Menu *menu,int index);

/* Convenience functions */
/* Return a text menu label with default alignment and color */
extern struct MenuText menu_txt_label(const char *text);
extern struct MenuText menu_func_label(const char *(*function) (struct MenuItem*));
extern struct MenuIcon *menu_icon_get(SDL_Surface *(*get_icon)(struct MenuItem *item));
extern struct MenuIcon *menu_icon_draw(int (*draw_icon) (int x, int y, MenuAlign align,struct MenuItem* item));

/* Create a menu item that leads to a submenu */
extern struct MenuItem *add_submenu(struct Menu *m, int id,const char *label,struct Menu *sub);

#endif


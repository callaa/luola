/*
 * Luola - 2D multiplayer cavern-flying game
 * Copyright (C) 2003 Calle Laakkonen
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

#ifndef MENU_H
#define MENU_H

#include "SDL.h"

/* Menu options */
#define MNU_PREDRAW_FUNC	0x01    /* The function to call before drawing this menu */
#define MNU_DRAWING_OPTIONS	0x02    /* Set the drawing options for this menu */
#define MNU_TOGGLE_TEXTS	0x03    /* Set the 'enabled' and 'disabled' texts to append to toggle buttons */
/* Menu types */
#define MNU_ITEM_SEP		0x10    /* Separator */
#define MNU_ITEM_LABEL		0x11    /* Label menu item */
#define MNU_ITEM_TOGGLE		0x12    /* Toggle-able menu item */
#define MNU_ITEM_VALUE		0x13    /* Value item */
#define MNU_ITEM_SUBMENU	0x14    /* Enter a submenu */
#define MNU_ITEM_RETURN		0x15    /* Return to the parent menu */
/* Menu item options */
#define MNU_OPT_LABEL		0x10    /* The label of this menu item (char* type) */
#define MNU_OPT_LABELF		0x11    /* The function to call to get the text of this menu item */
#define MNU_OPT_VALUE_TYPE	0x12    /* Type of the value */
#define MNU_OPT_VALUE		0x13    /* Pointer to the value */
#define MNU_OPT_MINVALUE	0x14    /* Minimium value */
#define MNU_OPT_MAXVALUE	0x15    /* Maximium value */
#define MNU_OPT_INCREMENT	0x16    /* Value increment (for left/right keys) */
#define MNU_OPT_LOCK		0x17    /* Pointer to the lock variable (not yet used in Luola) */
#define MNU_OPT_ACTION		0x18    /* Function to execute when enter is pressed over this item */
#define MNU_OPT_RVAL		0x19    /* The value returned when this menu item is activated */
#define MNU_OPT_COLOR		0x1A    /* The color of this menu item */
#define MNU_OPT_ALIGN		0x1B    /* How the text is aligned */
#define MNU_OPT_DISABLED	0x1C    /* Disable this entry */
#define MNU_OPT_TOGGLE_TEXTS	0x1D    /* Set special texts for this toggle button */
#define MNU_OPT_ICON		0x1E    /* Add an icon to this entry */
#define MNU_OPT_ICONF		0x1F    /* Add a function to return the icon for this entry */
#define MNU_OPT_FICON		0x20    /* Add a function to draw an icon for this entry */

/* Menu item option types */
#define MNU_TYP_CHAR		0x10
#define MNU_TYP_INT		0x11
#define MNU_TYP_FLOAT		0x12
#define MNU_TYP_DOUBLE		0x13
/* Alignments */
#define MNU_ALIGN_LEFT		0x10
#define MNU_ALIGN_CENTER	0x11
#define MNU_ALIGN_RIGHT		0x12

/* Menu commands */
typedef enum { MenuUp, MenuDown, MenuLeft, MenuRight, MenuEnter, MenuBack,
        MenuESC, MenuToggleLock } MenuCommand;

/* Forward declarations */
struct _MenuStruct;
struct _MenuItemStruct;

/** Unions **/
/* Menu label */
typedef union {
    char *text;
    char *(*function) (unsigned int mid, struct _MenuItemStruct * item);
} MenuText;
/* Used for values when we don't know what it is */
typedef union {
    unsigned char c;
    int i;
    float f;
    double d;
} IntDouble;

/* Menu icon */
/* draw_icon should return the width of the icon drawn */
typedef union {
    SDL_Surface *surface;
    SDL_Surface *(*get_icon) (unsigned int mid,
                              struct _MenuItemStruct * item);
    int (*draw_icon) (int x, int y, unsigned int align, unsigned int mid,
                      struct _MenuItemStruct * item);
} MenuIcon;

/** Structures **/
/* You should not need to touch these structures yourself.	*/
/* The support functions should be enough. 			*/
/* (Except for MenuDrawingOptions)				*/

/* Menu drawing style structure. This is something you may	*/
/* handle yourself. Note that if a submenu doesn't have		*/
/* this structure set, it inherits it from its parent.		*/
/* Meaning that, you MUST set this for the toplevel menu.	*/
/* Otherwise expect strange behaviour...			*/
typedef struct {
    SDL_Rect area;
    int spacing;
    Uint32 selection_color;
    int left_offset, right_offset;
} MenuDrawingOptions;

/* List of icons assigned to a menu entry */
typedef struct _MenuIconListStruct {
    MenuIcon icon;
    unsigned int type;
    unsigned int alignment;
    struct _MenuIconListStruct *next;
} MenuIconList;

/* Menu cache structure */
/* This is used to store the surfaces for the current menu and its parents. */
typedef struct _MenuCacheStruct {
    SDL_Surface **items;
    unsigned int *align;
    int itemcount;
    struct _MenuCacheStruct *parent;
} MenuCache;

/* Menu item structure */
typedef struct {
    unsigned int type;
    unsigned int ID;
    unsigned int disabled;
    unsigned int *lock;
    /* Label */
    MenuText label;
    unsigned int label_type;
    SDL_Color color;
    unsigned int align;
    char *text_enabled, *text_disabled;
    /* Icon */
    MenuIconList *icon, *last_icon;
    /* Value */
    void *value;
    unsigned int value_type;
    IntDouble min;
    IntDouble max;
    IntDouble inc;
    /* Function to execute when enter is pressed with this menu item */
    /* Should return a positive value if the calling menuitem needs to be refreshed */
    unsigned int (*action) (MenuCommand cmd, unsigned int mid,
                            struct _MenuItemStruct * item);
    /* Submenu */
    struct _MenuStruct *submenu;
    /* Return value */
    unsigned int rval;
} MenuItem;

/* Menu structure */
typedef struct _MenuStruct {
    unsigned int ID;
    MenuItem **items;
    struct _MenuStruct *parent;
    int itemcount;
    void (*predraw) (unsigned int id);
    MenuDrawingOptions drawopts;
    char *text_enabled, *text_disabled; /* Default */
} Menu;

/* Toplevel menu */
typedef struct {
    Menu *menu;
    Menu *current_menu;
    MenuCache *cache;
    int current_position;
    unsigned int escvalue;      /* Default return value when ESC is pressed */
} ToplevelMenu;

/** Functions **/
/* This function creates a new menu and its items	*/
/* according to to the parameters it gets.		*/
/* It is highly recommended that you use this function	*/
/* to create the menus. In order to actually use the	*/
/* menu, you must use the create_toplevel_menu as well	*/
extern Menu *create_menu (unsigned int id, ...);

/* This function creates the toplevel menu which is	*/
/* used by the menu handling commands. The first menu	*/
/* in the list should be the toplevel menu. The rest	*/
/* are submenus. Submenus should have only one parent	*/
/* and the toplevel menu should have none.		*/
extern ToplevelMenu *create_toplevel_menu (int menucount, Menu ** menus);

/* This function frees a menu and all its submenus	*/
extern void free_menu (Menu * menu);

/* This function frees a toplevel menu and all of its	*/
/* submenus and caches properly. None of the data	*/
/* contained in the items of menus are freed.		*/
extern void free_toplevel_menu (ToplevelMenu * menu);

/* This function updates the cursor position and	*/
/* executes commands according to the settings in the	*/
/* selected item. Call this from the keyboard handler	*/
/* or whatever your are using.				*/
extern unsigned int menu_control (ToplevelMenu * menu, MenuCommand cmd);

/* This function updates the surface cache for the	*/
/* current menu. You only need to call this yourself	*/
/* when some of the values in the current menu or its	*/
/* parents have been changed outside menu_control() .	*/
extern void update_menu_cache (ToplevelMenu * menu);

/* Create a basic MenuDrawingOptions structure.		*/
/* You should edit it afterwards, changing values for	*/
/* the left/right offsets and color			*/
extern MenuDrawingOptions make_menu_drawingoptions (int x, int y, int x2,
                                                    int y2);

/* Draw the menu onto the surface 'surface'.		*/
/* The menu is drawn into the area pointed by		*/
/* menu_are rectangle. (Remember to set it !)		*/
extern void draw_menu (SDL_Surface * surface, ToplevelMenu * menu);

/* This function returns a pointer to a menu with the 	*/
/* ID 'id'. The first occurence of a menu with that	*/
/* ID will be returned.					*/
extern Menu *find_menu (Menu * menu, int id);

/* This function returns the item with the ID 'id'	*/
/* from the menu 'menu'. The first occurence of an	*/
/* item with that ID will be returned.			*/
extern MenuItem *find_item (Menu * menu, int id);

#endif

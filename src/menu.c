/*
 * Luola - 2D multiplayer cave-flying game
 * Copyright (C) 2003-2006 Calle Laakkonen
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

#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "font.h"
#include "console.h"

/* Create and initialize a new menu */
struct Menu *create_menu(int id,struct Menu *parent,
        struct MenuDrawingOptions *options, char *text_enabled,
        char *text_disabled,int escvalue)
{
    struct Menu *menu;

    menu=malloc(sizeof(struct Menu));
    if(!menu) {
        perror("create_menu");
        return NULL;
    }

    menu->ID = id;
    menu->parent = parent;
    menu->children = NULL;
    menu->escvalue = escvalue;

    if(!options) {
        if(!parent) {
            fprintf(stderr,"Error: create_menu(%d): no drawing options provided!\n",id);
            free(menu);
            return NULL;
        }
        options = &parent->options;
    }
    menu->options = *options;
    menu->predraw = NULL;

    menu->selection = 0;
    if(!text_enabled) {
        if(!parent) {
            fprintf(stderr,"Error: create_menu(%d): no enabled text provided!\n",id);
            free(menu);
            return NULL;
        }
        menu->text_enabled = parent->text_enabled;
    } else {
        menu->text_enabled = text_enabled;
    }

    if(!text_disabled) {
        if(!parent) {
            fprintf(stderr,"Error: create_menu(%d): no disabled text provided!\n",id);
            free(menu);
            return NULL;
        }
        menu->text_disabled = parent->text_disabled;
    } else {
        menu->text_disabled = text_disabled;
    }

    return menu;
}

/* Redraw menu item text */
static void item_update_cache(struct MenuItem *i) {
    const char *format=NULL;
    if(i->label.cache)
        SDL_FreeSurface(i->label.cache);
    switch(i->label.type) {
        case MNU_TXT_STR: format = i->label.txt.text; break;
        case MNU_TXT_FUNC: format = i->label.txt.function(i); break;
    }
    if(format) {
        if(i->value.value && i->type!=MNU_ITEM_SUBMENU) {
            char str[256];
            if(i->type==MNU_ITEM_TOGGLE) {
                if(*i->value.value)
                    sprintf(str,format,i->text_enabled);
                else
                    sprintf(str,format,i->text_disabled);
            } else {
                sprintf(str,format,*i->value.value);
            }
            i->label.cache = renderstring(Bigfont, str, i->label.color);
        } else {
            i->label.cache = renderstring(Bigfont, format, i->label.color);
        }
    } else {
        i->label.cache = NULL;
    }
}

/* Create a new menu item and add it to a menu */
struct MenuItem *add_menu_item(struct Menu *menu,MenuItemType type, int id,
        struct MenuText text, struct MenuValue value)
{
    struct MenuItem *item;
    item = malloc(sizeof(struct MenuItem));
    if(!item) {
        perror("add_menu_item");
        return NULL;
    }
    if(menu->children)
        dllist_append(menu->children,item);
    else
        menu->children = dllist_append(menu->children,item);
    item->parent = menu;
    item->type = type;
    item->ID = id;
    item->sensitive = type!=MNU_ITEM_SEP;
    item->label = text;
    item->icons = NULL;
    item->value = value;
    item->text_enabled = menu->text_enabled;
    item->text_disabled = menu->text_disabled;
    item->action = NULL;
    return item;
}

/* Free a menu item (called by dllist_free from free_menu) */
static void free_menu_item(void *data) {
    struct MenuItem *item=data;
    if(item->label.cache)
        SDL_FreeSurface(item->label.cache);
    dllist_free(item->icons,free);
    if(item->type == MNU_ITEM_SUBMENU)
        free_menu((struct Menu*)item->value.value);
    free(item);
}

/* Free a menu and its children */
void free_menu(struct Menu *menu)
{
    if(menu) {
        dllist_free(menu->children,free_menu_item);
        free(menu);
    }
}

/* Decrease menu item value */
static void decrease_value(struct MenuItem *i) {
    *i->value.value -= i->value.inc;
    if(*i->value.value < i->value.min)
        *i->value.value = i->value.max;
}

/*  Increase menu item value */
static void increase_value(struct MenuItem *i) {
    *i->value.value += i->value.inc;
    if(*i->value.value > i->value.max)
        *i->value.value = i->value.min;
}

/* Toggle menu item value */
static void toggle_value(struct MenuItem *i) {
    *i->value.value = !*i->value.value;
}

/* Recursively free all cached surfaces from this menu and all its submenus */
void flush_menu(struct Menu *menu) {
    struct dllist *child=menu->children;
    while(child) {
        struct MenuItem *i=child->data;
        if(i->label.cache) {
            SDL_FreeSurface(i->label.cache);
            i->label.cache = NULL;
        }
        if(i->type == MNU_ITEM_SUBMENU)
            flush_menu((struct Menu*)i->value.value);
        child=child->next;
    }
}

/* Check if menu item can be selected */
static int is_sensitive(struct Menu *menu, int item) {
    struct dllist *list=menu->children;
    for(;item>0;item--)
        list=list->next;
    return ((struct MenuItem*)list->data)->sensitive;
}

/* Control a menu */
int menu_control(struct Menu **menu, MenuCommand cmd) {
    struct Menu *m=*menu;
    struct dllist *sellist=m->children;
    struct MenuItem *sel=NULL;
    int r;
    if(sellist) {
        for(r=0;r<m->selection;r++) {
            if(sellist->next) {
                sellist=sellist->next;
            } else {
                fprintf(stderr,"Bug! menu_control(): menu item %d doesn't exist!\n",m->selection);
                break;
            }
        }
        sel = sellist->data;
    }
    switch(cmd) {
        case MNU_UP: /* Move selection up */
            if(m->selection==0)
                m->selection=dllist_count(m->children)-1;
            else
                m->selection--;
            if(!is_sensitive(m,m->selection))
                menu_control(menu,cmd);
            break;
        case MNU_DOWN: /* Move selection down */
            if(m->selection==dllist_count(m->children)-1)
                m->selection=0;
            else
                m->selection++;
            if(!is_sensitive(m,m->selection))
                menu_control(menu,cmd);
            break;
        case MNU_LEFT: /* Decrease value */
            if(sel->value.value) {
                if(sel->type==MNU_ITEM_TOGGLE) {
                    menu_control(menu,MNU_ENTER);
                } else {
                    decrease_value(sel);
                    item_update_cache(sel);
                }
            }
            break;
        case MNU_RIGHT: /* Increase value */
            if(sel->value.value) {
                if(sel->type==MNU_ITEM_TOGGLE) {
                    menu_control(menu,MNU_ENTER);
                } else {
                    increase_value(sel);
                    item_update_cache(sel);
                }
            }
            break;
        case MNU_BACK: /* Return to parent menu */
            if(m->parent) {
                flush_menu(m);
                *menu = m->parent;
                break;
            }
            return m->escvalue;
        case MNU_ENTER: /* Execute selection */
            if(sel->type==MNU_ITEM_TOGGLE) {
                toggle_value(sel);
                item_update_cache(sel);
            } else if(sel->type==MNU_ITEM_SUBMENU) {
                *menu = (struct Menu*)sel->value.value;
                (*menu)->selection=0;
                if(is_sensitive(*menu,(*menu)->selection)==0)
                    menu_control(menu,MNU_DOWN);
            } else if(sel->type==MNU_ITEM_PARENT) {
                menu_control(menu,MNU_BACK);
            } else if(sel->type==MNU_ITEM_RETURN) {
                return sel->ID;
            }
            break;
    }
    /* Execute action if any */
    if(sel->action) { 
        if(sel->action(cmd,sel))
            item_update_cache(sel);
    }
    return 0;
}

/* Draw a single menu item */
static void draw_menu_item(SDL_Surface *surface, SDL_Rect area,
        struct MenuItem *item)
{
    struct dllist *icon=item->icons;
    int leftx,rightx;

    /* Draw the label */
    if(item->label.txt.text || item->label.txt.function) {
        if(item->label.cache==NULL)
            item_update_cache(item);
        switch(item->label.align) {
            case MNU_ALIGN_LEFT:
                area.x += item->parent->options.left_offset;
                break;
            case MNU_ALIGN_CENTER:
                area.x += area.w/2 - item->label.cache->w/2;
                break;
            case MNU_ALIGN_RIGHT:
                area.x += area.w - item->parent->options.right_offset -
                    item->label.cache->w;
                break;
        }
        SDL_BlitSurface(item->label.cache,NULL,surface,&area);
    }

    /* Draw icons */
    leftx = area.x;
    rightx = leftx + (item->label.cache?item->label.cache->w:0);
    while(icon) {
        struct MenuIcon *i = icon->data;
        SDL_Surface *iconsurf=i->icon.surface;
        switch(i->type) {
            case MNU_ICON_GET:
                iconsurf = i->icon.get_icon(item);
            case MNU_ICON_SURF:
                if(i->align==MNU_ALIGN_LEFT) {
                    leftx -= iconsurf->w + i->margin;
                    area.x = leftx;
                } else {
                    area.x = rightx + i->margin;
                    rightx += iconsurf->w + i->margin;
                }
                SDL_BlitSurface(iconsurf,NULL,surface,&area);
                break;
            case MNU_ICON_DRAW:
                if(i->align==MNU_ALIGN_LEFT)
                    leftx -= i->icon.draw_icon(leftx-i->margin,
                            area.y,i->align,item)+i->margin;
                else
                    rightx += i->icon.draw_icon(rightx+i->margin,
                            area.y,i->align,item)+i->margin;
                break;
        }
        icon=icon->next;
    }
}

/* Draw the menu */
void draw_menu(SDL_Surface *surface, struct Menu *menu) {
    SDL_Rect rect;
    struct dllist *item;

    /* Call the predraw function if any */
    if(menu->predraw)
        menu->predraw(menu);

    /* Draw the cursor */
    rect.x = menu->options.area.x;
    rect.w = menu->options.area.w;
    rect.h = menu->options.spacing;
    rect.y = menu->options.area.y+rect.h*menu->selection;
    fill_box(surface,rect.x,rect.y,rect.w,rect.h,menu->options.selection_color);

    /* Draw menu items */
    item = menu->children;
    rect.y = menu->options.area.y;
    while(item) {
        draw_menu_item(surface,rect,item->data);
        rect.y += rect.h;
        item=item->next;
    }

}

/* Get an menu item by index number */
struct MenuItem *get_menu_item(struct Menu *menu,int index) {
    struct dllist *list = menu->children;
    for(;index>0;index--) {
        if(list->next==NULL) return NULL;
        list=list->next;
    }
    return list->data;
}

/* Return a text menu label with default alignment and color */
struct MenuText menu_txt_label(const char *text) {
    struct MenuText txt;
    txt.type = MNU_TXT_STR;
    txt.txt.text = text;
    txt.color = font_color_white;
    txt.align = MNU_ALIGN_LEFT;
    txt.cache = NULL;
    return txt;
}

struct MenuText menu_func_label(const char *(*function) (struct MenuItem*)) {
    struct MenuText txt;
    txt.type = MNU_TXT_FUNC;
    txt.txt.function = function;
    txt.color = font_color_white;
    txt.align = MNU_ALIGN_LEFT;
    txt.cache = NULL;
    return txt;
}

struct MenuIcon *menu_icon_get(SDL_Surface *(*get_icon)(struct MenuItem *item))
{
    struct MenuIcon *icon;
    icon = malloc(sizeof(struct MenuIcon));
    if(icon) {
        icon->type = MNU_ICON_GET;
        icon->icon.get_icon = get_icon;
        icon->align = MNU_ALIGN_LEFT;
        icon->margin = 10;
    }
    return icon;
}

struct MenuIcon *menu_icon_draw(int (*draw_icon) (int x, int y,
            MenuAlign align,struct MenuItem* item))
{
    struct MenuIcon *icon;
    icon = malloc(sizeof(struct MenuIcon));
    if(icon) {
        icon->type = MNU_ICON_DRAW;
        icon->icon.draw_icon = draw_icon;
        icon->align = MNU_ALIGN_RIGHT;
        icon->margin = 10;
    }
    return icon;
}


struct MenuItem *add_submenu(struct Menu *m,int id, const char *label,struct Menu *sub) {
    struct MenuValue val;
    val.value = (int*)sub;
    return add_menu_item(m,MNU_ITEM_SUBMENU,id,menu_txt_label(label),val);
}


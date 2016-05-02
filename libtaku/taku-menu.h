/* 
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef HAVE_TAKU_MENU_H
#define HAVE_TAKU_MENU_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TAKU_TYPE_MENU (taku_menu_get_type ())

#define TAKU_MENU(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TAKU_TYPE_MENU, TakuMenu))

#define TAKU_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TAKU_TYPE_MENU, TakuMenuClass))

#define TAKU_IS_MENU(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TAKU_TYPE_MENU))

#define TAKU_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TAKU_TYPE_MENU))

#define TAKU_MENU_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TAKU_TYPE_MENU, TakuMenuClass))

typedef struct _TakuMenu TakuMenu;
typedef struct _TakuMenuClass TakuMenuClass;
typedef struct _TakuMenuPrivate TakuMenuPrivate;
typedef struct _TakuMenuItem TakuMenuItem;

struct _TakuMenu
{
  GObject         parent;

  /* private */
  TakuMenuPrivate   *priv;
};

struct _TakuMenuClass 
{
  /* private */
  GObjectClass    parent_class;

  /* signals */
  void (*item_added) (TakuMenu *menu, TakuMenuItem *item);
  void (*item_removed) (TakuMenu *menu, TakuMenuItem *item);
  
  /* future padding */
  void (*_taku_menu_1) (void);
  void (*_taku_menu_2) (void);
  void (*_taku_menu_3) (void);
  void (*_taku_menu_4) (void);
};

GType taku_menu_get_type (void) G_GNUC_CONST;

/*< Menu functions />*/

/* 
 * Expected to create a list of TakuTakuCategorys and TakuMenuItems 
 */
TakuMenu*
taku_menu_get_default (void);

/* 
 * Returns a list of TakuTakuCategorys 
 */
GList*
taku_menu_get_categories (TakuMenu *menu);

/* 
 * Returns a list of TakuMenuItems 
 */
GList*
taku_menu_get_items (TakuMenu *menu);


/*< MenuItem functions />*/

const gchar*
taku_menu_item_get_name (TakuMenuItem *item);

const gchar*
taku_menu_item_get_description (TakuMenuItem *item);

GdkPixbuf*
taku_menu_item_get_icon (TakuMenuItem *item, 
                         int           size);

GList*
taku_menu_item_get_categories (TakuMenuItem *item);

gboolean
taku_menu_item_launch (TakuMenuItem *item, GtkWidget *widget);

const gchar*
taku_menu_desktop_get_executable (TakuMenuItem *item);


G_END_DECLS

#endif

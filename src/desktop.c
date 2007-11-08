/* -*- mode: c -*- */
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

/* TODO: turn this into a GtkWindow subclass? */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <ctype.h>

#include "libtaku/taku-menu.h"
#include "libtaku/taku-table.h"
#include "libtaku/taku-icon-tile.h"
#include "libtaku/taku-launcher-tile.h"
#include "taku-category-bar.h"

#include "libtaku/xutil.h"

#include "desktop.h"

static GList *categories;
static TakuCategoryBar *bar;
static TakuTable *table;
static TakuMenu *menu;

static void
on_item_added (TakuMenu *menu, TakuMenuItem *item, gpointer null)
{
  GtkWidget *tile;

  tile = taku_launcher_tile_new_from_item (item);
  
  if (GTK_IS_WIDGET (tile))
    gtk_container_add (GTK_CONTAINER (table), tile); 
}

static void
on_item_removed (TakuMenu *menu, TakuMenuItem *item, gpointer null)
{
  TakuLauncherTile *tile = NULL;
  GList *tiles, *t;

  tiles = gtk_container_get_children (GTK_CONTAINER (table));
  for (t = tiles; t; t = t->next)
  {
    if (!TAKU_IS_LAUNCHER_TILE (t->data))
      continue;

    if (item == taku_launcher_tile_get_item (t->data))
    {
      tile = t->data;
      break;
    }
  }

  if (GTK_IS_WIDGET (tile))
    gtk_container_remove (GTK_CONTAINER (table), GTK_WIDGET (tile));
}


/* Handle failed focus events by switching between categories */
static gboolean
focus_cb (GtkWidget *widget, GtkDirectionType direction, gpointer user_data)
{
  /* If there are no categories, don't try switching */
  if (categories == NULL)
    return FALSE;

  if (direction == GTK_DIR_LEFT) {
    taku_category_bar_previous (bar);

    gtk_widget_grab_focus (GTK_WIDGET (table));

    return TRUE;
  } else if (direction == GTK_DIR_RIGHT) {
    taku_category_bar_next (bar);

    gtk_widget_grab_focus (GTK_WIDGET (table));

    return TRUE;
  }

  return FALSE;
}

/*
 * Load all .desktop files in @datadir/applications/.
 */
static void
load_items (TakuMenu *menu)
{
  GList *l;

  for (l = taku_menu_get_items (menu); l; l = l->next)
  {
    GtkWidget *tile;

    if (!l->data)
      continue;
    tile = taku_launcher_tile_new_from_item (l->data);
    if (GTK_IS_WIDGET (tile))
      gtk_container_add (GTK_CONTAINER (table), tile);
  }

}

GtkWidget *
create_desktop (void)
{
  GtkWidget *window, *box, *scrolled, *viewport, *fixed;

  /* Sane defaults in case something terrible happens in x_get_workarea() */
  int x = 0, y = 0;
  int w = 640, h = 480;

  /* Register the magic taku-icon size so that it can be controlled from the
     theme. */
  gtk_icon_size_register ("taku-icon", 64, 64);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_set_name (window, "TakuWindow");
  gtk_window_set_title (GTK_WINDOW (window), _("Desktop"));

#ifndef STANDALONE
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DESKTOP);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
  x_get_workarea (&x, &y, &w, &h);
#else
  gtk_window_set_default_size (GTK_WINDOW (window), w, h);
#endif

  gtk_widget_show (window);

#if BROKEN_WORKAREA
  /* This fixed is used to position the desktop itself in the work area */
  fixed = gtk_fixed_new ();
  gtk_widget_show (fixed);
  gtk_container_add (GTK_CONTAINER (window), fixed);
#endif

  /* Main VBox */
  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
#if BROKEN_WORKAREA
  gtk_widget_set_size_request (box, w, h);
  gtk_fixed_put (GTK_FIXED (fixed), box, x, y);
#else
  gtk_container_add (GTK_CONTAINER (window), box);
#endif

  /* Navigation bar */
  bar = TAKU_CATEGORY_BAR (taku_category_bar_new ());
  gtk_widget_show (GTK_WIDGET (bar));
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (bar), FALSE, TRUE, 0);

  /* Table area */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
                                GTK_SHADOW_NONE);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolled), viewport);

  table = TAKU_TABLE (taku_table_new ());
  g_signal_connect_after (table, "focus", G_CALLBACK (focus_cb), NULL);
  gtk_widget_show (GTK_WIDGET (table));
  gtk_container_add (GTK_CONTAINER (viewport), GTK_WIDGET (table));

  menu = taku_menu_get_default ();
  categories = taku_menu_get_categories (menu);

  taku_category_bar_set_table (bar, table);
  taku_category_bar_set_categories (bar, categories);

  g_signal_connect (menu, "item-added", G_CALLBACK (on_item_added), NULL);
  g_signal_connect (menu, "item-removed", G_CALLBACK (on_item_removed), NULL);  
  
  load_items (menu);

  return window;
}

void
destroy_desktop (void)
{
  while (categories) {
    TakuLauncherCategory *category = categories->data;
    
    taku_launcher_category_free (category);

    categories = g_list_delete_link (categories, categories);
  }
  g_object_unref (menu);
}

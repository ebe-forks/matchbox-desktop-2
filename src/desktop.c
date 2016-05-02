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
#include "libtaku/taku-icon-tile.h"
#include "libtaku/taku-launcher-tile.h"
#include "taku-category-bar.h"

#include "libtaku/xutil.h"

#include "desktop.h"

static GList *categories;
static TakuCategoryBar *bar;
static GtkWidget *table;
static TakuMenu *menu;
static GtkWidget *fixed, *box;

static void
on_item_added (TakuMenu *menu, TakuMenuItem *item, gpointer null)
{
  GtkWidget *tile;

  tile = taku_launcher_tile_new_from_item (item);

  if (tile) {
    gtk_widget_show (tile);
    gtk_container_add (GTK_CONTAINER (table), tile);
  }
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
    gtk_widget_child_focus (table, GTK_DIR_LEFT);
    return TRUE;
  } else if (direction == GTK_DIR_RIGHT) {
    taku_category_bar_next (bar);
    gtk_widget_child_focus (table, GTK_DIR_RIGHT);
    return TRUE;
  } else if (direction == GTK_DIR_UP) {
    gtk_flow_box_unselect_all (GTK_FLOW_BOX (table));
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
    if (tile) {
      gtk_widget_show (tile);
      gtk_container_add (GTK_CONTAINER (table), tile);
    }
  }
}

static gboolean
delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  /* prevent default handler from destroying the window */
  return TRUE;
}

static void
workarea_changed (int x, int y, int w, int h)
{
  gtk_widget_set_size_request (box, w, h);
  gtk_widget_queue_resize (box);
  gtk_fixed_move (GTK_FIXED (fixed), box, x, y);
}

static void table_size_allocate_cb (GtkWidget    *table,
                                    GdkRectangle *allocation,
                                    gpointer      user_data)
{
  gtk_flow_box_set_min_children_per_line (GTK_FLOW_BOX (table), allocation->width / 200); 
}

static gboolean
table_filter (GtkFlowBoxChild *child, gpointer user_data)
{
  TakuTile *tile = TAKU_TILE (gtk_bin_get_child (GTK_BIN (child)));

  return taku_tile_matches_filter (tile, taku_category_bar_get_current (bar));
}

static gint
table_sort (GtkFlowBoxChild *child1,
            GtkFlowBoxChild *child2,
            gpointer user_data)
{
  GtkWidget *a, *b;
  const char *ka, *kb;

  a = gtk_bin_get_child (GTK_BIN(child1));
  b = gtk_bin_get_child (GTK_BIN(child2));
  ka = taku_tile_get_sort_key (TAKU_TILE (a));
  kb = taku_tile_get_sort_key (TAKU_TILE (b));

  if (ka != NULL && kb == NULL)
    return 1;
  else if (ka == NULL && kb != NULL)
    return -1;
  else if (ka == NULL && kb == NULL)
    return 0;
  else
    return strcmp (ka, kb);
}

static void
table_child_activated_cb (GtkFlowBox *table,
                          GtkFlowBoxChild *child,
                          gpointer userdata)
{
  GtkWidget *tile;

  tile = gtk_bin_get_child (GTK_BIN (child));
  if (TAKU_IS_LAUNCHER_TILE (tile))
    taku_launcher_tile_activate (TAKU_LAUNCHER_TILE (tile));
}

GtkWidget *
create_desktop (DesktopMode mode)
{
  GtkWidget *window, *scrolled, *viewport;
  GdkScreen *screen;
  int width, height;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (window, "TakuWindow");
  gtk_window_set_title (GTK_WINDOW (window), _("Desktop"));

  screen = gtk_widget_get_screen (window);

  /* Main VBox */
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (box);

  switch (mode) {
  case MODE_DESKTOP:
  case MODE_TITLEBAR:
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    g_signal_connect (window, "delete-event", G_CALLBACK (delete_event_cb), NULL);

    width = gdk_screen_get_width (screen);
    height = gdk_screen_get_height (screen);
    gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    gtk_window_move (GTK_WINDOW (window), 0, 0);

    /* This fixed is used to position the desktop itself in the work area */
    fixed = gtk_fixed_new ();
    gtk_widget_show (fixed);
    gtk_container_add (GTK_CONTAINER (window), fixed);
    gtk_fixed_put (GTK_FIXED (fixed), box, 0, 0);

    /* Set a sane default in case there is no work area defined yet */
    workarea_changed (0, 0, width, height);
    if (mode == MODE_DESKTOP || mode == MODE_TITLEBAR) {
      x_monitor_workarea (screen, workarea_changed);
    }

    break;
  case MODE_WINDOW:
    width = 640;
    height = 480;
    gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_add (GTK_CONTAINER (window), box);
    break;
  default:
    g_assert_not_reached ();
  }
  gtk_widget_show (window);


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
  gtk_widget_set_valign (viewport, GTK_ALIGN_START);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
                                GTK_SHADOW_NONE);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolled), viewport);

  table = gtk_flow_box_new ();
  gtk_flow_box_set_homogeneous (GTK_FLOW_BOX (table), TRUE);
  gtk_flow_box_set_row_spacing (GTK_FLOW_BOX (table), 6);
  gtk_flow_box_set_column_spacing (GTK_FLOW_BOX (table), 6);
  g_signal_connect (table, "child-activated",
                    G_CALLBACK (table_child_activated_cb), NULL);
  g_signal_connect (table, "size-allocate",
                    G_CALLBACK (table_size_allocate_cb), NULL);
  g_signal_connect_after (table, "focus", G_CALLBACK (focus_cb), NULL);
  gtk_flow_box_set_filter_func (GTK_FLOW_BOX (table),
                                (GtkFlowBoxFilterFunc)table_filter,
                                bar, NULL);
  gtk_flow_box_set_sort_func (GTK_FLOW_BOX (table),
                              (GtkFlowBoxSortFunc)table_sort,
                              NULL, NULL);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (viewport), table);
  gtk_flow_box_set_vadjustment (GTK_FLOW_BOX (table),
                                gtk_scrollable_get_vadjustment (GTK_SCROLLABLE(viewport)));

  menu = taku_menu_get_default ();
  categories = taku_menu_get_categories (menu);

  taku_category_bar_set_table (bar, GTK_FLOW_BOX (table));
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

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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "libtaku/taku-table.h"
#include "libtaku/taku-icon-tile.h"
#include "libtaku/taku-launcher-tile.h"
#include "libtaku/xutil.h"

#if WITH_INOTIFY
#include "libtaku/inotify/inotify-path.h"
#include "libtaku/inotify/local_inotify.h"

static gboolean with_inotify;
G_LOCK_DEFINE(inotify_lock);
#endif

static GList *categories;
static GList *current_category;
static TakuLauncherCategory *fallback_category = NULL;

static TakuTable *table;

static GtkLabel *switcher_label;
static PangoAttrList *bold_attrs;

/* Make sure arrows request a square amount of space */
static void
arrow_size_request (GtkWidget      *widget,
                    GtkRequisition *requisition,
                    gpointer        user_data)
{
  requisition->width = requisition->height;
}

/* Changes the current category: Updates the switcher label and table filter */
static void
set_category (GList *category_list_item)
{
  TakuLauncherCategory *category = category_list_item->data;

  gtk_label_set_text (switcher_label, category->name);
  taku_table_set_filter (table, category);

  current_category = category_list_item;
}

static void
prev_category (void)
{
  if (current_category->prev == NULL)
    current_category = g_list_last (categories);
  else
    current_category = current_category->prev;

  set_category (current_category);
}

static void
next_category (void)
{
  if (current_category->next == NULL)
    current_category = categories;
  else
    current_category = current_category->next;

  set_category (current_category);
}

/* Handle failed focus events by switching between categories */
static gboolean
focus_cb (GtkWidget *widget, GtkDirectionType direction, gpointer user_data)
{
  /* If there are no categories, don't try switching */
  if (current_category == NULL)
    return FALSE;

  if (direction == GTK_DIR_LEFT) {
    prev_category ();

    gtk_widget_grab_focus (GTK_WIDGET (table));

    return TRUE;
  } else if (direction == GTK_DIR_RIGHT) {
    next_category ();

    gtk_widget_grab_focus (GTK_WIDGET (table));

    return TRUE;
  }

  return FALSE;
}

static void
position_menu (GtkMenu *menu,
               int *x, int *y, gboolean *push_in, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);

  gdk_window_get_origin (widget->window, x, y);

  *x += widget->allocation.x;
  *y += widget->allocation.y + widget->allocation.height;

  *push_in = TRUE;
}

static void
popdown_menu (GtkMenuShell *menu_shell, gpointer user_data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (user_data);

  /* Button up */
  gtk_toggle_button_set_active (button, FALSE);

  /* Destroy menu */
  gtk_widget_destroy (GTK_WIDGET (menu_shell));
}

static gboolean
popup_menu (GtkWidget *button, GdkEventButton *event, gpointer user_data)
{
  GtkWidget *menu;
  GList *l;

  /* Button down */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  /* Create menu */
  menu = gtk_menu_new ();
  gtk_widget_set_size_request (menu, GTK_WIDGET (button)->allocation.width, -1);

  g_signal_connect (menu, "selection-done", G_CALLBACK (popdown_menu), button);

  for (l = categories; l; l = l->next) {
    TakuLauncherCategory *category = l->data;
    GtkWidget *menu_item, *label;

    menu_item = gtk_menu_item_new ();
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (set_category), l);
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    label = gtk_label_new (category->name);
    gtk_label_set_attributes (GTK_LABEL (label), bold_attrs);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.0);
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (menu_item), label);
  }

  /* Popup menu */
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL,
                  position_menu, button,
                  event->button, event->time);

  return TRUE;
}

/*
 * Load all .directory files from @vfolderdir, and add them as tables.
 */
static void
load_vfolder_dir (const char *vfolderdir)
{
  GError *error = NULL;
  FILE *fp;
  char name[NAME_MAX], *filename;
  TakuLauncherCategory *category;

  categories = NULL;

  filename = g_build_filename (vfolderdir, "Root.order", NULL);
  fp = fopen (filename, "r");
  if (fp == NULL) {
    g_warning ("Cannot read %s", filename);
    g_free (filename);
    return;
  }
  g_free (filename);

  while (fgets (name, NAME_MAX - 9, fp) != NULL) {
    char **matches = NULL, *local_name = NULL;
    char **l;
    GKeyFile *key_file;

    if (name[0] == '#' || isspace (name[0]))
      continue;

    strcpy (name + strlen (name) - 1, ".directory");
  
    filename = g_build_filename (vfolderdir, name, NULL);

    key_file = g_key_file_new ();
    g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error);
    if (error) {
      g_warning ("Cannot read %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;

      goto done;
    }

    matches = g_key_file_get_string_list (key_file, "Desktop Entry", "Match", NULL, NULL);
    if (matches == NULL)
      goto done;

    local_name = g_key_file_get_locale_string (key_file, "Desktop Entry",
                                               "Name", NULL, NULL);
    if (local_name == NULL) {
      g_warning ("Directory file %s does not contain a \"Name\" field",
                 filename);
      g_strfreev (matches);
      goto done;
    }

    category = taku_launcher_category_new ();
    category->matches = matches;
    category->name  = local_name;

    categories = g_list_append (categories, category);

    /* Find a fallback category */
    if (fallback_category == NULL)
      for (l = matches; *l; l++)
        if (strcmp (*l, "meta-fallback") == 0)
          fallback_category = category;
    
  done:
    g_key_file_free (key_file);
    g_free (filename);
  }

  fclose (fp);
}

static void
match_category (TakuLauncherCategory *category, TakuLauncherTile *tile, gboolean *placed)
{
  const char **groups;
  char **match;
  
  for (match = category->matches; *match; match++) {
    /* Add all tiles to the all group */
    if (strcmp (*match, "meta-all") == 0) {
      taku_launcher_tile_add_group (tile, category);
      return;
    }
    
    for (groups = taku_launcher_tile_get_categories (tile);
         *groups; groups++) {
      if (strcmp (*match, *groups) == 0) {
        taku_launcher_tile_add_group (tile, category);
        *placed = TRUE;
        return;
      }
    }
  }
}

static void
set_groups (TakuLauncherTile *tile)
{
  gboolean placed = FALSE;
  GList *l;
  
  for (l = categories; l ; l = l->next) {
    match_category (l->data, tile, &placed);
  }

  if (!placed && fallback_category)
    taku_launcher_tile_add_group (tile, fallback_category);
}

/*
 * Load the desktop file @filename, and add it to the table.
 */
static void
load_desktop_file (const char *filename)
{
  GtkWidget *tile;
  
  g_assert (filename);

  /* TODO: load launcher data, probe that, and then create a tile */
  
  tile = taku_launcher_tile_for_desktop_file (filename);
  if (!tile) {
    return;
  }
  
  set_groups (TAKU_LAUNCHER_TILE (tile));
  
  gtk_container_add (GTK_CONTAINER (table), tile);
}

#if WITH_INOTIFY
/*
 * Monitor @directory with inotify, if available.
 */
static void
monitor (const char *directory)
{
  inotify_sub *sub;

  if (!with_inotify)
    return;
  
  sub = _ih_sub_new (directory, NULL, NULL);
  _ip_start_watching (sub);
}

/*
 * Used to delete tiles when they are removed from disk.  @a is the tile, @b is
 * the desktop filename to look for.
 */
static void
find_and_destroy (GtkWidget *widget, gpointer data)
{
  TakuLauncherTile *tile;
  const char *removed, *filename;

  tile = TAKU_LAUNCHER_TILE (widget);
  if (!tile)
    return;
  
  removed = data;
  
  filename = taku_launcher_tile_get_filename (tile);
  if (strcmp (removed, filename) == 0)
    gtk_widget_destroy (widget);
}

static void
inotify_event (ik_event_t *event, inotify_sub *sub)
{
  char *path;

  if (event->mask & IN_MOVED_TO || event->mask & IN_CREATE) {
    if (g_str_has_suffix (event->name, ".desktop")) {
      path = g_build_filename (sub->dirname, event->name, NULL);
      load_desktop_file (path);
      g_free (path);
    }
  } else if (event->mask & IN_MOVED_FROM || event->mask & IN_DELETE) {
    path = g_build_filename (sub->dirname, event->name, NULL);
    gtk_container_foreach (GTK_CONTAINER (table), find_and_destroy, path);
    g_free (path);
  }
}
#endif

/*
 * Load all .desktop files in @datadir/applications/.
 */
static void
load_data_dir (const char *datadir)
{
  GError *error = NULL;
  GDir *dir;
  char *directory;
  const char *name;

  g_assert (datadir);

  directory = g_build_filename (datadir, "applications", NULL);

  /* Check if the directory exists */
  if (! g_file_test (directory, G_FILE_TEST_IS_DIR)) {
    g_free (directory);
    return;
  }

#if WITH_INOTIFY
  monitor (directory);
#endif
  
  dir = g_dir_open (directory, 0, &error);
  if (error) {
    g_warning ("Cannot read %s: %s", directory, error->message);
    g_error_free (error);
    g_free (directory);
    return;
  }

  while ((name = g_dir_read_name (dir)) != NULL) {
    char *filename;
  
    if (! g_str_has_suffix (name, ".desktop"))
      continue;

    filename = g_build_filename (directory, name, NULL);

    load_desktop_file (filename);

    g_free (filename);
  }

  g_free (directory);
  g_dir_close (dir);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *box, *hbox, *prev_button, *next_button, *popup_button, 
    *arrow, *scrolled, *viewport, *fixed;
  GtkSizeGroup *size_group;
  PangoAttribute *attr;
  const char * const *dirs;
  char *vfolder_dir;
#ifndef STANDALONE
  /* Sane defaults in case something terrible happens in x_get_workarea() */
  int x = 0, y = 0;
#endif
  int w = 640, h = 480;

#if WITH_INOTIFY
  with_inotify = _ip_startup (inotify_event);
#endif
  
  gtk_init (&argc, &argv);
  g_set_application_name (_("Desktop"));
  
  /* Register the magic taku-icon size so that it can be controlled from the
     theme. */
  gtk_icon_size_register ("taku-icon", 64, 64);

  /* Create an attribute list for the category labels */
  bold_attrs = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index = -1;
  pango_attr_list_insert (bold_attrs, attr);
  attr = pango_attr_scale_new (PANGO_SCALE_LARGE);
  attr->start_index = 0;
  attr->end_index = -1;
  pango_attr_list_insert (bold_attrs, attr);

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

  /* This fixed is used to position the desktop itself in the work area */
  fixed = gtk_fixed_new ();
  gtk_widget_show (fixed);
  gtk_container_add (GTK_CONTAINER (window), fixed);

  /* Main VBox */
  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_widget_set_size_request (box, w, h);
  gtk_fixed_put (GTK_FIXED (fixed), box, x, y);

  /* Navigation bar */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  prev_button = gtk_button_new ();
  gtk_widget_set_name (prev_button, "MatchboxDesktopPrevButton");
  atk_object_set_name (gtk_widget_get_accessible (prev_button), "GroupPrevious");
  gtk_button_set_relief (GTK_BUTTON (prev_button), GTK_RELIEF_NONE);
  g_signal_connect (prev_button, "clicked", G_CALLBACK (prev_category), NULL);
  gtk_widget_show (prev_button);
  gtk_box_pack_start (GTK_BOX (hbox), prev_button, FALSE, TRUE, 0);

  arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_widget_set_size_request (arrow, 24, 24);
  g_signal_connect (arrow, "size-request",
                    G_CALLBACK (arrow_size_request), NULL);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (prev_button), arrow);
  gtk_size_group_add_widget (size_group, arrow);

  popup_button = gtk_toggle_button_new ();
  atk_object_set_name (gtk_widget_get_accessible (popup_button), "GroupButton");
  gtk_button_set_relief (GTK_BUTTON (popup_button), GTK_RELIEF_NONE);
  g_signal_connect (popup_button, "button-press-event",
                    G_CALLBACK (popup_menu), NULL);
  gtk_widget_show (popup_button);
  gtk_box_pack_start (GTK_BOX (hbox), popup_button, TRUE, TRUE, 0);

  switcher_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_attributes (switcher_label, bold_attrs);
  gtk_widget_show (GTK_WIDGET (switcher_label));
  gtk_container_add (GTK_CONTAINER (popup_button), GTK_WIDGET (switcher_label));
  gtk_size_group_add_widget (size_group, GTK_WIDGET (switcher_label));

  next_button = gtk_button_new ();
  gtk_widget_set_name (next_button, "MatchboxDesktopNextButton");
  atk_object_set_name (gtk_widget_get_accessible (next_button), "GroupNext");

  gtk_button_set_relief (GTK_BUTTON (next_button), GTK_RELIEF_NONE);
  g_signal_connect (next_button, "clicked", G_CALLBACK (next_category), NULL);
  gtk_widget_show (next_button);
  gtk_box_pack_end (GTK_BOX (hbox), next_button, FALSE, TRUE, 0);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
  gtk_widget_set_size_request (arrow, 24, 24);
  g_signal_connect (arrow, "size-request",
                    G_CALLBACK (arrow_size_request), NULL);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (next_button), arrow);
  gtk_size_group_add_widget (size_group, arrow);
  
  g_object_unref (size_group);

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
  gtk_container_add (GTK_CONTAINER (viewport), GTK_WIDGET (table));

  /* Load matchbox vfolders */
  vfolder_dir = g_build_filename (g_get_home_dir (),
                                  ".matchbox", "vfolders", NULL);
  if (g_file_test (vfolder_dir, G_FILE_TEST_EXISTS))
    load_vfolder_dir (vfolder_dir);
  else
    load_vfolder_dir (PKGDATADIR "/vfolders");
  g_free (vfolder_dir);

  /* Show the first category */
  if (categories) {
    set_category (categories);
  } else {
    gtk_label_set_text (switcher_label, _("No categories available"));
    gtk_widget_set_sensitive (prev_button, FALSE);
    gtk_widget_set_sensitive (popup_button, FALSE);
    gtk_widget_set_sensitive (next_button, FALSE);
  }

  /* Load all desktop files in the system data directories, and the user data
     directory. TODO: would it be best to do this in an idle handler and
     populate the desktop incrementally? */
  for (dirs = g_get_system_data_dirs (); *dirs; dirs++) {
    load_data_dir (*dirs);
  }
  load_data_dir (g_get_user_data_dir ());

  /* Go! */
  gtk_widget_show (GTK_WIDGET (table));

  gtk_main ();

  /* Cleanup */
  while (categories) {
    TakuLauncherCategory *category = categories->data;
    
    taku_launcher_category_free (category);

    categories = g_list_delete_link (categories, categories);
  }

  return 0;
}

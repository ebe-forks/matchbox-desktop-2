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
#include "taku-table.h"
#include "taku-icon-tile.h"
#include "taku-launcher-tile.h"
#include "xutil.h"

typedef struct {
  char *match;
  char *name;
} Category;

static GList *categories;
static GList *current_category;

static TakuTable *table;

static GtkLabel *switcher_label;

static gboolean
popup_menu (GtkButton *button, gpointer user_data);

/* Changes the current category: Updates the switcher label and table filter */
static void
set_category (GList *category_list_item)
{
  Category *category = category_list_item->data;

  gtk_label_set_text (switcher_label, category->name);
  taku_table_set_filter (table, category->match);

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
  g_signal_handlers_block_by_func (button, popup_menu, NULL);
  gtk_toggle_button_set_active (button, FALSE);
  g_signal_handlers_unblock_by_func (button, popup_menu, NULL);

  /* Destroy menu */
  gtk_widget_destroy (GTK_WIDGET (menu_shell));
}

static gboolean
popup_menu (GtkButton *button, gpointer user_data)
{
  GtkWidget *menu;
  GList *l;

  /* Button down */
  g_signal_handlers_block_by_func (button, popup_menu, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_handlers_unblock_by_func (button, popup_menu, NULL);

  /* Create menu */
  menu = gtk_menu_new ();
  gtk_widget_set_size_request (menu, GTK_WIDGET (button)->allocation.width, -1);

  g_signal_connect (menu, "selection-done", G_CALLBACK (popdown_menu), button);

  for (l = categories; l; l = l->next) {
    Category *category = l->data;
    GtkWidget *menu_item, *label;

    menu_item = gtk_menu_item_new ();
    g_signal_connect_swapped (menu_item, "activate",
                              G_CALLBACK (set_category), l);
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    label = gtk_label_new (category->name);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.0);
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (menu_item), label);
  }

  /* Popup menu */
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL,
                  position_menu,
                  button,
                  1,
                  GDK_CURRENT_TIME);

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
  Category *category;

  category = g_slice_new (Category);
  category->match = NULL;
  category->name  = g_strdup (_("All"));

  categories = NULL;
  categories = g_list_prepend (categories, category);

  filename = g_build_filename (vfolderdir, "Root.order", NULL);
  fp = fopen (filename, "r");
  if (fp == NULL) {
    g_warning ("Cannot read %s", filename);
    g_free (filename);
    return;
  }
  g_free (filename);

  while (fgets (name, NAME_MAX - 9, fp) != NULL) {
    char *match = NULL, *local_name = NULL;
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

    match = g_key_file_get_string (key_file, "Desktop Entry", "Match", NULL);
    if (match == NULL)
      goto done;

    local_name = g_key_file_get_locale_string (key_file, "Desktop Entry",
                                               "Name", NULL, NULL);
    if (local_name == NULL) {
      g_warning ("Directory file %s does not contain a \"Name\" field",
                 filename);
      g_free (match);
      goto done;
    }

    category = g_slice_new (Category);
    category->match = match;
    category->name  = local_name;

    categories = g_list_append (categories, category);

  done:
    g_key_file_free (key_file);
    g_free (filename);
  }

  fclose (fp);
}

/*
 * Load all .desktop files in @datadir/applications/, and add them to @table.
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

  dir = g_dir_open (directory, 0, &error);
  if (error) {
    g_warning ("Cannot read %s: %s", directory, error->message);
    g_error_free (error);
    g_free (directory);
    return;
  }

  while ((name = g_dir_read_name (dir)) != NULL) {
    char *filename;
    GtkWidget *tile;
  
    if (! g_str_has_suffix (name, ".desktop"))
      continue;

    filename = g_build_filename (directory, name, NULL);

    /* TODO: load launcher data, probe that, and then create a tile */

    tile = taku_launcher_tile_for_desktop_file (filename);
    if (!tile)
      goto done;

    gtk_container_add (GTK_CONTAINER (table), tile);

  done:
    g_free (filename);
  }

  g_free (directory);
  g_dir_close (dir);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *box, *hbox, *button, *arrow, *scrolled, *viewport;
  const char * const *dirs;
  char *vfolder_dir;
#ifndef STANDALONE
  /* Sane defaults in case something terrible happens in x_get_workarea() */
  int x = 0, y = 0;
#endif
  int w = 640, h = 480;

  gtk_init (&argc, &argv);
  g_set_application_name (_("Desktop"));
  
  /* Register the magic TakuIcon size so that it can be controlled from the
     theme. */
  gtk_icon_size_register ("TakuIcon", 64, 64);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_set_name (window, "TakuWindow");
  gtk_window_set_title (GTK_WINDOW (window), _("Desktop"));

#ifndef STANDALONE
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DESKTOP);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
  if (x_get_workarea (&x, &y, &w, &h)) {
    gtk_window_set_default_size (GTK_WINDOW (window), w, h);
    gtk_window_move (GTK_WINDOW (window), x, y);
  }
#else
  gtk_window_set_default_size (GTK_WINDOW (window), w, h);
#endif

  gtk_widget_show (window);

  /* Main VBox */
  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (window), box);

  /* Navigation bar */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);

  button = gtk_button_new ();
  gtk_widget_set_name (button, "MatchboxDesktopPrevButton");
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "clicked", G_CALLBACK (prev_category), NULL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (button), arrow);

  button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "clicked", G_CALLBACK (popup_menu), NULL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  switcher_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_widget_show (GTK_WIDGET (switcher_label));
  gtk_container_add (GTK_CONTAINER (button), GTK_WIDGET (switcher_label));

  button = gtk_button_new ();
  gtk_widget_set_name (button, "MatchboxDesktopNextButton");
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "clicked", G_CALLBACK (next_category), NULL);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (button), arrow);

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

  /* Show the 'All' category */
  set_category (categories);

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
    Category *category = categories->data;

    g_free (category->match);
    g_free (category->name);

    g_slice_free (Category, category);

    categories = g_list_delete_link (categories, categories);
  }

  return 0;
}

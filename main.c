#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "taku-table.h"
#include "taku-icon-tile.h"
#include "taku-launcher-tile.h"
#include "xutil.h"

/* A hash of group ID (not display name) to a TakuTable. */
static GHashTable *groups;

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
    const char *categories;
    GtkWidget *table, *tile;
  
    if (! g_str_has_suffix (name, ".desktop"))
      continue;

    filename = g_build_filename (directory, name, NULL);

    /* TODO: load launcher data, probe that, and then create a tile */

    tile = taku_launcher_tile_for_desktop_file (filename);
    if (!tile)
      goto done;

    categories = taku_launcher_tile_get_categories (TAKU_LAUNCHER_TILE (tile));
    if (!categories) {
      categories = "";
    }

    /* TODO: replace with dynamic menu system */ 
    if (strstr (categories, ";Office;")) {
      table = g_hash_table_lookup (groups, "office");
    } else if (strstr (categories, ";Settings;")) {
      table = g_hash_table_lookup (groups, "settings");
    } else {
      table = g_hash_table_lookup (groups, "other");
    }

    if (!table) {
      g_warning ("Cannot find table");
      goto done;
    }

    gtk_widget_show (tile);
    gtk_container_add (GTK_CONTAINER (table), tile);

  done:
    g_free (filename);
  }

  g_free (directory);
  g_dir_close (dir);
}

static void
make_table (const char *id, GtkNotebook *notebook)
{
  GtkWidget *scrolled, *table;

  g_assert (id);
  g_assert (notebook);

  table = taku_table_new ();
  gtk_widget_show (table);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_show (scrolled);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), table);
  
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled, gtk_label_new (id));
  
  g_hash_table_insert (groups, g_strdup (id), table);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *box, *notebook;
  const char * const *dirs;
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

  groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  
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

  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (window), box);

  notebook = gtk_notebook_new ();
  //gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_widget_show (notebook);
  gtk_box_pack_start (GTK_BOX (box), notebook, TRUE, TRUE, 0);

  make_table ("other", GTK_NOTEBOOK (notebook));
  make_table ("office", GTK_NOTEBOOK (notebook));
  make_table ("settings", GTK_NOTEBOOK (notebook));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
  
  /* Load all desktop files in the system data directories, and the user data
     directory. TODO: would it be best to do this in an idle handler and
     populate the desktop incrementally? */
  for (dirs = g_get_system_data_dirs (); *dirs; dirs++) {
    load_data_dir (*dirs);
  }
  load_data_dir (g_get_user_data_dir ());

  gtk_main();

  return 0;
}

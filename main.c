#include <gtk/gtk.h>
#include "taku-table.h"
#include "taku-icon-tile.h"
#include "taku-launcher-tile.h"

/*
 * Load all .desktop files in @datadir/applications/, and add them to @table.
 */
static void
load_data_dir (GtkWidget *table, const char *datadir)
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

    tile = taku_launcher_tile_for_desktop_file (filename);
    if (tile) {
      gtk_widget_show (tile);
      gtk_container_add (GTK_CONTAINER (table), tile);
    }

    g_free (filename);
  }

  g_free (directory);
  g_dir_close (dir);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *scrolled, *table;
  const char * const *dirs;

  gtk_init (&argc, &argv);
  
  /* Register the magic TakuIcon size so that it can be controlled from the
     theme. */
  gtk_icon_size_register ("TakuIcon", 64, 64);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_set_name(window, "TakuWindow");
#if AS_DESKTOP
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DESKTOP);
  gtk_window_fullscreen (GTK_WINDOW (window));
#else
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
#endif
  gtk_widget_show (window);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_widget_show (scrolled);
  gtk_container_add (GTK_CONTAINER (window), scrolled);
  
  table = taku_table_new ();
  gtk_widget_show (table);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
                                         table);

  /* Load all desktop files in the system data directories, and the user data
     directory. TODO: would it be best to do this in an idle handler and
     populate the desktop incrementally? */
  for (dirs = g_get_system_data_dirs (); *dirs; dirs++) {
    load_data_dir (table, *dirs);
  }
  load_data_dir (table, g_get_user_data_dir ());

  gtk_main();

  return 0;
}

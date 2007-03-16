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
static GtkNotebook *notebook;
static GtkLabel *switcher_label;

static void
switch_page_cb (GtkNotebook *notebook,
                GtkNotebookPage *nb_page, int page_num, gpointer user_data)
{
  GtkWidget *page;
  const char *text;

  page = gtk_notebook_get_nth_page (notebook, page_num);

  text = gtk_notebook_get_tab_label_text (notebook, page);
  gtk_label_set_text (switcher_label, text);
}

static void
prev_page (GtkButton *button, gpointer user_data)
{
  int page, last_page;

  last_page = gtk_notebook_get_n_pages (notebook) - 1;

  page = gtk_notebook_get_current_page (notebook);
  if (page == 0)
    page = last_page;
  else
    page--;

  gtk_notebook_set_current_page (notebook, page);
}

static void
next_page (GtkButton *button, gpointer user_data)
{
  int page, last_page;

  last_page = gtk_notebook_get_n_pages (notebook) - 1;

  page = gtk_notebook_get_current_page (notebook);
  if (page == last_page)
    page = 0;
  else
    page++;

  gtk_notebook_set_current_page (notebook, page);
}

static void
switch_to_page (GtkMenuItem *menu_item,
                gpointer     user_data)
{
  GtkWidget *widget = GTK_WIDGET (menu_item);
  GList *children, *l;
  int i = 0;

  /* Find menu item index */
  children = gtk_container_get_children (GTK_CONTAINER (widget->parent));
  for (l = children; l; l = l->next) {
    if (l->data == menu_item)
      break;
    
    i++;
  }

  /* Switch to page */
  gtk_notebook_set_current_page (notebook, i);
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
popup_menu (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  GtkWidget *menu;
  GList *children, *l;

  /* Button down */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

  /* Create menu */
  menu = gtk_menu_new ();
  gtk_widget_set_size_request (menu, widget->allocation.width, -1);

  g_signal_connect (menu, "selection-done", G_CALLBACK (popdown_menu), widget);

  children = gtk_container_get_children (GTK_CONTAINER (notebook));
  for (l = children; l; l = l->next) {
    const char *text;
    GtkWidget *menu_item, *label;

    text = gtk_notebook_get_tab_label_text (notebook, l->data);

    menu_item = gtk_menu_item_new ();
    g_signal_connect (menu_item, "activate", G_CALLBACK (switch_to_page), NULL);
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    label = gtk_label_new (text);
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.0);
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (menu_item), label);
  }

  /* Popup menu */
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL,
                  position_menu,
                  widget,
                  event->button,
                  gdk_event_get_time ((GdkEvent *) event));

  return TRUE;
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
    } else if (strstr (categories, "Game;")) {
      table = g_hash_table_lookup (groups, "games");
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
make_table (const char *id, const char *label)
{
  GtkWidget *scrolled, *viewport, *table;

  g_assert (id);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
                                GTK_SHADOW_NONE);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (scrolled), viewport);

  table = taku_table_new ();
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (viewport), table);
  
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled, gtk_label_new (label));
  
  g_hash_table_insert (groups, g_strdup (id), table);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *box, *hbox, *button, *arrow;
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

  /* Main VBox */
  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (window), box);

  /* Navigation bar */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);

  button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "clicked", G_CALLBACK (prev_page), NULL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (button), arrow);

  button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "button-press-event", G_CALLBACK (popup_menu), NULL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  switcher_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_widget_show (GTK_WIDGET (switcher_label));
  gtk_container_add (GTK_CONTAINER (button), GTK_WIDGET (switcher_label));

  button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "clicked", G_CALLBACK (next_page), NULL);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (button), arrow);

  /* Notebook */
  notebook = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_notebook_set_show_tabs (notebook, FALSE);
  g_signal_connect (notebook, "switch-page", G_CALLBACK (switch_page_cb), NULL);
  gtk_widget_show (GTK_WIDGET (notebook));
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (notebook), TRUE, TRUE, 0);

  make_table ("office", _("Office"));
  make_table ("games", _("Games"));
  make_table ("other", _("Other"));
  make_table ("settings", _("Settings"));
  gtk_notebook_set_current_page (notebook, 0);
  
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

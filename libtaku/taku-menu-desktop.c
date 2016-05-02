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
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <ctype.h>

#include "taku-menu.h"
#include "taku-launcher-tile.h"
#include "launcher-util.h"

#if WITH_INOTIFY
#include <sys/inotify.h>
#include "inotify/inotify-path.h"

static gboolean with_inotify;
G_LOCK_DEFINE (inotify_lock);
#endif

G_DEFINE_TYPE (TakuMenu, taku_menu, G_TYPE_OBJECT)

#define TAKU_MENU_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
  TAKU_TYPE_MENU, TakuMenuPrivate))

struct _TakuMenuPrivate
{
  GList *categories;
  GList *items;

  GHashTable *path_items_hash;

  TakuLauncherCategory *fallback_category;
};

struct _TakuMenuItem
{
  gchar *path;
  gchar *name;
  gchar *description;
  gchar *icon_name;
  gchar **cats;
  GList *categories;

  gchar **argv;
  gboolean use_sn;
  gboolean single_instance;
};

enum
{
  ITEM_ADDED,
  ITEM_REMOVED,

  LAST_SIGNAL
};
static guint _menu_signals[LAST_SIGNAL] = {0};


/*
 * Public functions
 */

/*
 * Returns a list of TakuLauncherCategorys
 */
GList*
taku_menu_get_categories (TakuMenu *menu)
{
  g_return_val_if_fail (TAKU_IS_MENU (menu), NULL);

  return menu->priv->categories;
}

/*
 * Returns a list of TakuMenuItems
 */
GList*
taku_menu_get_items (TakuMenu *menu)
{
  g_return_val_if_fail (TAKU_IS_MENU (menu), NULL);

  return menu->priv->items;
}

/*
 * < MenuItem functions />
 */

const gchar*
taku_menu_item_get_name (TakuMenuItem *item)
{
  g_return_val_if_fail (item, NULL);

  return item->name;
}

const gchar*
taku_menu_item_get_description (TakuMenuItem *item)
{
  g_return_val_if_fail (item, NULL);

  return item->description;
}

GdkPixbuf*
taku_menu_item_get_icon (TakuMenuItem *item, int size)
{
  g_return_val_if_fail (item, NULL);

  return get_icon (item->icon_name, size);
}

GList*
taku_menu_item_get_categories (TakuMenuItem *item)
{
  g_return_val_if_fail (item, NULL);

  return item->categories;
}


gboolean
taku_menu_item_launch (TakuMenuItem *item, GtkWidget *widget)
{
  g_return_val_if_fail (item, FALSE);

  launcher_start (widget,
                  item,
                  item->argv,
                  item->use_sn,
                  item->single_instance);

  return TRUE;
}

const gchar *
taku_menu_desktop_get_executable (TakuMenuItem *item)
{
  g_return_val_if_fail (item, NULL);

  return item->argv[0];
}

/*
 * private functions
 */

/*
 * Load all .directory files from @vfolderdir, and add them as tables.
 */
static void
load_vfolder_dir (TakuMenu *menu, const char *vfolderdir)
{
  TakuMenuPrivate *priv;
  GError *error = NULL;
  FILE *fp;
  char name[NAME_MAX], *filename;
  TakuLauncherCategory *category;

  g_return_if_fail (TAKU_IS_MENU (menu));
  priv = menu->priv;

  priv->categories = NULL;

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

    matches = g_key_file_get_string_list (key_file, "Desktop Entry",
                                          "Match", NULL, NULL);
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

    priv->categories = g_list_append (priv->categories, category);

    /* Find a fallback category */
    if (priv->fallback_category == NULL)
      for (l = matches; *l; l++)
        if (strcmp (*l, "meta-fallback") == 0)
          priv->fallback_category = category;

  done:
    g_key_file_free (key_file);
    g_free (filename);
  }

  fclose (fp);
}

static void
match_category (TakuLauncherCategory  *category,
                TakuMenuItem          *item,
                gboolean              *placed)
{
  char **groups;
  char **match;

  for (match = category->matches; *match; match++) {
    /* Add all tiles to the all group */
    if (strcmp (*match, "meta-all") == 0) {
      item->categories = g_list_append (item->categories, category);
      return;
    }

    for (groups = item->cats; *groups; groups++) {
      if (strcmp (*match, *groups) == 0) {
        item->categories = g_list_append (item->categories, category);
        *placed = TRUE;
        return;
      }
    }
  }
}

static void
set_groups (TakuMenu *menu, TakuMenuItem *item)
{
  gboolean placed = FALSE;
  GList *l;

  for (l = menu->priv->categories; l ; l = l->next) {
    match_category (l->data, item, &placed);
  }

  if (!placed && menu->priv->fallback_category)
    item->categories = g_list_append (item->categories,
                                      menu->priv->fallback_category);
}

/*
 * Get the boolean for the key @key from @key_file, and if it cannot be parsed
 * or does not exist return @def.
 */
static gboolean
get_desktop_boolean (GKeyFile *key_file, const char *key, gboolean def)
{
  GError *error = NULL;
  gboolean b;

  g_assert (key_file);
  g_assert (key);

  b = g_key_file_get_boolean (key_file, DESKTOP, key, &error);
  if (error) {
    g_error_free (error);
    b = def;
  }

  return b;
}

static char *
get_desktop_string (GKeyFile *key_file, const char *key)
{
  char *s;

  g_assert (key_file);
  g_assert (key);

  /* Get the key */
  s = g_key_file_get_locale_string (key_file, DESKTOP, key, NULL, NULL);
  /* Strip any whitespace */
  s = s ? g_strstrip (s) : NULL;
  if (s && s[0] != '\0') {
    return s;
  } else {
    if (s) g_free (s);
    return NULL;
  }
}

/*
 * Load the desktop file @filename, and add it to the table.
 */
static TakuMenuItem*
load_desktop_file (TakuMenu *menu, const char *filename)
{
  TakuMenuPrivate *priv;
  TakuMenuItem *item = NULL;
  GKeyFile *key_file;
  GError *err = NULL;
  gchar *exec, *cats;

  g_assert (filename);
  g_return_val_if_fail (TAKU_IS_MENU (menu), NULL);
  priv = menu->priv;

  /* Check for duplicate desktop files based on path name */
  if (g_hash_table_lookup (priv->path_items_hash, filename))
    return NULL;

  key_file = g_key_file_new ();

  /* Do the checks to make sure the .desktop file is valid */
  if (!g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &err)) {
    g_key_file_free (key_file);
    g_error_free (err);
    return NULL;
  }

  if (get_desktop_boolean (key_file, "NoDisplay", FALSE)) {
    g_key_file_free (key_file);
    return NULL;
  }

  /* This is important, so read it first to simplyfy cleanup */
  exec = get_desktop_string (key_file, "Exec");
  if (exec == NULL) {
    g_free (exec);
    g_key_file_free (key_file);
    return NULL;
  }

  /* Okay, were good to go */
  item = g_slice_new0 (TakuMenuItem);

  item->path = g_strdup (filename);
  item->name = get_desktop_string (key_file, "Name");
  item->description = get_desktop_string (key_file, "Comment");
  item->icon_name = get_desktop_string (key_file, "Icon");
  item->use_sn = get_desktop_boolean (key_file, "StartupNotify", FALSE);
  item->single_instance = get_desktop_boolean (key_file,
                                               "X-MB-SingleInstance", FALSE)
                          || get_desktop_boolean (key_file,
                                                  "SingleInstance", FALSE);
  item->argv = exec_to_argv (exec);
  g_free (exec);

  cats = get_desktop_string (key_file, "Categories");
  if (cats == NULL)
    cats = g_strdup ("");
  item->cats = g_strsplit (cats, ";", -1);
  g_free (cats);


  set_groups (menu, item);
  priv->items = g_list_append (priv->items, item);
  g_hash_table_insert (priv->path_items_hash, item->path, item);

  return item;
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
/*
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
*/

static TakuMenuItem *
_find_item (TakuMenu *menu, const gchar *path)
{
  TakuMenuPrivate *priv;
  GList *l;

  g_return_val_if_fail (TAKU_IS_MENU (menu), NULL);
  priv = menu->priv;

  for (l = priv->items; l; l = l->next)
  {
    TakuMenuItem *item = l->data;

    if (strcmp (item->path, path) == 0)
      return item;
  }
  return NULL;
}

static void
_remove_item (TakuMenu *menu, TakuMenuItem *item)
{
  TakuMenuPrivate *priv;

  g_return_if_fail (TAKU_IS_MENU (menu));
  priv = menu->priv;

  priv->items = g_list_remove (priv->items, item);

  g_hash_table_remove (priv->path_items_hash, item->path);

  /* TODO: Lots of leaks here */
  g_slice_free (TakuMenuItem, item);
}

static void
inotify_event (ik_event_t *event, inotify_sub *sub)
{
  char *path;
  TakuMenu *menu = taku_menu_get_default ();
  TakuMenuItem *item = NULL;

  if (event->mask & IN_MOVED_TO || event->mask & IN_CREATE) {
    if (g_str_has_suffix (event->name, ".desktop")) {
      path = g_build_filename (sub->dirname, event->name, NULL);
      item = load_desktop_file (taku_menu_get_default (), path);

      if (item)
        g_signal_emit (menu, _menu_signals[ITEM_ADDED], 0, item);

      g_free (path);
    }
  } else if (event->mask & IN_MOVED_FROM || event->mask & IN_DELETE) {
    path = g_build_filename (sub->dirname, event->name, NULL);
    item = _find_item (menu, path);

    if (item) {
      g_signal_emit (menu, _menu_signals[ITEM_REMOVED], 0, item);
      _remove_item (menu, item);
    }

    g_free (path);
  }
}
#endif

/*
 * Recursively load all desktop files in @directory
 */
static void
load_desktop_files (TakuMenu *menu, const char *directory)
{
  GError *error = NULL;
  GDir *dir;
  const char *name;

  g_assert (menu);
  g_assert (directory);

  /* Check if the directory exists */
  if (! g_file_test (directory, G_FILE_TEST_IS_DIR)) {
    return;
  }

#if WITH_INOTIFY
  monitor (directory);
#endif

  dir = g_dir_open (directory, 0, &error);
  if (error) {
    g_warning ("Cannot read %s: %s", directory, error->message);
    g_error_free (error);
    return;
  }

  while ((name = g_dir_read_name (dir)) != NULL) {
    char *filename;

    filename = g_build_filename (directory, name, NULL);

    if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
      load_desktop_files (menu, filename);
    } else if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) &&
               g_str_has_suffix (name, ".desktop")) {
      load_desktop_file (menu, filename);
    }

    g_free (filename);
  }

  g_dir_close (dir);
}

/*
 * Load all .desktop files in @datadir/applications/.
 */
static void
load_data_dir (TakuMenu *menu, const char *datadir)
{
  char *directory;

  g_return_if_fail (TAKU_IS_MENU (menu));
  g_return_if_fail (datadir);

  directory = g_build_filename (datadir, "applications", NULL);
  load_desktop_files (menu, directory);
  g_free (directory);
}


/* GObject stuff */

static void
taku_menu_finalize (GObject *menu)
{
  /* TODO */
  G_OBJECT_CLASS (taku_menu_parent_class)->finalize (menu);
}

static void
taku_menu_class_init (TakuMenuClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = taku_menu_finalize;

  /* Class signals */
  _menu_signals[ITEM_ADDED] =
    g_signal_new ("item-added",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TakuMenuClass, item_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  _menu_signals[ITEM_REMOVED] =
    g_signal_new ("item-removed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TakuMenuClass, item_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_type_class_add_private (obj_class, sizeof(TakuMenuPrivate));

}

static void
taku_menu_init (TakuMenu *menu)
{
  TakuMenuPrivate *priv;
  gchar *vfolder_dir = NULL;
  const gchar * const *dirs;

  priv = menu->priv = TAKU_MENU_GET_PRIVATE (menu);

  priv->path_items_hash = g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 NULL,
                                                 NULL);

#if WITH_INOTIFY
  with_inotify = _ip_startup (inotify_event);
#endif

  /* Create the categories from matchbox vfolders*/
  vfolder_dir = g_build_filename (g_get_home_dir (),
                                  ".matchbox", "vfolders", NULL);
  if (g_file_test (vfolder_dir, G_FILE_TEST_EXISTS))
    load_vfolder_dir (menu, vfolder_dir);
  else
    load_vfolder_dir (menu, PKGDATADIR "/vfolders");
  g_free (vfolder_dir);

  /*
   * Load all desktop files in the system data directories, and the user data
   * directory. TODO: would it be best to do this in an idle handler and
   * populate the desktop incrementally?
   */
  for (dirs = g_get_system_data_dirs (); *dirs; dirs++) {
    load_data_dir (menu, *dirs);
  }
  load_data_dir (menu, g_get_user_data_dir ());
}

/*
 * Expected to create a list of TakuLauncherCategorys and TakuMenuItems
 */
TakuMenu*
taku_menu_get_default (void)
{
  static TakuMenu *menu = NULL;

  if (menu == NULL)
    menu = g_object_new (TAKU_TYPE_MENU, NULL);

  return menu;
}

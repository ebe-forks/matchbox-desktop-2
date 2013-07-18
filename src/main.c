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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "desktop.h"

#if WITH_DBUS
#include <dbus/dbus.h>

static gboolean
emit_loaded_signal (gpointer user_data)
{
  DBusError error = DBUS_ERROR_INIT;
  DBusConnection *conn;
  DBusMessage *msg;

  conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!conn) {
    g_printerr ("Cannot connect to system bus: %s", error.message);
    dbus_error_free (&error);
    return FALSE;
  }

  msg = dbus_message_new_signal ("/", "org.matchbox_project.desktop", "Loaded");

  dbus_connection_send (conn, msg, NULL);
  dbus_message_unref (msg);

  /* Flush explicitly because we're too lazy to integrate DBus into the main
     loop. We're only sending a signal, so if we got as far as here it's
     unlikely to block. */
  dbus_connection_flush (conn);
  dbus_connection_unref (conn);

  return FALSE;
}
#endif

static void
load_style (GtkWidget *widget)
{
  GtkCssProvider *provider;
  GError *error = NULL;

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_path (GTK_CSS_PROVIDER (provider),
                                   PKGDATADIR "/style.css", &error);
  if (error && !g_error_matches (error, GTK_CSS_PROVIDER_ERROR, GTK_CSS_PROVIDER_ERROR_IMPORT)) {
    g_warning ("Cannot load CSS: %s", error->message);
    g_error_free (error);
  } else {
    gtk_style_context_add_provider_for_screen
      (gtk_widget_get_screen (widget),
       GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  g_object_unref (provider);
}

int
main (int argc, char **argv)
{
  GtkWidget *desktop;
  char *mode_string = NULL;
  GError *error = NULL;
  GOptionContext *option_context;
  GOptionGroup *option_group;
  GOptionEntry option_entries[] = {
    { "mode", 'm', 0, G_OPTION_ARG_STRING, &mode_string,
      N_("Desktop mode"), N_("DESKTOP|TITLEBAR|WINDOW") },
    { NULL }
  };
  DesktopMode mode = MODE_DESKTOP;

  g_set_application_name (_("Desktop"));

  option_context = g_option_context_new (NULL);
  option_group = g_option_group_new ("matchbox-desktop",
                                     N_("Matchbox Desktop"),
                                     N_("Matchbox Desktop options"),
                                     NULL, NULL);
  g_option_group_add_entries (option_group, option_entries);
  g_option_context_set_main_group (option_context, option_group);
  g_option_context_add_group (option_context, gtk_get_option_group (TRUE));

  error = NULL;
  if (!g_option_context_parse (option_context, &argc, &argv, &error)) {
    g_option_context_free (option_context);

    g_warning ("%s", error->message);
    g_error_free (error);

    return 1;
  }

  g_option_context_free (option_context);

  if (mode_string) {
    if (g_ascii_strcasecmp (mode_string, "desktop") == 0) {
      mode = MODE_DESKTOP;
    } else if (g_ascii_strcasecmp (mode_string, "titlebar") == 0) {
      mode = MODE_TITLEBAR;
    } else if (g_ascii_strcasecmp (mode_string, "window") == 0) {
      mode = MODE_WINDOW;
    } else {
      g_printerr ("Unparsable mode '%s', expecting desktop/titlebar/window\n", mode_string);
      return 1;
    }
    g_free (mode_string);
  }

#if WITH_DBUS
  g_idle_add (emit_loaded_signal, NULL);
#endif

  desktop = create_desktop (mode);
  load_style (desktop);
  gtk_main ();
  destroy_desktop ();

  return 0;
}

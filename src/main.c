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

  gtk_init (&argc, &argv);
  g_set_application_name (_("Desktop"));

#if WITH_DBUS
  g_idle_add (emit_loaded_signal, NULL);
#endif

  desktop = create_desktop ();
  load_style (desktop);
  gtk_main ();
  destroy_desktop ();

  return 0;
}

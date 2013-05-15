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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <stdlib.h>
#include "xutil.h"

char *
x_strerror (int code)
{
#define BUFFER_SIZE 255
  char *s;

  s = g_malloc (BUFFER_SIZE);

  XGetErrorText (gdk_x11_get_default_xdisplay (), code, s, BUFFER_SIZE);

  return s;
}

static void
net_workarea_changed (WorkAreaFunc cb, GdkWindow *window)
{
  Atom real_type;
  int result, xres, real_format;
  unsigned long items_read, items_left;
  long *coords;
  Atom workarea_atom = gdk_x11_get_xatom_by_name ("_NET_WORKAREA");

  gdk_error_trap_push ();
  result = XGetWindowProperty (gdk_x11_get_default_xdisplay (), GDK_ROOT_WINDOW (),
                               workarea_atom, 0L, 4L, False,
                               XA_CARDINAL, &real_type, &real_format,
                               &items_read, &items_left,
                               (unsigned char **) (void*)&coords);
  if ((xres = gdk_error_trap_pop ()) != 0) {
    char *s = x_strerror (xres);
    g_warning ("Cannot get property: %s", s);
    g_free (s);
    return;
  }

  if (result == Success && items_read) {
    /* Note that this only considers the first workspace, and if used with
       multi-desktop systems this may produce incorrect results. */
    cb (coords[0], coords[1], coords[2], coords[3]);
    XFree(coords);
  }
}

static GdkFilterReturn
workarea_property_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
  XEvent *xevent = gdk_xevent;
  WorkAreaFunc cb = data;
  
  switch (xevent->type) {
  case PropertyNotify:
    if (xevent->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_WORKAREA"))
      net_workarea_changed (cb, event->any.window);
    break;
  default:
    break;
  }
  
  return GDK_FILTER_CONTINUE;
}

void
x_monitor_workarea (GdkScreen *screen, WorkAreaFunc cb)
{
  GdkWindow *root;
  
  root = gdk_screen_get_root_window (screen);
  
  gdk_window_set_events (root,
                         gdk_window_get_events (root) |
                         GDK_PROPERTY_CHANGE_MASK);
  gdk_window_add_filter (root, workarea_property_filter, cb);
  
  net_workarea_changed (cb, root);
}

void
x_window_activate (Window win, guint32 timestamp)
{
  /* Note that this doesn't work if the WM doesn't support _NET_ACTIVE_WINDOW.
     However, that is pretty much given really. */
  
  Atom atom_net_active = gdk_x11_get_xatom_by_name ("_NET_ACTIVE_WINDOW");
  XClientMessageEvent ev;

  memset (&ev, 0, sizeof ev);
  ev.type = ClientMessage;
  ev.window = win;
  ev.message_type = atom_net_active;
  ev.format = 32;

  ev.data.l[0] = 2; /* 0: unknown, 1: application; 2: pager */
  ev.data.l[1] = timestamp;
  ev.data.l[2] = None; /* Currently active window */
  ev.data.l[3] = 0;
  ev.data.l[4] = 0;

  XSendEvent (gdk_x11_get_default_xdisplay (), GDK_ROOT_WINDOW (), 
	      False, SubstructureRedirectMask, (XEvent*)&ev);
}

#define RETURN_NONE_IF_NULL(p) if ((p) == '\0') return None; 

Window
mb_single_instance_get_window (const char *bin_name)
{
  Atom atom_exec_map = gdk_x11_get_xatom_by_name ("_MB_CLIENT_EXEC_MAP");

  Atom type;
  int format;
  unsigned char *data = NULL;
  unsigned long n_items, bytes_after;
  int result;

  unsigned char *p, *key = NULL, *value = NULL;
  Window win_found;

  result = XGetWindowProperty (gdk_x11_get_default_xdisplay (), GDK_ROOT_WINDOW (), 
			       atom_exec_map,
			       0, 10000L,
			       False, XA_STRING,
			       &type, &format, &n_items,
			       &bytes_after, (unsigned char **) &data);

  if (result != Success || data == NULL || n_items == 0) {
    if (data)
      XFree (data);
    return None;
  }

  p = data;

  while (*p != '\0') { 
    key = p;
    while (*p != '=' && *p != '\0') p++;

    RETURN_NONE_IF_NULL(*p);

    *p = '\0'; p++;

    RETURN_NONE_IF_NULL(*p);

    value = p;

    while (*p != '|' && *p != '\0') p++;

    RETURN_NONE_IF_NULL(*p);

    *p = '\0';      

    if (!strcmp ((char*) key, (char*) bin_name)) {
      win_found = atoi ((char*) value); /* XXX should check window ID 
				           actually exists */
      XFree (data);
      return ((win_found > 0) ? win_found : None);
    }

    p++;
  }
  XFree (data);

  return None;
}

gboolean
mb_single_instance_is_starting (const char *bin_name)
{
  Atom atom_exec_map = gdk_x11_get_xatom_by_name ("_MB_CLIENT_STARTUP_LIST");

  Atom type;
  int format;
  unsigned char *data = NULL;
  unsigned long n_items, bytes_after;
  int result;

  result = XGetWindowProperty (gdk_x11_get_default_xdisplay (), GDK_ROOT_WINDOW (), 
			       atom_exec_map,
			       0, 10000L,
			       False, XA_STRING,
			       &type, &format, &n_items,
			       &bytes_after, (unsigned char **) &data);

  if (result != Success || data == NULL) {
    if (data)
      XFree (data);
    return FALSE;
  }

  if (strstr ((char*) data, (char*) bin_name) != NULL) {
    XFree (data);
    return TRUE;
  }

  XFree (data);
  return FALSE;
}

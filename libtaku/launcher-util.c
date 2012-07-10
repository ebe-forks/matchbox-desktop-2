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
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "launcher-util.h"
#include "xutil.h"

#ifdef USE_LIBSN
#define SN_API_NOT_YET_FROZEN 1
#include <libsn/sn.h>
#endif

/* Convert command line to argv array, stripping % conversions on the way */
#define MAX_ARGS 255
char **
exec_to_argv (const char *exec)
{
  const char *p;
  char *buf, *bufp, **argv;
  int nargs;
  gboolean escape, single_quote, double_quote;
  
  argv = g_new (char *, MAX_ARGS + 1);
  buf = g_alloca (strlen (exec) + 1);
  bufp = buf;
  nargs = 0;
  escape = single_quote = double_quote = FALSE;
  
  for (p = exec; *p; p++) {
    if (escape) {
      *bufp++ = *p;
      
      escape = FALSE;
    } else {
      switch (*p) {
      case '\\':
        escape = TRUE;

        break;
      case '%':
        /* Strip '%' conversions */
        if (p[1] && p[1] == '%')
          *bufp++ = *p;
        
        p++;

        break;
      case '\'':
        if (double_quote)
          *bufp++ = *p;
        else
          single_quote = !single_quote;
        
        break;
      case '\"':
        if (single_quote)
          *bufp++ = *p;
        else
          double_quote = !double_quote;
        
        break;
      case ' ':
        if (single_quote || double_quote)
          *bufp++ = *p;
        else {
          *bufp = 0;
          
          if (nargs < MAX_ARGS)
            argv[nargs++] = g_strdup (buf);
          
          bufp = buf;
        }
        
        break;
      default:
        *bufp++ = *p;
        break;
      }
    }
  }
  
  if (bufp != buf) {
    *bufp = 0;
    
    if (nargs < MAX_ARGS)
      argv[nargs++] = g_strdup (buf);
  }
  
  argv[nargs] = NULL;
  
  return argv;
}

/* Strips extension off filename */
static char *
strip_extension (const char *file)
{
        char *stripped, *p;

        stripped = g_strdup (file);

        p = strrchr (stripped, '.');
        if (p &&
            (!strcmp (p, ".png") ||
             !strcmp (p, ".svg") ||
             !strcmp (p, ".xpm")))
	        *p = 0;

        return stripped;
}

#define MISSING_IMAGE "gtk-missing-image"
#define GENERIC_EXECUTABLE "application-x-executable"

GdkPixbuf*
get_icon (const gchar *name, gint pixel_size)
{
  static GtkIconTheme *theme = NULL;
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  gchar *stripped = NULL;
  gint width, height;

  if (G_UNLIKELY (theme == NULL))
    theme = gtk_icon_theme_get_default ();

  if (name == NULL) {
    return get_icon (GENERIC_EXECUTABLE, pixel_size);
  }

  if (g_path_is_absolute (name)) {
    pixbuf = gdk_pixbuf_new_from_file_at_scale (name, pixel_size, pixel_size,
                                                TRUE, &error);
    if (error) {
      g_warning ("Error loading icon: %s", error->message);
      g_error_free (error);
      error = NULL;
    }
    return pixbuf;
  }

  stripped = strip_extension (name);
  
  pixbuf = gtk_icon_theme_load_icon (theme,
                                     stripped,
                                     pixel_size,
                                     0, &error);
  if (error) {
    g_warning ("Error loading icon: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  g_free (stripped);

  /* Fallback on generic executable, then missing image */
  if (pixbuf == NULL) {
    if (strcmp (name, MISSING_IMAGE) == 0) {
      return NULL;
    } else if (strcmp (name, GENERIC_EXECUTABLE) == 0) {
      return get_icon (MISSING_IMAGE, pixel_size);
    } else {
      return get_icon (GENERIC_EXECUTABLE, pixel_size);
    }
  }
 
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width != pixel_size || height != pixel_size) {
    GdkPixbuf *new;
    
    new = gdk_pixbuf_scale_simple (pixbuf,
                                   pixel_size, pixel_size,
                                   GDK_INTERP_BILINEAR);
    
    g_object_unref (pixbuf);
    pixbuf = new;
  }

  return pixbuf;
}


static void
child_setup (gpointer user_data)
{
#ifdef USE_LIBSN
  if (user_data) {
    sn_launcher_context_setup_child_process (user_data);
  }
#endif
}


/* TODO: optionally link to GtkUnique and directly handle that? */
void
launcher_start (GtkWidget *widget, 
                TakuMenuItem *item, 
                gchar **argv,
                gboolean use_sn,
                gboolean single_instance)
{
  GError *error = NULL;
#ifdef USE_LIBSN
  SnLauncherContext *context;
#endif

  /* Check for an existing instance if Matchbox single instance */
  if (single_instance) {
    Window win_found;

    if (mb_single_instance_is_starting (argv[0]))
      return;

    win_found = mb_single_instance_get_window (argv[0]);
    if (win_found != None) {
      x_window_activate (win_found, gtk_get_current_event_time ());

      return;
    }
  }
  
#ifdef USE_LIBSN
  context = NULL;
  
  if (use_sn) {
    SnDisplay *sn_dpy;
    Display *display;
    int screen;
    
    display = gdk_x11_display_get_xdisplay (gtk_widget_get_display (widget));
    sn_dpy = sn_display_new (display, NULL, NULL);
    
    screen = gdk_screen_get_number (gtk_widget_get_screen (widget));
    context = sn_launcher_context_new (sn_dpy, screen);
    sn_display_unref (sn_dpy);
    
    sn_launcher_context_set_name (context, taku_menu_item_get_name (item));
    sn_launcher_context_set_binary_name (context, argv[0]);
    /* TODO: set workspace, steal gedit_utils_get_current_workspace */
    
    sn_launcher_context_initiate (context,
                                  g_get_prgname () ?: "unknown",
                                  argv[0],
                                  gtk_get_current_event_time ());
  }
#endif

  /* TODO: use GAppInfo */
  if (!g_spawn_async (NULL, argv, NULL,
                      G_SPAWN_SEARCH_PATH,
                      child_setup,
#ifdef USE_LIBSN
                      use_sn ? context : NULL,
#else
                      NULL,
#endif
                      NULL,
                      &error)) {
    g_warning ("Cannot launch %s: %s", argv[0], error->message);
    g_error_free (error);
#ifdef USE_LIBSN
    if (context)
      sn_launcher_context_complete (context);
#endif
  }
  
#ifdef USE_LIBSN
  if (use_sn)
    sn_launcher_context_unref (context);
#endif
}


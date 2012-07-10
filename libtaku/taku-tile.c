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

#include <gtk/gtk.h>
#include "taku-tile.h"

G_DEFINE_ABSTRACT_TYPE (TakuTile, taku_tile, GTK_TYPE_EVENT_BOX);

enum {
  ACTIVATE,
  CLICKED,
  LAST_SIGNAL,
};
static guint signals[LAST_SIGNAL];

static gboolean
taku_tile_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  GtkStyleContext *context;
  int width, height;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  if (gtk_style_context_get_state (context) != GTK_STATE_FLAG_ACTIVE &&
      gtk_widget_has_focus (widget)) {
    gtk_style_context_set_state (context, GTK_STATE_SELECTED);

    gtk_render_background (context, cr, 0, 0, width, height);
  }

  return FALSE;
}

/*
 * The tile was clicked, so start the state animation and fire the signal.
 */
static void
taku_tile_clicked (TakuTile *tile)
{
  g_return_if_fail (TAKU_IS_TILE (tile));

  g_signal_emit (tile, signals[CLICKED], 0);
}

/*
 * This is called by GtkWidget when the tile is activated.  Generally this means
 * the user has focused the tile and pressed [return] or [space].
 */
static void
taku_tile_real_activate (TakuTile *tile)
{
  taku_tile_clicked (tile);
}

/*
 * Callback when mouse enters the tile.
 */
static gboolean
taku_tile_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
  TakuTile *tile = (TakuTile *)widget;
  tile->in_tile = TRUE;

  return TRUE;
}

/*
 * Callback when mouse leaves the tile.
 */
static gboolean
taku_tile_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
  TakuTile *tile = (TakuTile *)widget;
  tile->in_tile = FALSE;

  return TRUE;
}

/*
 * Callback when a button is released inside the tile.
 */
static gboolean
taku_tile_button_release (GtkWidget *widget, GdkEventButton *event)
{
  TakuTile *tile = (TakuTile *)widget;
  if ((event->button == 1) && (tile->in_tile)) {
    gtk_widget_grab_focus (widget);
    taku_tile_clicked (tile);
  }
  
  return TRUE;
}

static void
taku_tile_class_init (TakuTileClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->button_release_event = taku_tile_button_release;
  widget_class->enter_notify_event = taku_tile_enter_notify;
  widget_class->leave_notify_event = taku_tile_leave_notify;

  klass->activate = taku_tile_real_activate;

  /* Hook up the activate signal so we get keyboard handling for free */
  signals[ACTIVATE] = g_signal_new ("activate",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                    G_STRUCT_OFFSET (TakuTileClass, activate),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
  widget_class->activate_signal = signals[ACTIVATE];

  signals[CLICKED] = g_signal_new ("clicked",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                    G_STRUCT_OFFSET (TakuTileClass, clicked),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
}

static void
taku_tile_init (TakuTile *self)
{
  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  g_signal_connect (self, "draw", G_CALLBACK (taku_tile_draw), NULL);
}

GtkWidget *
taku_tile_new (void)
{
  return g_object_new (TAKU_TYPE_TILE, NULL);
}

const char *
taku_tile_get_sort_key (TakuTile *tile)
{
  TakuTileClass *class;

  g_return_val_if_fail (TAKU_IS_TILE (tile), NULL);

  class = TAKU_TILE_GET_CLASS (tile);
  if (class->get_sort_key != NULL)
    return class->get_sort_key (tile);
  else
    return NULL;
}

const char *
taku_tile_get_search_key (TakuTile *tile)
{
  TakuTileClass *class;

  g_return_val_if_fail (TAKU_IS_TILE (tile), NULL);

  class = TAKU_TILE_GET_CLASS (tile);
  if (class->get_search_key != NULL)
    return class->get_search_key (tile);
  else
    return NULL;
}

gboolean
taku_tile_matches_filter (TakuTile *tile, gpointer filter)
{
  TakuTileClass *class;

  g_return_val_if_fail (TAKU_IS_TILE (tile), FALSE);
  g_return_val_if_fail (filter != NULL, FALSE);

  class = TAKU_TILE_GET_CLASS (tile);
  if (class->matches_filter != NULL)
    return class->matches_filter (tile, filter);
  else
    return TRUE;
}

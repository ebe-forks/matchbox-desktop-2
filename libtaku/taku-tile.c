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

/*
 * TODO: either admit that we only have launchers and merge this with
 * TakuLauncherTile, or turn this into a GInterface and actually introduce
 * alternative tiles (BackTile and hierarchical navigation?)
 */

#include <gtk/gtk.h>
#include "taku-tile.h"

G_DEFINE_ABSTRACT_TYPE (TakuTile, taku_tile, GTK_TYPE_BIN);

static void
taku_tile_class_init (TakuTileClass *klass)
{
}

static void
taku_tile_init (TakuTile *self)
{
  gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (self), FALSE);
}

/* TODO steal GtkButton's draw function and comment out the focus draw code */

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

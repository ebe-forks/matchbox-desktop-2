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
#include "taku-icon-tile.h"

G_DEFINE_TYPE (TakuIconTile, taku_icon_tile, TAKU_TYPE_TILE);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TAKU_TYPE_ICON_TILE, TakuIconTilePrivate))

struct _TakuIconTilePrivate
{
  GtkWidget *icon;
  GtkWidget *primary;
  GtkWidget *secondary;
  gchar *collation_key;
  GtkOrientation orientation;
};

enum {
  PROP_0,
  PROP_PIXBUF,
  PROP_PRIMARY,
  PROP_SECONDARY,
};

static void
tile_arrange (TakuIconTile *tile)
{
  GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;
  gboolean show_secondary = TRUE;
  
  gtk_widget_style_get (GTK_WIDGET (tile),
                        "show-secondary-text", &show_secondary,
                        "orientation", &orientation,
                        NULL);
  
  if (orientation != tile->priv->orientation) {
    GtkWidget *box, *vbox;
    if (tile->priv->orientation != -1) {
      gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (tile)));
    }
    
    tile->priv->orientation = orientation;
    gtk_widget_show (tile->priv->icon);
    gtk_widget_show (tile->priv->primary);
    
    switch (orientation) {
    case GTK_ORIENTATION_VERTICAL :
      gtk_label_set_xalign (GTK_LABEL (tile->priv->primary), 0.5);
      gtk_label_set_xalign (GTK_LABEL (tile->priv->secondary), 0.5);
      box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      break;
    default:
    case GTK_ORIENTATION_HORIZONTAL :
      gtk_label_set_xalign (GTK_LABEL (tile->priv->primary), 0.0);
      gtk_label_set_xalign (GTK_LABEL (tile->priv->secondary), 0.0);
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      break;
    }
    
    gtk_widget_show (box);
    
    gtk_box_pack_start (GTK_BOX (box), tile->priv->icon, FALSE, FALSE, 0);
    
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_show (vbox);

    gtk_box_pack_start (GTK_BOX (vbox), tile->priv->primary, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), tile->priv->secondary, TRUE, TRUE, 0);
    
    gtk_box_pack_start (GTK_BOX (box), vbox, TRUE, TRUE, 0);
    
    gtk_container_add (GTK_CONTAINER (tile), box);
  }
  
  if (show_secondary) {
    gtk_widget_show (tile->priv->secondary);
  } else {
    gtk_widget_hide (tile->priv->secondary);
  }
}

static void
taku_icon_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  case PROP_PIXBUF:
    {
      TakuIconTilePrivate *priv = GET_PRIVATE (object);
      g_value_set_object (value, gtk_image_get_pixbuf (GTK_IMAGE (priv->icon)));
    }
    break;
  case PROP_PRIMARY:
    g_value_set_string (value, taku_icon_tile_get_primary (TAKU_ICON_TILE (object)));
    break;
  case PROP_SECONDARY:
    g_value_set_string (value, taku_icon_tile_get_secondary (TAKU_ICON_TILE (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
taku_icon_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  case PROP_PIXBUF:
    taku_icon_tile_set_pixbuf (TAKU_ICON_TILE (object),
                               g_value_get_object (value));
    break;
  case PROP_PRIMARY:
    taku_icon_tile_set_primary (TAKU_ICON_TILE (object),
                                g_value_get_string (value));
    break;
  case PROP_SECONDARY:
    taku_icon_tile_set_secondary (TAKU_ICON_TILE (object),
                                  g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
taku_icon_tile_finalize (GObject *object)
{
  g_free (TAKU_ICON_TILE (object)->priv->collation_key);
  
  G_OBJECT_CLASS (taku_icon_tile_parent_class)->finalize (object);
}

static void
taku_icon_tile_style_set (GtkWidget *widget, GtkStyle *previous)
{
  GTK_WIDGET_CLASS (taku_icon_tile_parent_class)->style_set (widget, previous);

  tile_arrange (TAKU_ICON_TILE (widget));
}

static const char *
taku_icon_tile_get_sort_key (TakuTile *tile)
{
  return TAKU_ICON_TILE (tile)->priv->collation_key;
}

static const char *
taku_icon_tile_get_search_key (TakuTile *tile)
{
  return taku_icon_tile_get_primary (TAKU_ICON_TILE (tile));
}

static void
taku_icon_tile_dispose (GObject *object)
{
  TakuIconTilePrivate *priv = GET_PRIVATE (object);

  if (priv->icon) {
    g_object_unref (priv->icon);
    priv->icon = NULL;
  }

  if (priv->primary) {
    g_object_unref (priv->primary);
    priv->primary = NULL;
  }

  if (priv->secondary) {
    g_object_unref (priv->secondary);
    priv->secondary = NULL;
  }

  G_OBJECT_CLASS (taku_icon_tile_parent_class)->dispose (object);
}

static void
taku_icon_tile_class_init (TakuIconTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  TakuTileClass *tile_class = TAKU_TILE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (TakuIconTilePrivate));

  object_class->get_property = taku_icon_tile_get_property;
  object_class->set_property = taku_icon_tile_set_property;
  object_class->dispose = taku_icon_tile_dispose;
  object_class->finalize = taku_icon_tile_finalize;

  widget_class->style_set = taku_icon_tile_style_set;

  tile_class->get_sort_key = taku_icon_tile_get_sort_key;
  tile_class->get_search_key = taku_icon_tile_get_search_key;

  g_object_class_install_property (object_class, PROP_PIXBUF,
                                   g_param_spec_object ("pixbuf", "pixbuf",
                                                        "The pixbuf",
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PRIMARY,
                                   g_param_spec_string ("primary", "primary",
                                                        "The primary string",
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SECONDARY,
                                   g_param_spec_string ("secondary", "secondary",
                                                        "The secondary string",
                                                        "",
                                                        G_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("show-secondary-text",
                                                                 "show secondary text",
                                                                 "show secondary text",
                                                                 TRUE,
                                                                 G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("orientation",
                                                              "orientation",
                                                              "The orientation of the tile layout",
                                                              GTK_TYPE_ORIENTATION,
                                                              GTK_ORIENTATION_HORIZONTAL,
                                                              G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_uint ("taku-icon-size",
                                                              "Taku icon size",
                                                              "Icon size used by all Taku Icons",
                                                              0,
                                                              256,
                                                              64,
                                                              G_PARAM_READABLE));

}

static void
make_bold (GtkLabel *label)
{
  static PangoAttrList *list = NULL;

  if (list == NULL) {
    PangoAttribute *attr;

    list = pango_attr_list_new ();
    
    attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
    attr->start_index = 0;
    attr->end_index = G_MAXUINT;
    pango_attr_list_insert (list, attr);
    
    attr = pango_attr_scale_new (1.2);
    attr->start_index = 0;
    attr->end_index = G_MAXUINT;
    pango_attr_list_insert (list, attr);
  } 

  gtk_label_set_attributes (label, list);
}

static void
taku_icon_tile_init (TakuIconTile *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->icon = gtk_image_new ();
  gtk_widget_show (self->priv->icon);
  g_object_ref (self->priv->icon);

  self->priv->primary = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (self->priv->primary), PANGO_ELLIPSIZE_END);
  make_bold (GTK_LABEL (self->priv->primary));
  gtk_widget_show (self->priv->primary);
  g_object_ref (self->priv->primary);

  self->priv->secondary = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (self->priv->secondary), PANGO_ELLIPSIZE_END);
  gtk_widget_show (self->priv->secondary);
  g_object_ref (self->priv->secondary);

  self->priv->orientation = -1;
}

GtkWidget *
taku_icon_tile_new (void)
{
  return g_object_new (TAKU_TYPE_ICON_TILE, NULL);
}

void
taku_icon_tile_set_pixbuf (TakuIconTile *tile, GdkPixbuf *pixbuf)
{
  g_return_if_fail (TAKU_IS_ICON_TILE (tile));

  gtk_image_set_from_pixbuf (GTK_IMAGE (tile->priv->icon), pixbuf);

  g_object_notify (G_OBJECT (tile), "pixbuf");
}

void
taku_icon_tile_set_icon_name (TakuIconTile *tile, const char *name)
{
  uint size;
  g_return_if_fail (TAKU_IS_ICON_TILE (tile));

  gtk_widget_style_get (GTK_WIDGET (tile),
                        "taku-icon-size", &size,
                        NULL);

  gtk_image_set_from_icon_name (GTK_IMAGE (tile->priv->icon),
                                name, size);
}

void
taku_icon_tile_set_primary (TakuIconTile *tile, const char *text)
{
  g_return_if_fail (TAKU_IS_ICON_TILE (tile));

  gtk_label_set_text (GTK_LABEL (tile->priv->primary), text);

  if (tile->priv->collation_key)
    g_free (tile->priv->collation_key);
  
  if (text) {
    gchar *text_casefold = g_utf8_casefold (text, -1);
    tile->priv->collation_key = g_utf8_collate_key (text_casefold, -1);
    g_free (text_casefold);
  } else {
    tile->priv->collation_key = NULL;
  }

  atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (tile)), text ?: "");

  g_object_notify (G_OBJECT (tile), "primary");
}

const char *
taku_icon_tile_get_primary (TakuIconTile *tile)
{
  return gtk_label_get_text (GTK_LABEL (tile->priv->primary));
}

void
taku_icon_tile_set_secondary (TakuIconTile *tile, const char *text)
{
  g_return_if_fail (TAKU_IS_ICON_TILE (tile));

  gtk_label_set_text (GTK_LABEL (tile->priv->secondary), text);

  g_object_notify (G_OBJECT (tile), "secondary");
}

const char *
taku_icon_tile_get_secondary (TakuIconTile *tile)
{
  return gtk_label_get_text (GTK_LABEL (tile->priv->secondary));
}

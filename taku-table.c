#include <gtk/gtk.h>
#include "taku-table.h"

G_DEFINE_TYPE (TakuTable, taku_table, GTK_TYPE_TABLE);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TAKU_TYPE_TABLE, TakuTablePrivate))

struct _TakuTablePrivate
{
  int columns;
  int x, y;
};

static gboolean
on_tile_focus (GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
  GtkWidget *viewport = GTK_WIDGET (user_data)->parent;
  GtkAdjustment *adjustment;
  
  /* If the lowest point of the tile is lower than the height of the viewport, or
     if the top of the tile is higher than the viewport is... */
  if (widget->allocation.y + widget->allocation.height > viewport->allocation.height ||
      widget->allocation.y < viewport->allocation.height) {
    adjustment = gtk_viewport_get_vadjustment (GTK_VIEWPORT (viewport));
    
    gtk_adjustment_clamp_page (adjustment,
                               widget->allocation.y,
                               widget->allocation.y + widget->allocation.height);
  }

  return FALSE;
}

/*
 * Implementation of gtk_container_add, so that applications can just call that
 * and this class manages the position.
 */
static void
container_add (GtkContainer *container, GtkWidget *widget)
{
  TakuTable *self = TAKU_TABLE (container);

  g_return_if_fail (self);

  /* Attach the tile to the current coordinates */
  gtk_table_attach (GTK_TABLE (container), widget,
                    self->priv->x, self->priv->x + 1,
                    self->priv->y, self->priv->y + 1,
                    GTK_EXPAND | GTK_FILL | GTK_SHRINK,
                    0,
                    0, 0);

  /* If we've reached the end of the row, move to the next one */
  if (++self->priv->x >= self->priv->columns) {
    self->priv->x = 0;
    self->priv->y++;
  }
  
  g_signal_connect (widget, "focus-in-event", G_CALLBACK (on_tile_focus), self);
}

static void
reflow (GtkWidget *widget, gpointer user_data)
{
  TakuTable *table = TAKU_TABLE (user_data);
  GtkContainer *container = GTK_CONTAINER (user_data);

  gtk_container_child_set (container, widget,
                           "left-attach", table->priv->x,
                           "right-attach", table->priv->x + 1,
                           "top-attach", table->priv->y,
                           "bottom-attach", table->priv->y + 1,
                           NULL);
  if (++table->priv->x >= table->priv->columns) {
    table->priv->x = 0;
    table->priv->y++;
  }
}

static void
calculate_columns (GtkWidget *widget)
{
  static gboolean reflowing = FALSE;
  TakuTable *table = TAKU_TABLE (widget);
  PangoContext *context;
  PangoFontMetrics *metrics;
  int width, new_cols;

  /* If we are currently reflowing the tiles, or the final allocation hasn't
     been decided yet, return */
  if (reflowing || widget->allocation.width <= 1)
    return;

  context = gtk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context, widget->style->font_desc, NULL);

  width = PANGO_PIXELS (25 * pango_font_metrics_get_approximate_char_width (metrics));
  new_cols = widget->allocation.width / width;

  if (table->priv->columns != new_cols) {
    table->priv->columns = new_cols;
    table->priv->x = table->priv->y = 0;
    gtk_table_resize (GTK_TABLE (table), 1, 1);
    reflowing = TRUE;
    gtk_container_foreach (GTK_CONTAINER (table), reflow, table);
    reflowing = FALSE;
  }

  pango_font_metrics_unref (metrics);
}

static void
taku_table_size_allocate (GtkWidget     *widget,
                         GtkAllocation *allocation)
{
  /* TODO: to work around viewport bug, connect to the scrolled window's
     size-allocate instead */
  (* GTK_WIDGET_CLASS (taku_table_parent_class)->size_allocate) (widget, allocation);

  calculate_columns (widget);
}

static void
taku_table_style_set (GtkWidget *widget,
                     GtkStyle  *previous_style)
{
  (* GTK_WIDGET_CLASS (taku_table_parent_class)->style_set) (widget, previous_style);

  calculate_columns (widget);
}


static void
taku_table_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
taku_table_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
taku_table_class_init (TakuTableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (TakuTablePrivate));

  object_class->get_property = taku_table_get_property;
  object_class->set_property = taku_table_set_property;
  
  widget_class->size_allocate = taku_table_size_allocate;
  widget_class->style_set = taku_table_style_set;
  
  container_class->add = container_add;
}

static void
taku_table_init (TakuTable *self)
{
  self->priv = GET_PRIVATE (self);

  gtk_container_set_border_width (GTK_CONTAINER (self), 6);
  gtk_table_set_homogeneous (GTK_TABLE (self), TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (self), 6);
  gtk_table_set_col_spacings (GTK_TABLE (self), 6);
  self->priv->columns = 2;
}

GtkWidget *
taku_table_new (void)
{
  return g_object_new (TAKU_TYPE_TABLE, NULL);
}

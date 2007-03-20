#include <gtk/gtk.h>
#include "eggsequence.h"
#include "taku-table.h"
#include "taku-icon-tile.h"

G_DEFINE_TYPE (TakuTable, taku_table, GTK_TYPE_TABLE);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TAKU_TYPE_TABLE, TakuTablePrivate))

struct _TakuTablePrivate
{
  int columns;
  int x, y;
  gboolean reflowing;
  EggSequence *seq;
  GList *dummies;
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

static void
reflow_foreach (gpointer widget, gpointer user_data)
{
  TakuTable *table = TAKU_TABLE (user_data);
  GtkContainer *container = GTK_CONTAINER (user_data);

  gtk_container_child_set (container, GTK_WIDGET (widget),
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
reflow (TakuTable *table)
{
  int i;

  /* Only reflow when necessary */
  if (!GTK_WIDGET_VISIBLE (table))
    return;

  /* Remove dummies */
  while (table->priv->dummies) {
    /* Call into the parent class straight away in order to bypass our
     * own remove implementation. */
    (* GTK_CONTAINER_CLASS (taku_table_parent_class)->remove)
      (GTK_CONTAINER (table), table->priv->dummies->data);

    table->priv->dummies = g_list_delete_link (table->priv->dummies,
                                               table->priv->dummies);
  }

  /* Reflow children */
  table->priv->x = table->priv->y = 0;

  table->priv->reflowing = TRUE;
  egg_sequence_foreach (table->priv->seq, reflow_foreach, table);
  table->priv->reflowing = FALSE;

  /* Add dummy items if necessary to get required amount of columns */
  for (i = egg_sequence_get_length (table->priv->seq);
       i < table->priv->columns; i++) {
    GtkWidget *dummy = gtk_label_new (NULL);
    gtk_widget_show (dummy);

    gtk_table_attach (GTK_TABLE (table),
                      dummy,
                      table->priv->x,
                      table->priv->x + 1,
                      table->priv->y,
                      table->priv->y + 1,
                      GTK_FILL | GTK_EXPAND,
                      GTK_FILL,
                      0,
                      0);
    table->priv->x++;

    table->priv->dummies = g_list_prepend (table->priv->dummies, dummy);
  }

  /* Crop table */
  gtk_table_resize (GTK_TABLE (table), 1, 1);
}

static int
sort (gconstpointer a,
      gconstpointer b,
      gpointer      user_data)
{
  TakuIconTile *ta, *tb;

  ta = TAKU_ICON_TILE (a);
  tb = TAKU_ICON_TILE (b);

  return g_utf8_collate (taku_icon_tile_get_primary (ta),
                         taku_icon_tile_get_primary (tb));
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
  g_return_if_fail (TAKU_IS_ICON_TILE (widget));

  egg_sequence_insert_sorted (self->priv->seq, widget, sort, NULL);

  gtk_table_attach (GTK_TABLE (container), widget, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  reflow (self);

  g_signal_connect (widget, "focus-in-event", G_CALLBACK (on_tile_focus), self);
}

static int
search (gconstpointer a, gconstpointer b, gpointer user_data)
{
  if (a < b)
    return -1;
  else if (a > b)
    return 1;
  else
    return 0;
}

static void
container_remove (GtkContainer *container, GtkWidget *widget)
{
  TakuTable *self = TAKU_TABLE (container);
  EggSequenceIter *iter;

  g_return_if_fail (self);

  /* Find the appropriate iter first */
  iter = egg_sequence_search (self->priv->seq,
                              widget,
                              search,
                              NULL);
  iter = egg_sequence_iter_prev (iter);
  if (egg_sequence_get (iter) != widget)
    return;

  /* And then remove it */
  egg_sequence_remove (iter);

  (* GTK_CONTAINER_CLASS (taku_table_parent_class)->remove) (container, widget);

  reflow (self);
}

static void
calculate_columns (GtkWidget *widget)
{
  TakuTable *table = TAKU_TABLE (widget);
  PangoContext *context;
  PangoFontMetrics *metrics;
  int width, new_cols;

  /* If we are currently reflowing the tiles, or the final allocation hasn't
     been decided yet, return */
  if (!GTK_WIDGET_VISIBLE (widget) || table->priv->reflowing ||
      widget->allocation.width <= 1)
    return;

  context = gtk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context, widget->style->font_desc, NULL);

  width = PANGO_PIXELS (25 * pango_font_metrics_get_approximate_char_width (metrics));
  new_cols = MAX (1, widget->allocation.width / width);

  if (table->priv->columns != new_cols) {
    table->priv->columns = new_cols;

    reflow (table);
  }

  pango_font_metrics_unref (metrics);
}

static void
taku_table_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  /* TODO: to work around viewport bug, connect to the scrolled window's
     size-allocate instead */
  widget->allocation = *allocation;

  calculate_columns (widget);

  (* GTK_WIDGET_CLASS (taku_table_parent_class)->size_allocate) (widget, allocation);
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
taku_table_finalize (GObject *object)
{
  TakuTable *table = TAKU_TABLE (object);

  egg_sequence_free (table->priv->seq);

  g_list_free (table->priv->dummies);

  G_OBJECT_CLASS (taku_table_parent_class)->finalize (object);
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
  object_class->finalize     = taku_table_finalize;
  
  widget_class->size_allocate = taku_table_size_allocate;
  widget_class->style_set = taku_table_style_set;
  
  container_class->add    = container_add;
  container_class->remove = container_remove;
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

  self->priv->reflowing = FALSE;

  self->priv->seq = egg_sequence_new (NULL);

  self->priv->dummies = NULL;
}

GtkWidget *
taku_table_new (void)
{
  return g_object_new (TAKU_TYPE_TABLE, NULL);
}

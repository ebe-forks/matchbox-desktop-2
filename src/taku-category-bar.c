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

#include "libtaku/taku-launcher-tile.h"

#include "taku-category-bar.h"

G_DEFINE_TYPE (TakuCategoryBar, taku_category_bar, GTK_TYPE_BOX);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TAKU_TYPE_CATEGORY_BAR, TakuCategoryBarPrivate))

#define LIST_DATA "taku-category-bar:list"

typedef struct {
  GtkFlowBox *table;
  GList *categories;
  GList *current_category;
  GtkWidget *prev_button, *popup_button, *next_button;
  GtkLabel *switcher_label;
} TakuCategoryBarPrivate;


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

/* Changes the current category: Updates the switcher label and table filter */
static void
set_category (TakuCategoryBar *bar, GList *category_list_item)
{
  TakuCategoryBarPrivate *priv = GET_PRIVATE (bar);
  TakuLauncherCategory *category = NULL;
   
  if (!category_list_item)
    return;
  category = category_list_item->data;

  gtk_label_set_text (priv->switcher_label, category->name);
  priv->current_category = category_list_item;
  gtk_flow_box_invalidate_filter (priv->table);
}

static void
prev_category (TakuCategoryBar *bar)
{
  TakuCategoryBarPrivate *priv;
  
  g_return_if_fail (TAKU_IS_CATEGORY_BAR (bar));
  priv = GET_PRIVATE (bar);

  if (!priv->current_category)
    return;

 if (priv->current_category->prev == NULL)
    priv->current_category = g_list_last (priv->categories);
  else
    priv->current_category = priv->current_category->prev;

  set_category (bar, priv->current_category);
}

static void
next_category (TakuCategoryBar *bar)
{
  TakuCategoryBarPrivate *priv;
  
  g_return_if_fail (TAKU_IS_CATEGORY_BAR (bar));
  priv = GET_PRIVATE (bar);  
  
  if (!priv->current_category)
    return;

  if (priv->current_category->next == NULL)
    priv->current_category = priv->categories;
  else
    priv->current_category = priv->current_category->next;

  set_category (bar, priv->current_category);
}

static void
position_menu (GtkMenu *menu,
               int *x, int *y, gboolean *push_in, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GtkAllocation allocation;

  gdk_window_get_origin (gtk_widget_get_window (widget), x, y);
  gtk_widget_get_allocation (widget, &allocation);

  *x += allocation.x;
  *y += allocation.y + allocation.height;

  *push_in = TRUE;
}

static void
popdown_menu (GtkMenuShell *menu_shell, gpointer user_data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (user_data);

  /* Button up */
  gtk_toggle_button_set_active (button, FALSE);

  /* Destroy menu */
  gtk_widget_destroy (GTK_WIDGET (menu_shell));
}

static void
menu_activated (GtkMenuItem *menu_item, gpointer user_data)
{
  TakuCategoryBar *bar = TAKU_CATEGORY_BAR (user_data);
  GList *l;

  l = g_object_get_data (G_OBJECT (menu_item), LIST_DATA);
  set_category (bar, l);
}

static gboolean
popup_menu (GtkWidget *button, GdkEventButton *event, gpointer user_data)
{
  TakuCategoryBar *bar;
  TakuCategoryBarPrivate *priv;
  GtkWidget *menu;
  GtkAllocation allocation;
  GList *l;

  bar = TAKU_CATEGORY_BAR (user_data);
  priv = GET_PRIVATE (bar);
  g_assert (priv);

  /* Button down */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  /* Create menu */
  menu = gtk_menu_new ();
  gtk_widget_get_allocation (button, &allocation);
  gtk_widget_set_size_request (menu, allocation.width, -1);

  g_signal_connect (menu, "selection-done", G_CALLBACK (popdown_menu), button);

  for (l = priv->categories; l; l = l->next) {
    TakuLauncherCategory *category = l->data;
    GtkWidget *menu_item, *label;

    menu_item = gtk_menu_item_new ();
    g_object_set_data (G_OBJECT (menu_item), LIST_DATA, l); /* need a better way */
    g_signal_connect (menu_item, "activate", G_CALLBACK (menu_activated), bar);
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    label = gtk_label_new (category->name);
    make_bold (GTK_LABEL (label));
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (menu_item), label);
  }

  /* Popup menu */
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL,
                  position_menu, button,
                  event->button, event->time);

  return TRUE;
}

static void
taku_category_bar_class_init (TakuCategoryBarClass *klass)
{
  g_type_class_add_private (klass, sizeof (TakuCategoryBarPrivate));
}

static void
taku_category_bar_init (TakuCategoryBar *bar)
{
  TakuCategoryBarPrivate *priv;
  GtkWidget *button;

  priv = GET_PRIVATE (bar);

  /* Previous button */

  priv->prev_button = button = gtk_button_new_from_icon_name ("go-previous-symbolic",
                                                              GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_name (button, "MatchboxDesktopPrevButton");
  atk_object_set_name (gtk_widget_get_accessible (button), "GroupPrevious");
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (prev_category), bar);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (bar), button, FALSE, TRUE, 0);

  /* Category name button */
  
  priv->popup_button = button = gtk_toggle_button_new ();
  atk_object_set_name (gtk_widget_get_accessible (button), "GroupButton");
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (popup_menu), bar);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (bar), button, TRUE, TRUE, 0);

  priv->switcher_label = GTK_LABEL (gtk_label_new (NULL));
  make_bold (GTK_LABEL (priv->switcher_label));
  gtk_widget_show (GTK_WIDGET (priv->switcher_label));
  gtk_container_add (GTK_CONTAINER (button), GTK_WIDGET (priv->switcher_label));

  /* Next button */

  priv->next_button = button = gtk_button_new_from_icon_name ("go-next-symbolic",
                                                              GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_name (button, "MatchboxDesktopNextButton");
  atk_object_set_name (gtk_widget_get_accessible (button), "GroupNext");

  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (next_category), bar);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (bar), button, FALSE, TRUE, 0);
}


/* Public methods */

GtkWidget *
taku_category_bar_new (void)
{
  return g_object_new (TAKU_TYPE_CATEGORY_BAR, NULL);
}

void
taku_category_bar_set_table (TakuCategoryBar *bar, GtkFlowBox *table)
{
  TakuCategoryBarPrivate *priv;

  g_return_if_fail (TAKU_IS_CATEGORY_BAR (bar));
  g_return_if_fail (GTK_IS_FLOW_BOX (table));

  priv = GET_PRIVATE (bar);
  
  /* TODO: ref table and unref in dispose */
  
  priv->table = table;
}

void
taku_category_bar_set_categories (TakuCategoryBar *bar, GList *categories)
{
  TakuCategoryBarPrivate *priv;

  g_return_if_fail (TAKU_IS_CATEGORY_BAR (bar));

  priv = GET_PRIVATE (bar);

  priv->categories = categories;
  
  if (categories) {
    /* TODO: handle category list of 1 by disabling the buttons */
    set_category (bar, categories);
  } else {
    gtk_label_set_text (priv->switcher_label, _("No categories available"));
    gtk_widget_set_sensitive (priv->prev_button, FALSE);
    gtk_widget_set_sensitive (priv->popup_button, FALSE);
    gtk_widget_set_sensitive (priv->next_button, FALSE);
  }
}

TakuLauncherCategory*
taku_category_bar_get_current (TakuCategoryBar *bar)
{
  TakuCategoryBarPrivate *priv;

  g_return_val_if_fail (TAKU_IS_CATEGORY_BAR (bar), NULL);
  priv = GET_PRIVATE (bar);

  return (TakuLauncherCategory*)priv->current_category->data;
}

void
taku_category_bar_next (TakuCategoryBar *bar)
{
  g_return_if_fail (TAKU_IS_CATEGORY_BAR (bar));

  next_category (bar);
}

void
taku_category_bar_previous (TakuCategoryBar *bar)
{
  g_return_if_fail (TAKU_IS_CATEGORY_BAR (bar));

  prev_category (bar);
}

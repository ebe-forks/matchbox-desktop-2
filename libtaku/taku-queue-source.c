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
#include <glib.h>
#include "taku-queue-source.h"

typedef struct {
  GSource source;
  GQueue *queue;
} TakuQueueSource;

static gboolean 
prepare (GSource *source, gint *timeout)
{
  TakuQueueSource *queue_source = (TakuQueueSource*)source;
  *timeout = -1;
  return !g_queue_is_empty (queue_source->queue);
}

static gboolean 
check (GSource *source)
{
  TakuQueueSource *queue_source = (TakuQueueSource*)source;
  return !g_queue_is_empty (queue_source->queue);
}

static gboolean
dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  if (!callback) {
    g_warning ("Queue source dispatched without callback\n"
               "You must call g_source_set_callback().");
    return FALSE;
  }
  
  return callback (user_data);
}

static GSourceFuncs funcs = {
  prepare, check, dispatch, NULL
};

/**
 * taku_idle_queue_add:
 * @queue: the #GQueue to watch
 * @function: the #GSourceFunc to call when @queue is not empty
 * @data: user data to pass to @function
 *
 * Add a function to be called whenever @queue has items in.
 */
guint
taku_idle_queue_add (GQueue *queue, GSourceFunc function, gpointer data)
{
  GSource *source;
  TakuQueueSource *queue_source;
  guint32 id;

  source = g_source_new (&funcs, sizeof (TakuQueueSource));
  g_source_set_priority (source, G_PRIORITY_DEFAULT_IDLE);

  queue_source = (TakuQueueSource*)source;
  queue_source->queue = queue;
  
  g_source_set_callback (source, function, data, NULL);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

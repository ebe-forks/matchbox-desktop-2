#ifndef _TAKU_TABLE
#define _TAKU_TABLE

#include <gtk/gtktable.h>
#include "taku-tile.h"

G_BEGIN_DECLS

#define TAKU_TYPE_TABLE taku_table_get_type()

#define TAKU_TABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TAKU_TYPE_TABLE, TakuTable))

#define TAKU_TABLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TAKU_TYPE_TABLE, TakuTableClass))

#define TAKU_IS_TABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TAKU_TYPE_TABLE))

#define TAKU_IS_TABLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TAKU_TYPE_TABLE))

#define TAKU_TABLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TAKU_TYPE_TABLE, TakuTableClass))

typedef struct _TakuTablePrivate TakuTablePrivate;

typedef struct {
  GtkTable parent;
  TakuTablePrivate *priv;
} TakuTable;

typedef struct {
  GtkTableClass parent_class;
} TakuTableClass;

GType taku_table_get_type (void);

GtkWidget* taku_table_new (void);

G_END_DECLS

#endif /* _TAKU_TABLE */

#ifndef _TAKU_ICON_TILE
#define _TAKU_ICON_TILE

#include "taku-tile.h"

G_BEGIN_DECLS

#define TAKU_TYPE_ICON_TILE taku_icon_tile_get_type()

#define TAKU_ICON_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TAKU_TYPE_ICON_TILE, TakuIconTile))

#define TAKU_ICON_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TAKU_TYPE_ICON_TILE, TakuIconTileClass))

#define TAKU_IS_ICON_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TAKU_TYPE_ICON_TILE))

#define TAKU_IS_ICON_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TAKU_TYPE_ICON_TILE))

#define TAKU_ICON_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TAKU_TYPE_ICON_TILE, TakuIconTileClass))

typedef struct _TakuIconTilePrivate TakuIconTilePrivate;

typedef struct {
  TakuTile parent;
  TakuIconTilePrivate *priv;
} TakuIconTile;

typedef struct {
  TakuTileClass parent_class;
} TakuIconTileClass;

GType taku_icon_tile_get_type (void);

GtkWidget* taku_icon_tile_new (void);

void taku_icon_tile_set_pixbuf (TakuIconTile *tile, GdkPixbuf *pixbuf);
void taku_icon_tile_set_icon_name (TakuIconTile *tile, const char *name);
void taku_icon_tile_set_primary (TakuIconTile *tile, const char *text);
const char *taku_icon_tile_get_primary (TakuIconTile *tile);
void taku_icon_tile_set_secondary (TakuIconTile *tile, const char *text);
const char *taku_icon_tile_get_secondary (TakuIconTile *tile);

G_END_DECLS

#endif /* _TAKU_ICON_TILE */

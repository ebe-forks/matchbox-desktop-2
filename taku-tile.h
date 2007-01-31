#ifndef _TAKU_TILE
#define _TAKU_TILE

#include <gtk/gtkeventbox.h>

G_BEGIN_DECLS

#define TAKU_TYPE_TILE taku_tile_get_type()

#define TAKU_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TAKU_TYPE_TILE, TakuTile))

#define TAKU_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TAKU_TYPE_TILE, TakuTileClass))

#define TAKU_IS_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TAKU_TYPE_TILE))

#define TAKU_IS_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TAKU_TYPE_TILE))

#define TAKU_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TAKU_TYPE_TILE, TakuTileClass))

typedef struct {
  GtkEventBox parent;
} TakuTile;

typedef struct {
  GtkEventBoxClass parent_class;
  void (* activate) (TakuTile *tile);
  void (* clicked) (TakuTile *tile);
} TakuTileClass;

GType taku_tile_get_type (void);

GtkWidget* taku_tile_new (void);

G_END_DECLS

#endif /* _TAKU_TILE */

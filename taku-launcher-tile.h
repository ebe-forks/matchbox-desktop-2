#ifndef _TAKU_LAUNCHER_TILE
#define _TAKU_LAUNCHER_TILE

#include "taku-icon-tile.h"

G_BEGIN_DECLS

#define TAKU_TYPE_LAUNCHER_TILE taku_launcher_tile_get_type()

#define TAKU_LAUNCHER_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  TAKU_TYPE_LAUNCHER_TILE, TakuLauncherTile))

#define TAKU_LAUNCHER_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  TAKU_TYPE_LAUNCHER_TILE, TakuLauncherTileClass))

#define TAKU_IS_LAUNCHER_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  TAKU_TYPE_LAUNCHER_TILE))

#define TAKU_IS_LAUNCHER_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  TAKU_TYPE_LAUNCHER_TILE))

#define TAKU_LAUNCHER_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  TAKU_TYPE_LAUNCHER_TILE, TakuLauncherTileClass))

typedef struct _TakuLauncherTilePrivate TakuLauncherTilePrivate;

typedef struct {
  TakuIconTile parent;
  TakuLauncherTilePrivate *priv;
} TakuLauncherTile;

typedef struct {
  TakuIconTileClass parent_class;
} TakuLauncherTileClass;

GType taku_launcher_tile_get_type (void);

GtkWidget* taku_launcher_tile_new (void);

GtkWidget * taku_launcher_tile_for_desktop_file (const char *filename);

G_END_DECLS

#endif /* _TAKU_LAUNCHER_TILE */

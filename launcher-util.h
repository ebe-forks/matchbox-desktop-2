#include <glib.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkwidget.h>

typedef struct {
  /*< public >*/
  char *name; /* Human-readable name */
  char *description; /* Description */
  char *icon; /* Icon name (can be NULL) */
  char *categories; /* Categories */
  /*< private >*/
  char **argv; /* argv to execute when starting this program */
  gboolean use_sn;
} LauncherData;

LauncherData *launcher_parse_desktop_file (const char *filename, GError **error);

void launcher_start (GtkWidget *widget, LauncherData *data);

char * launcher_get_icon (GtkIconTheme *icon_theme, LauncherData *data, int size);

void launcher_destroy (LauncherData *data);

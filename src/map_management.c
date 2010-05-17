

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gstring.h>
#include <glib/gprintf.h>
#include <gconf/gconf.h>
#include <math.h>
#include "support.h"
#include "globals.h"
#include "map_management.h"
#include "converter.h"
#include "tile_management.h"
#include "callbacks.h"
#include "friends.h"
#include "gps_functions.h"
#include "geo_photos.h"
#include "poi.h"
#include "wp.h"
#include "tracks.h"

void
fill_tiles_pixel(	int pixel_x,
			int pixel_y,
			int zoom,
			gboolean force_refresh)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_x",
				global_x,
				error);
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_y",
				global_y,
				error);
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_zoom",
				global_zoom,
				error);
}

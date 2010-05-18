#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/types.h> 
#include <curl/easy.h> 
#include <math.h>
#include <osm-gps-map.h>

#include "globals.h"
#include "tile_management.h"
#include "support.h"

GtkWidget *Bar = NULL; 
static GSList *tile_download_list = NULL;


int
update_thread_number (int change)
{
	static int current_number = 0;
	int ret_val;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	
	g_static_mutex_lock (&mutex);
	ret_val = current_number += change; 
	g_static_mutex_unlock (&mutex);

	return ret_val;
}
	
void
cb_download_maps(GtkWidget *dialog)
{
	GtkToggleButton *z1, *z2, *z3, *z4, *z5, *z6;
	int zoom_end = 1;
	
	int zoom;
	g_object_get(OSM_GPS_MAP(mapwidget), "zoom", &zoom, NULL);

	z1 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton2");
	z2 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton3");
	z3 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton4");
	z4 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton5");
	z5 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton6");
	z6 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton7");

	zoom_end = (gtk_toggle_button_get_active(z1)) ? zoom + 1 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z2)) ? zoom + 2 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z3)) ? zoom + 3 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z4)) ? zoom + 4 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z5)) ? zoom + 5 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z6)) ? zoom + 6 : zoom_end;
	
	coord_t c1,c2;
	osm_gps_map_get_bbox(OSM_GPS_MAP(mapwidget), &c1, &c2);
	printf("*** downloading tiles: bbox=%f,%f-%f,%f zoom=%d-%d\n", c1.rlat, c1.rlon, c2.rlat, c2.rlon, zoom+1, zoom_end);
	osm_gps_map_download_maps(OSM_GPS_MAP(mapwidget), &c1, &c2, global_zoom+1, zoom_end);
	gtk_widget_hide(dialog);
}



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
#include "gps_functions.h"
#include "callbacks.h"
#include "support.h"
#include "converter.h"
#include "geo_photos.h"
#include "friends.h"
#include "poi.h"
#include "wp.h"
#include "tracks.h"

GtkWidget *Bar = NULL; 
static GSList *tile_download_list = NULL;

typedef struct  {
	char *tile_url;
} Repo_data_t;

	

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
	
size_t 
cb_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	
	
	return 
	fwrite(ptr, size, nmemb, stream); 
}


size_t
cb_read_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fread(ptr, size, nmemb, stream);
}

int
cb_progress_func(GtkWidget *Bar,
                     double t, 
                     double d, 
                     double ultotal,
                     double ulnow)
{

  return 0;

}

void
cb_download_maps(GtkWidget *dialog)
{
	GtkToggleButton *z1, *z2, *z3, *z4, *z5, *z6;
	int zoom_end = 1;
	
	z1 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton2");
	z2 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton3");
	z3 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton4");
	z4 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton5");
	z5 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton6");
	z6 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton7");

	zoom_end = (gtk_toggle_button_get_active(z1)) ? global_zoom + 1 : zoom_end; 
	zoom_end = (gtk_toggle_button_get_active(z2)) ? global_zoom + 2 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z3)) ? global_zoom + 3 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z4)) ? global_zoom + 4 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z5)) ? global_zoom + 5 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z6)) ? global_zoom + 6 : zoom_end;
	
	coord_t c1,c2;
	osm_gps_map_get_bbox(OSM_GPS_MAP(mapwidget), &c1, &c2);
	osm_gps_map_download_maps(OSM_GPS_MAP(mapwidget), &c1, &c2, global_zoom+1, zoom_end);

	gtk_widget_hide(dialog);
}

bbox_t
get_bbox()
{
	bbox_t bbox;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	bbox.lat1 = pixel2lat(global_zoom, global_y);
	bbox.lon1 = pixel2lon(global_zoom, global_x);
	bbox.lat2 = pixel2lat(global_zoom, global_y + global_drawingarea_height);
	bbox.lon2 = pixel2lon(global_zoom, global_x + global_drawingarea_width);

	printf("BBOX: %f %f %f %f \n", bbox.lat1, bbox.lon1, bbox.lat2, bbox.lon2);
	
	return bbox;
}

bbox_t
get_bbox_deg()
{
	bbox_t bbox;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	bbox.lat1 = rad2deg( pixel2lat(global_zoom, global_y) );
	bbox.lon1 = rad2deg( pixel2lon(global_zoom, global_x) );
	bbox.lat2 = rad2deg( pixel2lat(global_zoom, global_y + global_drawingarea_height) );
	bbox.lon2 = rad2deg( pixel2lon(global_zoom, global_x + global_drawingarea_width) );

	printf("BBOX: %f %f %f %f \n", bbox.lat1, bbox.lon1, bbox.lat2, bbox.lon2);
	
	return bbox;
}

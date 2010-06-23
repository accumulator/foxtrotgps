
#include <string.h>
#include <glib/gprintf.h>
#include <osm-gps-map.h>

#include "support.h"
#include "converter.h"
#include "callbacks.h"
#include "wp.h"
#include "globals.h"

static GdkPixbuf	*wp_icon = NULL;
static GdkPixbuf	*myposition_icon = NULL;
static GdkGC		*gc_map = NULL;

/*
 * pass (0,0) to unset WP
 */
void
set_current_wp(float lat, float lon)
{
	GError *error = NULL;

	if (lat == 0 && lon == 0) {
		// unset wp
		printf("* unset WP\n");
		global_wp_on = FALSE;
		if (wp_icon) {
			osm_gps_map_remove_image(OSM_GPS_MAP(mapwidget), wp_icon);
		}
	}
	else
	{
		printf("* set WAYPOINT: lat %f - lon %f\n", lat, lon);
		if (global_wp_on && wp_icon) {
			osm_gps_map_remove_image(OSM_GPS_MAP(mapwidget), wp_icon);
		}
		global_wp_on = TRUE;
		global_wp.lat = lat;
		global_wp.lon = lon;
	
		if(!wp_icon)
		{
			wp_icon = gdk_pixbuf_new_from_file_at_size (
				PACKAGE_PIXMAPS_DIR "/" PACKAGE "-wp.png", 36, 36, &error);
			if (error)
			{
				g_print ("%s(): loading pixbuf failure. %s\n", __FUNCTION__, error->message);
				g_error_free (error);
			}
		}

		osm_gps_map_add_image_with_alignment(OSM_GPS_MAP(mapwidget), lat, lon, wp_icon, 0.06, 1.0);
	}
}

/* deprecated */
void
osd_wp()
{
	PangoContext		*context = NULL;
	PangoLayout		*layout  = NULL;
	PangoFontDescription	*desc    = NULL;
	
	GdkColor color;
	GdkGC *gc;
	
	gchar *buffer;
	static gchar distunit[3];
	
	int global_drawingarea_width;
	int global_drawingarea_height;

	static int width = 0, height = 0;

	float distance;
	double unit_conv = 1;
		
	//printf("* %s() deprecated\n", __PRETTY_FUNCTION__);
	return;

	if(gpsdata)
	{
		switch (global_speed_unit)
		{
			case 0:
				unit_conv = 1.0;
				g_sprintf(distunit, "%s", "km");
				break;
			case 1 :
				unit_conv = 1.0 / 1.609344;
				g_sprintf(distunit, "%s", "m");
				break;
			case 2 :
				unit_conv = 1.0 / 1.852;
				g_sprintf(distunit, "%s", "NM");
				break;		
		}

		distance = get_distance(
					gpsdata->fix.latitude, gpsdata->fix.longitude,
					global_wp.lat,global_wp.lon);
		buffer = g_strdup_printf("%.3f\n%.1f°", 
					distance*unit_conv, 
					gpsdata->fix.bearing);
		
		context = gtk_widget_get_pango_context (map_drawable);
		layout  = pango_layout_new (context);
		desc    = pango_font_description_new();
		
		pango_font_description_set_size (desc, 20 * PANGO_SCALE);
		pango_layout_set_font_description (layout, desc);
		pango_layout_set_text (layout, buffer, strlen(buffer));
	
	
		gc = gdk_gc_new (map_drawable->window);
	
		
		color.red = 0;
		color.green = 0;
		color.blue = 0;
		
		gdk_gc_set_rgb_fg_color (gc, &color);
	
	
		
		
		
		
		gdk_draw_drawable (
			map_drawable->window,
			map_drawable->style->fg_gc[GTK_WIDGET_STATE (map_drawable)],
			pixmap,
			global_drawingarea_width - width - 10, 
			global_drawingarea_height - height - 10,
			global_drawingarea_width - width - 10, 
			global_drawingarea_height - height - 10,
			width+10,height+10);


		
		pango_layout_get_pixel_size(layout, &width, &height);
		
		
		
			gdk_draw_layout(map_drawable->window,
					gc,
					global_drawingarea_width - width - 10, 
					global_drawingarea_height - height -10,
					layout);
		

	
	
		g_free(buffer);
		pango_font_description_free (desc);
		g_object_unref (layout);
		g_object_unref (gc);
	}
}

/*
 * give (0,0) to unset myposition
 */
void
set_myposition(float lat, float lon)
{
	GError *error = NULL;

	if (lat == 0 && lon == 0)
	{
		printf("* unset MYPOSITION\n");
		if (myposition_icon) {
			osm_gps_map_remove_image(OSM_GPS_MAP(mapwidget), myposition_icon);
		}
	}
	else
	{
		printf("* set MYPOSITION: lat %f - lon %f\n", lat, lon);
		if (myposition_icon) {
			osm_gps_map_remove_image(OSM_GPS_MAP(mapwidget), myposition_icon);
		}

		if(!myposition_icon)
		{
			myposition_icon = gdk_pixbuf_new_from_file_at_size (
				PACKAGE_PIXMAPS_DIR "/" PACKAGE "-myposition.png", 36, 36, &error);
			if (error)
			{
				g_print ("%s(): loading pixbuf failure. %s\n", __FUNCTION__, error->message);
				g_error_free (error);
			}
		}

		osm_gps_map_add_image_with_alignment(OSM_GPS_MAP(mapwidget), lat, lon, myposition_icon, 0.06, 1.0);
	}

	global_myposition.lat = lat;
	global_myposition.lon = lon;

	gconf_client_set_float(global_gconfclient, GCONF"/myposition_lat", lat, NULL);
	gconf_client_set_float(global_gconfclient, GCONF"/myposition_lon", lon, NULL);
}

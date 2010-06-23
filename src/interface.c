

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <osm-gps-map.h>
#include <osm-gps-map-osd.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget *
create_mapwidget(void)
{
	GtkWidget *widget = g_object_new (OSM_TYPE_GPS_MAP,
			"map-source", OSM_GPS_MAP_SOURCE_OPENSTREETMAP,
			"tile-cache", OSM_GPS_MAP_CACHE_AUTO,
			"proxy-uri", g_getenv("http_proxy"),
			NULL);

	OsmGpsMap *map = OSM_GPS_MAP(widget);

	OsmGpsMapLayer *osd = g_object_new (OSM_TYPE_GPS_MAP_OSD,
                        "show-scale",TRUE,
                        "show-coordinates",TRUE,
                        "show-crosshair",FALSE,
                        "show-dpad",FALSE,
                        "show-zoom",FALSE,
                        "show-gps-in-dpad",TRUE,
                        "show-gps-in-zoom",FALSE,
                        "dpad-radius", 30,
                        NULL);
    osm_gps_map_add_layer(OSM_GPS_MAP(widget), osd);
    g_object_unref(G_OBJECT(osd));

    g_signal_connect (G_OBJECT (widget), "button-press-event",
						G_CALLBACK (on_map_button_press_event), NULL);
    g_signal_connect (G_OBJECT (widget), "button-release-event",
						G_CALLBACK (on_map_button_release_event), NULL);
    g_signal_connect (G_OBJECT (widget), "notify::tiles-queued",
						G_CALLBACK(on_tiles_queued_changed), NULL);
    g_signal_connect (G_OBJECT (widget), "notify::zoom",
						G_CALLBACK(on_zoom_changed), NULL);

    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_FULLSCREEN, GDK_F11);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_UP, GDK_Up);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_DOWN, GDK_Down);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_LEFT, GDK_Left);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_RIGHT, GDK_Right);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_ZOOMIN, GDK_Page_Up);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_ZOOMOUT, GDK_Page_Down);

    return widget;
}

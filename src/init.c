

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <gconf/gconf.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include <osm-gps-map.h>

#include "globals.h"
#include "converter.h"
#include "gps_functions.h"
#include "support.h"
#include "callbacks.h"
#include "wp.h"


GSList *gconf_get_repolist();
void repoconfig__set_current_list_pointer();
void repoconfig__create_dropdown();


void
pre_init()
{
	GError	**err = NULL;

	g_type_init();

	trackpoint_list = g_queue_new();

	global_home_dir = getenv("HOME");

	global_gconfclient	= gconf_client_get_default();
	global_curr_reponame	= gconf_client_get_string(global_gconfclient, GCONF"/repo_name",err);


	if(global_curr_reponame == NULL)
	{
		printf("gconf repo_name not set\n");
		global_curr_reponame = g_strdup("OSM");
	}

	gconf_get_repolist();	
	repoconfig__set_current_list_pointer();
	
	global_detail_zoom = gconf_client_get_int (
			global_gconfclient, GCONF"/global_detail_zoom", err);

	global_server = gconf_client_get_string (
			global_gconfclient, GCONF"/gpsd_host", err);

	if (global_server == NULL)
	{
		printf("gconf GPSD address not set\n");
		global_server	= g_strdup("127.0.0.1");
	}

	global_port = gconf_client_get_string (
			global_gconfclient, GCONF "/gpsd_port", err);

	if (global_port == NULL)
	{
		printf("gconf GPSD port not set\n");
		global_port	= g_strdup("2947");
	}

	if(gconf_client_get_bool(global_gconfclient, GCONF"/started_before", err))
	{
		global_auto_download = gconf_client_get_bool(
				global_gconfclient, GCONF"/auto_download", err);
	}
	else
	{
		gconf_client_set_bool(global_gconfclient, GCONF"/started_before", TRUE, err);
		gconf_client_set_bool(global_gconfclient, GCONF"/auto_download", TRUE, err);
		global_auto_download = TRUE;
	}
}

void
init()
{
	GError	*err = NULL;
	const gchar *nick, *pass, *me_msg;
	GtkWidget *widget;
	gchar buffer[128];
	gboolean gconf_fftimer_running;
	char *str = NULL;
	
	
	foxtrotgps_dir = g_strconcat(global_home_dir, "/." PACKAGE, NULL);
	g_mkdir(foxtrotgps_dir, 0700);

	repoconfig__create_dropdown();

	gtk_widget_grab_focus(mapwidget);

	global_gconfclient	= gconf_client_get_default(); 
	nick			= gconf_client_get_string(global_gconfclient, GCONF"/nick",&err);
	pass			= gconf_client_get_string(global_gconfclient, GCONF"/pass",&err);
	me_msg			= gconf_client_get_string(global_gconfclient, GCONF"/me_msg",&err);
	
	global_speed_unit	= gconf_client_get_int(global_gconfclient, GCONF"/speed_unit",&err);
	global_alt_unit		= gconf_client_get_int(global_gconfclient, GCONF"/alt_unit",&err);
	global_latlon_unit	= gconf_client_get_int(global_gconfclient, GCONF"/latlon_unit",&err);
	
	switch (global_speed_unit)
	{
		case 1:
			widget = lookup_widget(window1, "radiobutton15");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			break;
		case 2:
			widget = lookup_widget(window1, "radiobutton16");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			break;
	}
	
	switch (global_alt_unit)
	{
		case 1:
			widget = lookup_widget(window1, "radiobutton18");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			break;
	}
	
	switch (global_latlon_unit)
	{
		case 1:
			widget = lookup_widget(window1, "radiobutton20");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			break;
		case 2:
			widget = lookup_widget(window1, "radiobutton21");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			break;
	}
	
	widget  = lookup_widget(window1, "entry7");
	if(nick)
		gtk_entry_set_text( GTK_ENTRY(widget), nick );
	widget  = lookup_widget(window1, "entry8");
	if(pass)
		gtk_entry_set_text( GTK_ENTRY(widget), pass );
	widget  = lookup_widget(window1, "entry29");
	if(me_msg)
		gtk_entry_set_text( GTK_ENTRY(widget), me_msg );

	
	global_track_dir	= gconf_client_get_string(global_gconfclient, GCONF"/track_dir",&err);
	if(!global_track_dir)
		global_track_dir = g_strdup_printf("%s/Maps/",global_home_dir);
	
	widget = lookup_widget(window1, "checkbutton2");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), global_auto_download);
	
	gconf_fftimer_running = gconf_client_get_bool(global_gconfclient, GCONF"/fftimer_running",&err);
	global_ffupdate_interval_minutes = gconf_client_get_float(global_gconfclient, GCONF"/ffupdate_interval_minutes",&err);
	
	if(!global_ffupdate_interval_minutes)
		global_ffupdate_interval_minutes = 5;
	
	global_ffupdate_interval = global_ffupdate_interval_minutes * 60000;

	if (global_ffupdate_interval_minutes<5)
		g_sprintf(buffer, "%.1f", global_ffupdate_interval_minutes);
	else
		g_sprintf(buffer, "%.0f", global_ffupdate_interval_minutes);

	widget = lookup_widget(window1, "entry16");
	gtk_entry_set_text( GTK_ENTRY(widget), buffer );
	
	if(gconf_fftimer_running)
	{
		widget = lookup_widget(menu1, "item19");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
	}
		
	

	str = gconf_client_get_string(global_gconfclient, GCONF"/gpsd_host",&err);
	widget = lookup_widget(window1, "entry3");
	if(str)
		gtk_entry_set_text(GTK_ENTRY(widget), g_strdup(str));
	g_free(str);
	
	str = gconf_client_get_string(global_gconfclient, GCONF"/gpsd_port",&err);
	widget = lookup_widget(window1, "entry4");
	if(str)
		gtk_entry_set_text(GTK_ENTRY(widget), g_strdup(str));
	g_free(str);
	
	if (gconf_client_get_bool(global_gconfclient, GCONF"/tracklog_on", NULL)) {
		widget = lookup_widget(window1, "checkbutton17");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	}
	
	gps_provider_init();
	
	gtk_window_set_icon_from_file(GTK_WINDOW(window1), PACKAGE_PIXMAPS_DIR "/" PACKAGE ".png" ,&err);
	if (err)
	{
		fprintf (stderr, "Failed to load pixbuf file:  %s\n", err->message);
		g_error_free (err);
	}
	
	float lat, lon;
	lat = gconf_client_get_float(global_gconfclient, GCONF"/myposition_lat", NULL);
	lon = gconf_client_get_float(global_gconfclient, GCONF"/myposition_lon", NULL);
	set_myposition(lat, lon);

	gboolean show_pois;
	show_pois = gconf_client_get_bool(global_gconfclient, GCONF"/show_pois", NULL);
	global_poi_cat = gconf_client_get_int(global_gconfclient, GCONF"/poi_cat", NULL);
	widget = lookup_widget(menu1, "item20");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), show_pois);

	float view_lat, view_lon;
	int view_zoom;
	view_lat = gconf_client_get_float(global_gconfclient, GCONF"/view_lat", NULL);
	view_lon = gconf_client_get_float(global_gconfclient, GCONF"/view_lon", NULL);
	view_zoom = gconf_client_get_int(global_gconfclient, GCONF"/view_zoom", NULL);

	osm_gps_map_set_mapcenter(OSM_GPS_MAP(mapwidget), view_lat, view_lon, view_zoom);

	global_autocenter = gconf_client_get_bool(global_gconfclient, GCONF"/autocenter", NULL);
	if (global_autocenter) {
		GtkToggleToolButton * ttbutton;
		ttbutton = GTK_TOGGLE_TOOL_BUTTON(lookup_widget(window1, "button3"));
		gtk_toggle_tool_button_set_active(ttbutton, TRUE);
		ttbutton = GTK_TOGGLE_TOOL_BUTTON(lookup_widget(window1, "button56"));
		gtk_toggle_tool_button_set_active(ttbutton, TRUE);
	}
}

/*
 * quit the application
 */
void
quit()
{
	gboolean success = FALSE;
	GError **error = NULL;

	int zoom;
	float lat, lon;

	g_object_get(G_OBJECT(mapwidget), "zoom", &zoom, NULL);

	osm_gps_map_screen_to_geographic(OSM_GPS_MAP(mapwidget),
			mapwidget->allocation.width/2, mapwidget->allocation.height/2,
			&lat, &lon);

	success = gconf_client_set_float(global_gconfclient, GCONF"/view_lat", lat,	error);
	success = gconf_client_set_float(global_gconfclient, GCONF"/view_lon", lon,	error);
	success = gconf_client_set_int(global_gconfclient, GCONF"/view_zoom", zoom,	error);

	success = gconf_client_set_bool(global_gconfclient, GCONF"/autocenter", global_autocenter, error);

	success = gconf_client_set_bool(global_gconfclient, GCONF"/show_pois", global_show_pois, error);
	success = gconf_client_set_int(global_gconfclient, GCONF"/poi_cat", global_poi_cat, error);

	gtk_main_quit();
}


GSList *
gconf_get_repolist()
{
	GSList	*repo_list;
	
	GSList	*list;
	GError **error = NULL;
	
	printf("* %s()\n", __PRETTY_FUNCTION__);

	// read user defined repositories
	repo_list = gconf_client_get_list(
			global_gconfclient, GCONF"/repos", GCONF_VALUE_STRING, error);
	
	if (repo_list)
	{
		for(list = repo_list; list != NULL; list = list->next)
		{
			gchar **array;
			gchar *str = list->data;
			repo_t *repo = g_new0(repo_t, 1);

			array = g_strsplit(str,"|",4);

			repo->type = REPO_TYPE_FOXTROTGPS;
			repo->name = g_strdup(array[0]);
			repo->uri  = g_strdup(array[1]);
			repo->dir  = g_strdup(array[2]);
			repo->inverted_zoom = (atoi(g_strdup(array[3])) == 1) ? TRUE : FALSE;

			global_repo_list = g_slist_append(global_repo_list, repo);

			printf("GCONF: \n -- name: %s \n -- uri: %s \n -- dir: %s \n",
				repo->name, repo->uri, repo->dir);
		}
	}
	
	// add OGM map providers
	OsmGpsMapSource_t ogm_sources[] = {
			OSM_GPS_MAP_SOURCE_OPENSTREETMAP,
			OSM_GPS_MAP_SOURCE_OPENSTREETMAP_RENDERER,
			OSM_GPS_MAP_SOURCE_OPENAERIALMAP,
			OSM_GPS_MAP_SOURCE_MAPS_FOR_FREE,
			OSM_GPS_MAP_SOURCE_OPENCYCLEMAP,
			OSM_GPS_MAP_SOURCE_OSM_PUBLIC_TRANSPORT,
			OSM_GPS_MAP_SOURCE_GOOGLE_STREET,
			OSM_GPS_MAP_SOURCE_GOOGLE_SATELLITE,
			OSM_GPS_MAP_SOURCE_GOOGLE_HYBRID,
			OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_STREET,
			OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE,
			OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID,
			OSM_GPS_MAP_SOURCE_YAHOO_STREET,
			OSM_GPS_MAP_SOURCE_YAHOO_SATELLITE,
			OSM_GPS_MAP_SOURCE_YAHOO_HYBRID,
			OSM_GPS_MAP_SOURCE_OSMC_TRAILS
	};
	
	int i;
	for (i = 0; i < (sizeof(ogm_sources)/sizeof(OsmGpsMapSource_t)); i++)
	{
		repo_t *repo = g_new0(repo_t, 1);

		repo->type = REPO_TYPE_OSM_GPS_MAP;
		repo->ogm_source = ogm_sources[i];
		repo->name = g_strdup(osm_gps_map_source_get_friendly_name(ogm_sources[i]));

		printf("*** %d %s\n", i, repo->name);

		global_repo_list = g_slist_append(global_repo_list, repo);
	}
	
	printf("*** global repo list length = %d\n", g_slist_length(global_repo_list));

	return global_repo_list;	
					
}

void
gconf_set_repolist()
{
	
	GSList	*list;
	GSList	*gconf_list = NULL;
	GError **error = NULL;
	gboolean success = FALSE;
	
	printf("* %s()\n", __PRETTY_FUNCTION__);

	for(list = global_repo_list; list != NULL; list = list->next)
	{
		repo_t *repo;
		gchar gconf_str[1024];
		
		repo = list->data;
		
		// skip OGM defined sources in the list
		if (repo->type == REPO_TYPE_OSM_GPS_MAP)
			continue;
		
		g_sprintf(gconf_str,
				"%s|%s|%s|%i",
				repo->name, repo->uri, repo->dir, repo->inverted_zoom);
		
		gconf_list = g_slist_append(gconf_list, g_strdup(gconf_str));

		printf("GCONFSAVE: \n -- name: %s \n -- uri: %s \n -- dir: %s \n -- zoom: %i \n\n %s \n",
			repo->name, repo->uri, repo->dir, repo->inverted_zoom, gconf_str);
	}
	
	success = gconf_client_set_list(	global_gconfclient, 
						GCONF"/repos",
						GCONF_VALUE_STRING,
						gconf_list,
						error);
	
	printf("*** %s(): %i \n",__PRETTY_FUNCTION__, success);

}



void
repoconfig__set_current_list_pointer()
{
	
	
	GSList		*list;
	const gchar	*reponame;
	int unused;
	
	for(list = global_repo_list; list != NULL; list = list->next)
	{
		repo_t *repo = list->data;
		
		reponame = g_strdup(repo->name);
		
		if(	g_strrstr(reponame,global_curr_reponame) != NULL &&
			strlen(reponame) == strlen(global_curr_reponame)	
		)
			global_curr_repo = list;
	}

	// still undefined? Take first in repo list
	if (!global_curr_repo)
	{
		printf("Selecting default repository because no tile repository configured, " \
				"or configured repository not found\n");
		global_curr_repo = global_repo_list;
		repo_t *repo = global_curr_repo->data;
		global_curr_reponame = g_strdup(repo->name);
	}
}

void
repoconfig__create_dropdown()
{
	GtkWidget	*combobox;
	GSList		*list;
	int 		i = 0;
	int		j = 0;
	const gchar	*reponame;
	
	combobox = lookup_widget(window1, "combobox1");

	for(list = global_repo_list; list != NULL; list = list->next)
	{
		repo_t	*repo = list->data;

		if (repo->type == REPO_TYPE_FOXTROTGPS)
		{
			reponame = g_strdup(repo->name);
		}
		else if (repo->type == REPO_TYPE_OSM_GPS_MAP)
		{
			reponame = g_strdup(repo->name);
		}
		else
			continue;
		
		gtk_combo_box_append_text (GTK_COMBO_BOX(combobox), g_strdup(repo->name));
		
		if(	g_strrstr(reponame,global_curr_reponame) != NULL &&
			strlen(reponame) == strlen(global_curr_reponame) )
		{
			j = i;
			global_curr_repo = list;
		}
		i++;
	}

	global_repo_cnt = i;
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), j);
}

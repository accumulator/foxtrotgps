#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <gdk/gdkkeysyms.h>
#include <osm-gps-map.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "globals.h"
#include "converter.h"
#include "gps_functions.h"
#include "friends.h"
#include "init.h"
#include "geo_photos.h"
#include "poi.h"
#include "wp.h"
#include "tracks.h"

#define DRAG_TRESHOLD 10
#define ICON_SELECT_TRESHOLD 15

void do_distance();
void do_pickpoint();

static int	friendfinder_timer = 0;
static gboolean distance_mode = FALSE;
static gboolean pickpoint_mode = FALSE;
static int	pickpoint;
static int	msg_timer = 0;
static gboolean msg_pane_visible=TRUE;
static int maximized = 0;
static int local_x = 0;
static int local_y = 0;
static int drag_started = 0;
static GdkPixmap *pixmap_photo = NULL;
static GdkPixmap *pixmap_photo_big = NULL;

GtkWidget *dialog10 = NULL;


void
set_cursor(int type)
{
	printf("setting cursor to %d\n", type);
	static GdkCursor *cursor_cross = NULL;
	static GdkCursor *cursor_default = NULL;
	
	if(!cursor_cross)
	{
		cursor_cross = gdk_cursor_new(GDK_CROSSHAIR);
		cursor_default = gdk_cursor_new(GDK_LEFT_PTR);
	}

	if(type == GDK_CROSSHAIR)
		gdk_window_set_cursor(window1->window, cursor_cross);
	else
		gdk_window_set_cursor(window1->window, cursor_default);
}

gboolean
on_map_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	printf("* %s()\n", __PRETTY_FUNCTION__);

	mouse_x = (int) event->x;
	mouse_y = (int) event->y;

	return FALSE;
}

gboolean
on_map_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	int item_x, item_y;
	GSList *list;
	GSList *friends_found = NULL;
	GSList *photos_found = NULL;
	GSList *pois_found = NULL;

	printf("* %s()\n", __PRETTY_FUNCTION__);

	// bail out when dragging
	if (abs(mouse_x - (int) event->x) > DRAG_TRESHOLD || abs(mouse_y - (int) event->y) > DRAG_TRESHOLD)
		return FALSE;

	if(global_show_friends)
	{
		for(list = friends_list; list != NULL; list = list->next)
		{
			friend_t *f = list->data;

			osm_gps_map_geographic_to_screen(OSM_GPS_MAP(mapwidget), f->lat, f->lon, &item_x, &item_y);

			if (abs(item_x - mouse_x) < ICON_SELECT_TRESHOLD && abs(item_y - mouse_y) < ICON_SELECT_TRESHOLD)
			{
				friends_found = g_slist_append(friends_found, f);
			}
		}
	}

	if(global_show_photos)
	{
		for(list = photo_list; list != NULL; list = list->next)
		{
			photo_t *p = list->data;

			osm_gps_map_geographic_to_screen(OSM_GPS_MAP(mapwidget), p->lat, p->lon, &item_x, &item_y);

			if (abs(item_x - mouse_x) < ICON_SELECT_TRESHOLD && abs(item_y - mouse_y) < ICON_SELECT_TRESHOLD)
			{
				photos_found = g_slist_append(photos_found, p);
			}
		}
	}

	if (global_show_pois )
	{
		for(list = poi_list; list != NULL; list = list->next)
		{
			poi_t *p = list->data;

			osm_gps_map_geographic_to_screen(OSM_GPS_MAP(mapwidget), p->lat, p->lon, &item_x, &item_y);

			if (abs(item_x - mouse_x) < ICON_SELECT_TRESHOLD && abs(item_y - mouse_y) < ICON_SELECT_TRESHOLD)
			{
				pois_found = g_slist_append(pois_found, p);
			}
		}
	}

	if (!friends_found && !photos_found && !pois_found &&
		!distance_mode && !pickpoint_mode)
	{
		gtk_widget_show(menu1);
		gtk_menu_popup (GTK_MENU(menu1), NULL, NULL, NULL, NULL,
			  event->button, event->time);
	}

	if(distance_mode)
		do_distance();
	else if (pickpoint_mode)
		do_pickpoint();
	else
	{
		// TODO don't show more than one popup if different kinds of objects are found.
		if (friends_found)
			create_friends_window(friends_found);
		if (photos_found)
			process_selected_photos(photos_found);
		if (pois_found)
			create_pois_window(pois_found);
	}

	return FALSE;
}

void
on_button1_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;

	switch (maximized)
	{
	case 0:
		gtk_window_fullscreen(GTK_WINDOW(window1));
		maximized = 1;
		break;
	case 1:
		widget = lookup_widget(window1, "eventbox5");
		gtk_widget_set_visible(widget, FALSE);
		maximized = 2;
		break;
	default:
	case 2:
		widget = lookup_widget(window1, "eventbox5");
		gtk_widget_set_visible(widget, TRUE);
		gtk_window_unfullscreen(GTK_WINDOW(window1));
		maximized = 0;
	}
}

void
on_toolbar_button_zoom_in_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
	osm_gps_map_zoom_in(OSM_GPS_MAP(mapwidget));
}


void
on_toolbar_button_autocenter_clicked   (GtkToggleToolButton *button,
                                        gpointer         user_data)
{
	printf("* %s()\n", __PRETTY_FUNCTION__);

	global_autocenter = gtk_toggle_tool_button_get_active(button);
	
	if (!global_autocenter)
		return;

	if (gpsdata) {
		if (isnan(gpsdata->fix.latitude) == 0 && isnan(gpsdata->fix.longitude)== 0 &&
		    gpsdata->fix.latitude != 0 && gpsdata->fix.longitude != 0)
		{
			osm_gps_map_set_center(OSM_GPS_MAP(mapwidget),
					gpsdata->fix.latitude, gpsdata->fix.longitude);
		}
	}
	else
		printf("Not autocentering map due to missing gps data\n");
}

gboolean
on_window1_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	printf("%s()\n",__PRETTY_FUNCTION__);
	quit();
	return FALSE; 
}


gboolean
on_window1_destroy_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	printf("%s()\n",__PRETTY_FUNCTION__);
	quit();
	return FALSE;
}


void
on_toolbar_button_zoom_out_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
	osm_gps_map_zoom_out(OSM_GPS_MAP(mapwidget));
}

gboolean
on_vscale1_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{	
	int zoom;
	zoom = gtk_range_get_value(GTK_RANGE(widget));
	osm_gps_map_set_zoom(OSM_GPS_MAP(mapwidget), zoom);
	return FALSE;
}

void
on_combobox1_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
	GSList	*list;
	gchar *reponame_combo;
	GError **error = NULL;
	gboolean success = FALSE;
	repo_t *repo;

	printf("* %s()\n", __PRETTY_FUNCTION__);
	
	reponame_combo = gtk_combo_box_get_active_text(combobox);
	global_curr_reponame = g_strdup(reponame_combo);
	
	if(!global_curr_reponame)
	{
		global_curr_reponame = g_strdup("OSM");
		printf("YOUR DISTRIBUTION SUCKS BIGTIME\n");
	}
	
	for(list = global_repo_list; list != NULL; list = list->next)
	{
		gchar	*reponame;

		repo = list->data;
		reponame = g_strdup(repo->name);
		
		if(	g_strrstr(reponame,global_curr_reponame) != NULL &&
			strlen(reponame) == strlen(global_curr_reponame)	
		) 
		{
			global_curr_repo = list;
		}
	}
	
	success = gconf_client_set_string(
					global_gconfclient, 
					GCONF"/repo_name",
					global_curr_reponame,
					error);
	
	global_repo_nr = gtk_combo_box_get_active(combobox);
	
	repo = global_curr_repo->data;
	if (repo->type == REPO_TYPE_OSM_GPS_MAP)
	{
		// make sure the cache base is set up for internal osm-gps-map source
		char *ogm_cache_dir = osm_gps_map_get_default_cache_directory();

		printf("* setting OGM map source, base=%s cache=%s source=%d\n",
				ogm_cache_dir, OSM_GPS_MAP_CACHE_AUTO, repo->ogm_source);

		g_object_set(G_OBJECT(mapwidget), "tile-cache-base", ogm_cache_dir, NULL);
		g_object_set(G_OBJECT(mapwidget), "tile-cache", OSM_GPS_MAP_CACHE_AUTO, NULL);
		// set the source ID
		g_object_set(G_OBJECT(mapwidget), "map-source", repo->ogm_source, NULL);
	}
	else
	{
		printf("* setting FoxtrotGPS map source, base=%s source=%s\n", repo->dir, repo->uri);

		g_object_set(G_OBJECT(mapwidget), "tile-cache-base", repo->dir, NULL);
		g_object_set(G_OBJECT(mapwidget), "tile-cache", "", NULL);
		g_object_set(G_OBJECT(mapwidget), "repo-uri", repo->uri, NULL);
	}
}

void
on_dialog1_close                       (GtkDialog       *dialog,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

}


void
on_dialog1_response                    (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

}


void
on_cancelbutton1_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide(dialog1);

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
}


void
on_okbutton1_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*entry_repo;
	GtkWidget	*combobox;
	GtkWidget	*entry_uri;
	GtkWidget	*entry_dir;
	GtkWidget	*togglebutton;
	const gchar	*reponame, *uri, *dir;
	gboolean	reversed;
	repo_t *repo = g_new0(repo_t, 1);
	
	
	entry_repo = lookup_widget(dialog1, "entry5");
	entry_uri = lookup_widget(dialog1, "entry20");
	entry_dir = lookup_widget(dialog1, "entry21");
	togglebutton = lookup_widget(dialog1, "checkbutton12");
	combobox = lookup_widget(window1, "combobox1");

	reponame = gtk_entry_get_text(GTK_ENTRY(entry_repo));
	uri = gtk_entry_get_text(GTK_ENTRY(entry_uri));
	dir = gtk_entry_get_text(GTK_ENTRY(entry_dir));
	reversed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	
	gtk_combo_box_append_text (GTK_COMBO_BOX(combobox), g_strdup(reponame));
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), global_repo_cnt);
	global_repo_cnt++;
	

	repo->name = g_strdup(reponame);
	repo->dir = g_strdup(dir);
	repo->uri = g_strdup(uri);
	repo->inverted_zoom = reversed;
	
	global_repo_list = g_slist_prepend(global_repo_list, repo);
	global_curr_repo = global_repo_list;
	
	gconf_set_repolist();

	gtk_widget_hide(dialog1);

	
	
	printf("*** %s(): new repo: %s\n",__PRETTY_FUNCTION__, reponame);
	


}


void
on_button7_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*entry_repo;
	GtkWidget	*entry_uri;
	GtkWidget	*entry_dir;
	GtkWidget	*togglebutton;

	dialog1 = glade_xml_get_widget (gladexml, "dialog1");

	entry_repo = lookup_widget(dialog1, "entry5");
	entry_uri = lookup_widget(dialog1, "entry20");
	entry_dir = lookup_widget(dialog1, "entry21");
	togglebutton = lookup_widget(dialog1, "checkbutton12");

	gtk_entry_set_text(GTK_ENTRY(entry_repo), "");
	gtk_entry_set_text(GTK_ENTRY(entry_uri), "");
	gtk_entry_set_text(GTK_ENTRY(entry_dir), "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), FALSE);

	gtk_widget_show(dialog1);
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
}

void
on_button8_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	trip_distance	= 0;
	trip_starttime	= 0;
	trip_time	= 0;
	trip_maxspeed	= 0;
}

void
on_checkbutton2_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean auto_download;
	gboolean success = FALSE;
	GError **error = NULL;	
	
	auto_download = gtk_toggle_button_get_active(togglebutton);
	global_auto_download = auto_download;
	
	success = gconf_client_set_bool(
			global_gconfclient, GCONF"/auto_download", auto_download, error);
	
	g_object_set(G_OBJECT(mapwidget), "auto-download", auto_download, NULL);
}

void
on_button9_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*entry_server, *entry_port;
	const gchar	*server, *port;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	entry_server	= lookup_widget(window1, "entry3");
	entry_port	= lookup_widget(window1, "entry4");
	server	= gtk_entry_get_text(GTK_ENTRY(entry_server));
	port	= gtk_entry_get_text(GTK_ENTRY(entry_port));
	global_server	= g_strdup(server);
	global_port	= g_strdup(port);
	
	gps_provider_init();
}



void
on_button11_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	gboolean success = FALSE;
	GError **error = NULL;
	
	
	if(!global_fftimer_running)
	{
		update_position();
		send_message(NULL);
		friendfinder_timer = g_timeout_add_seconds(global_ffupdate_interval/1000,update_position,NULL);
		msg_timer	   = g_timeout_add_seconds(global_ffupdate_interval/1000,send_message,NULL);

		widget = lookup_widget(window1, "image24");
		gtk_widget_show(widget);
		
		gtk_button_set_label(button, "Stop");
		
		widget = lookup_widget(menu1, "item19");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
		
		global_fftimer_running = TRUE;
	}
	
	
	
	else
	{
		widget = lookup_widget(window1, "image24");
		gtk_widget_hide(widget);
		
		g_source_remove(friendfinder_timer);
		g_source_remove(msg_timer);
		friendfinder_timer = 0;
		msg_timer =0;
		global_fftimer_running = FALSE;
		
		gtk_button_set_label(button, "Share!");
	}

	
	success = gconf_client_set_bool(
		global_gconfclient, 
		GCONF"/fftimer_running",
		global_fftimer_running,
		error);
}

void
on_item4_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *label;
	gchar buffer[512];
	gchar latlon[64];
	static float start_lat=0, start_lon=0;
	static float overall_distance = 0;
	static int start_x=0, start_y=0;
	float lat, lon,lat_deg,lon_deg, bearing=0;
	float distance=0;
	double unit_conv = 1;
	static gchar distunit[3];
	
	osm_gps_map_screen_to_geographic(OSM_GPS_MAP(mapwidget), mouse_x, mouse_y, &lat, &lon);

	printf("screen (x,y) world (lat,lon): %d %d %f %f\n",mouse_x, mouse_y, lat, lon);

	if(!distance_mode)
		overall_distance = 0.0;

	set_cursor(GDK_CROSSHAIR);
		
	switch (global_latlon_unit) 
	{
	case 0:
		g_sprintf(latlon, "%f - %f", lat, lon);
		break;
	case 1:
		g_sprintf(latlon, "%s   %s", 
			  latdeg2latmin(lat),
			  londeg2lonmin(lon));
		break;
	case 2:
		g_sprintf(latlon, "%s   %s", 
			  latdeg2latsec(lat),
			  londeg2lonsec(lon));
	}
	
	if(global_speed_unit==1)
	{
		unit_conv = 1.0/1.609344;
		g_sprintf(distunit, "%s", "m");
	}
	else if(global_speed_unit==2)
	{
		unit_conv = 1.0/1.852;
		g_sprintf(distunit, "%s", "NM");
	}
	else
	{
		g_sprintf(distunit, "%s", "km");
	}
	
	if(distance_mode)
	{
		distance = get_distance(start_lat, start_lon, lat, lon);
		bearing = get_bearing(start_lat, start_lon, lat, lon);
	}
	else if(gpsdata !=NULL && gpsdata->fix.latitude)
	{
		distance = get_distance(gpsdata->fix.latitude, gpsdata->fix.longitude, lat, lon);
	}

	if(distance_mode)
		overall_distance += distance;
	
	
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), latlon, -1);
	
	
	
	label = lookup_widget(window2,"label64");
	
	if(!distance_mode && gpsdata && gpsdata->fix.latitude)
	{
		g_sprintf(buffer,"<b>This point:</b> \n%s \n"
				"<small><i>(coords auto-copied to clipboard)\n</i></small>\n"
				"<b>Bearing:</b>\n%.1f°\n"
				"<b>Distance from your location:</b>\n%.2f%s\n"
				"Click another point for distance",
				latlon, bearing,
				distance*unit_conv, distunit);
	}
	else if (!distance_mode && (!gpsdata || (gpsdata && !gpsdata->fix.latitude)))
	{
		g_sprintf(buffer,"<b>This point:</b> \n%s \n"
				"<small><i>(coords auto-copied to clipboard)\n</i></small>\n" 
				"Click another point for distance",
				latlon);		
	}
	else
	{
		g_sprintf(buffer,"<b>This point:</b> \n%s \n"
				"<small><i>(coords auto-copied to clipboard)\n</i></small>\n"
				"<b>Bearing:</b>\n%.1f°\n"
				"<b>Distance from last point:</b>\n%.2f%s\n"
				"<b>Overall Distance:</b>\n%.2f%s",
				latlon, bearing,
				distance*unit_conv, distunit, 
				overall_distance*unit_conv, distunit);
	}
	
	gtk_label_set_label(GTK_LABEL(label),buffer);
	gtk_widget_show (window2);

	
	
	if(distance_mode)
	{
		/* Original drawing code removed
		 * TODO : draw line between (start_lat,start_lon) and last click location */
	}
	else
	{
		/* Original drawing code removed
		 * TODO : draw indicator at click location */
	}
	
	
	start_x = mouse_x;
	start_y = mouse_y;
	start_lat = lat;
	start_lon = lon;
	
	
	distance_mode = TRUE;

}



void
on_button14_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *window;
	window = lookup_widget(GTK_WIDGET(button), "window2");
	gtk_widget_hide(window);
	
	distance_mode = FALSE;
	set_cursor(GDK_HAND2);
	repaint_all();
}

gboolean
on_item5_activate                      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *label;
	gchar buffer[512];
	float lat, lon;
	
	osm_gps_map_screen_to_geographic(OSM_GPS_MAP(mapwidget), mouse_x, mouse_y, &lat, &lon);
	
	set_myposition(lat, lon);

	label = lookup_widget(window2,"label64");
	
	g_sprintf(buffer,"<b>Manually set position</b>\n\nThis point: \n\n"
			"  <i>%f %f</i> \n\n"
			"will now be used as your location\n"
			"for the friend finder service.",
			lat, lon);
			
	gtk_label_set_label(GTK_LABEL(label),buffer);
	gtk_widget_show (window2);
	
	return FALSE;
}

void
on_button15_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	trip_counter_on = (trip_counter_on) ? FALSE : TRUE;
	
	if(trip_counter_on)
		gtk_button_set_label(button, "Stop");
	else
		gtk_button_set_label(button, "Resume");
}

void
on_entry7_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *nick;
	const gchar *n;
	GError **error = NULL;
	gboolean success = FALSE;
	
	nick  = lookup_widget(window1, "entry7");
	
	n = gtk_entry_get_text(GTK_ENTRY(nick));
		
	success = gconf_client_set_string(
					global_gconfclient, 
					GCONF"/nick",
					n,
					error);
}


void
on_entry8_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *pass;
	const gchar *p;
	GError **error = NULL;
	gboolean success = FALSE;
	
	pass  = lookup_widget(window1, "entry8");
	
	p = gtk_entry_get_text(GTK_ENTRY(pass));
	
	
	success = gconf_client_set_string(
					global_gconfclient, 
					GCONF"/pass",
					p,
					error);

}

void
on_button13_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	register_nick();
}



void
on_button19_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	
	track_log_close();
	track_log_open();
	
}


void
on_button20_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dialog3, *entry;
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	dialog3 = glade_xml_get_widget (gladexml, "dialog3");
	entry = lookup_widget(dialog3, "entry12");
	gtk_entry_set_text(GTK_ENTRY(entry), global_track_dir);
	
	gtk_widget_show(dialog3);
}

void
on_cancelbutton2_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*dialog3;

	dialog3 = lookup_widget(GTK_WIDGET(button), "dialog3");
	
	gtk_widget_hide(dialog3);

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
}


void
on_okbutton2_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dialog3;
	GtkEntry *entry;
	gint mkres;
	GError **error = NULL;
	gboolean success = FALSE;
	int result;

	
	entry = (GtkEntry *)lookup_widget(GTK_WIDGET(button), "entry12");
	global_track_dir = g_strconcat(gtk_entry_get_text(entry),"/",NULL);

	result = strncmp(global_track_dir, "~", 1); 
	
	if(!result)
	{
		char *sub_home, *home_dir;
		
		strsep(&global_track_dir, "~");
		sub_home = g_strdup(strsep(&global_track_dir, "~"));
		home_dir = getenv("HOME");
		
		g_free(global_track_dir);
		global_track_dir = g_strconcat(home_dir, sub_home, NULL);
		
		g_free(sub_home);
	}
	
	
	printf("TRACKDIR: %s - ~ %d\n",global_track_dir, result);
	
	
	mkres = g_mkdir_with_parents(global_track_dir,0700);
	if(mkres==-1) {
		printf("MKDIR ERROR\n\n");
		perror("mkdir........");
	}
	
	success = gconf_client_set_string(
				global_gconfclient, 
				GCONF"/track_dir",
				global_track_dir,
				error);
	
	dialog3 = lookup_widget(GTK_WIDGET(button), "dialog3");
	
	gtk_widget_hide(dialog3);
}



void
on_item6_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


gboolean
on_item7_activate                      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	set_myposition(0,0);
	return FALSE;
}

void
on_item8_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *dialog;
	
	dialog = glade_xml_get_widget (gladexml, "dialog4");
	gtk_widget_show(dialog);
}

void
on_cancelbutton3_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = lookup_widget(GTK_WIDGET(button), "dialog4");
	gtk_widget_hide(dialog);
}

void
on_okbutton3_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	GtkToggleButton *z1, *z2, *z3, *z4, *z5, *z6;
	int zoom_end = 1;
	int zoom;

	dialog = lookup_widget(GTK_WIDGET(button), "dialog4");
	z1 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton2");
	z2 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton3");
	z3 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton4");
	z4 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton5");
	z5 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton6");
	z6 = (GtkToggleButton *)lookup_widget(dialog, "radiobutton7");

	g_object_get(OSM_GPS_MAP(mapwidget), "zoom", &zoom, NULL);

	zoom_end = (gtk_toggle_button_get_active(z1)) ? zoom + 1 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z2)) ? zoom + 2 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z3)) ? zoom + 3 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z4)) ? zoom + 4 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z5)) ? zoom + 5 : zoom_end;
	zoom_end = (gtk_toggle_button_get_active(z6)) ? zoom + 6 : zoom_end;

	coord_t c1,c2;
	osm_gps_map_get_bbox(OSM_GPS_MAP(mapwidget), &c1, &c2);
	printf("*** downloading tiles: bbox=%f,%f-%f,%f zoom=%d-%d\n", c1.rlat, c1.rlon, c2.rlat, c2.rlon, zoom+1, zoom_end);
	osm_gps_map_download_maps(OSM_GPS_MAP(mapwidget), &c1, &c2, zoom+1, zoom_end);
	gtk_widget_hide(dialog);
}

gboolean
on_drawingarea2_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	

	
	if (!pixmap_photo)
	pixmap_photo = gdk_pixmap_new (
			widget->window,
			widget->allocation.width,
			widget->allocation.height,
			-1);

	if(pixmap_photo)
		printf("pixmap_photo NOT NULL");
	else
		printf("aieee: pixmap_photo NULL\n");

	
	
	
	
	
	
	return FALSE;
}


gboolean
on_drawingarea2_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
	printf("** D2: expose event\n");

	gdk_draw_drawable (
		widget->window,
		widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		pixmap_photo,
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);
	
	
	return FALSE;
}



void
process_selected_photos (GSList *photos)
{
	GtkWidget *label, *widget;
	GtkWidget *drawingarea2;
	
	static gulong hd1 = 0, hd2 = 0;

	GSList *list;
	gchar buffer[512];
	gchar *photo_file = NULL;
	gboolean photo_found = FALSE;

	GdkPixbuf *photo = NULL;
	GError	*error = NULL;
	GdkGC *gc;
	
	waypoint_t *wp = g_new0(waypoint_t,1);
	
	gtk_widget_show(window3);

	drawingarea2 = lookup_widget(window3,"drawingarea2");
	label = lookup_widget(window3,"label104");
	
	for(list = photo_list; list != NULL; list = list->next)
	{
		photo_t *p = list->data;
		printf("\n\nPIXEL PHOTOS: %d %d   \n\n",p->screen_x,p->screen_y);
		
		if(abs(p->screen_x - mouse_x) < 15 &&
		   abs(p->screen_y - mouse_y) < 15 &&
		   !photo_found && !photo) 
		{
			printf("FOUND PHOTO X: %d %d %s\n",p->screen_x, mouse_x, p->name);
			
			g_sprintf(buffer, 
				"%s ",
				p->name);
			photo_found = TRUE;
			wp->lat = p->lat;
			wp->lon = p->lon;
			
			photo = gdk_pixbuf_new_from_file_at_size (
							p->filename, 240,-1,
							&error);
			if(!photo)
			{
				printf ("+++++++++++++ FOTO NOT FOUND +++++++++\n");
				g_sprintf(buffer, "File not found");
			}
			else
			{
				photo_file = p->filename;
				
				printf ("+++++++++++++ F*CKING  DRAWINF +++++++++\n");

				gc = gdk_gc_new(pixmap_photo);
				
				gdk_draw_rectangle (
					pixmap_photo,
					drawingarea2->style->white_gc,
					TRUE,
					0, 0,
					drawingarea2->allocation.width,
					drawingarea2->allocation.height);
				
				gdk_draw_pixbuf (
					pixmap_photo,
					gc,
					photo,
					0,0,
					0, 0,
					-1,-1,
					GDK_RGB_DITHER_NONE, 0, 0);
				
				gdk_draw_drawable (
					drawingarea2->window,
					drawingarea2->style->fg_gc[GTK_WIDGET_STATE (drawingarea2)],
					pixmap_photo,
					0,0,
					0,0,
					-1,-1);

				gtk_widget_queue_draw_area (
					drawingarea2, 
					0, 0,
					80,80);

			}


	
		}
	
	}
	
	if(!photo_found)
		g_sprintf(buffer, "No Geo Photo found");
	
	gtk_label_set_text(GTK_LABEL(label),buffer);

	widget = lookup_widget(window3, "button29");
	if(hd1) g_signal_handler_disconnect(G_OBJECT(widget), hd1);
	hd1 = g_signal_connect (	(gpointer) widget, "clicked",
				G_CALLBACK (on_button29_clicked),
				(gpointer) wp);
	
	widget = lookup_widget(window3, "button21");
	if(hd2) g_signal_handler_disconnect(G_OBJECT(widget), hd2);
	hd2 = g_signal_connect (	(gpointer) widget, "clicked",
				G_CALLBACK (on_button21_clicked),
				(gpointer) g_strdup(photo_file));
		
}


void
on_item11_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

void
on_button21_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GladeXML *gladexml;
	GtkWidget *widget;
	GtkWidget *drawingarea;

	gchar buffer[512];
	gchar *photo_file = user_data;

	GdkPixbuf *photo = NULL;
	GError	*error = NULL;
	GdkGC *gc;
	
printf("*** %s(): \n",__PRETTY_FUNCTION__);

	
	gladexml = glade_xml_new (gladefile, "win13_biggeo", GETTEXT_PACKAGE);
	glade_xml_signal_autoconnect (gladexml);
	widget = glade_xml_get_widget (gladexml, "win13_biggeo");
	g_signal_connect_swapped (widget, "destroy",
				  G_CALLBACK (g_object_unref), gladexml);

	gtk_widget_show(widget);
	
	drawingarea = lookup_widget(widget, "drawingarea3");
	
	photo = gdk_pixbuf_new_from_file_at_size (
							photo_file, 640,-1,
							&error);
	if(!photo)
	{
		printf ("+++++++++++++ FOTO NOT FOUND: %s +++++++++\n", photo_file);
		g_sprintf(buffer, "File not found");
	}
	else
	{
		gc = gdk_gc_new(pixmap_photo);
		
		gdk_draw_rectangle (
			pixmap_photo,
			drawingarea->style->white_gc,
			TRUE,
			0, 0,
			drawingarea->allocation.width,
			drawingarea->allocation.height);
		
		gdk_draw_pixbuf (
			pixmap_photo,
			gc,
			photo,
			0,0,
			0, 0,
			-1,-1,
			GDK_RGB_DITHER_NONE, 0, 0);
		
		gdk_draw_drawable (
			drawingarea->window,
			drawingarea->style->fg_gc[GTK_WIDGET_STATE (drawingarea)],
			pixmap_photo,
			0,0,
			0,0,
			-1,-1);

		gtk_widget_queue_draw_area (
			drawingarea, 
			0, 0,
			80,80);

	}


	
	
	gtk_widget_hide(window3);printf("*** %s(): 44\n",__PRETTY_FUNCTION__);

}



gboolean
on_item12_activate                     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	show_window6();
	
	return FALSE;
}


gboolean
on_item14_activate                     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *dialog, *combobox;

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	dialog = glade_xml_get_widget (gladexml, "dialog6");
	gtk_widget_show(dialog);
	
	combobox = lookup_widget(dialog, "combobox4");
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

	return FALSE;
}


void
on_cancelbutton4_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*dialog;

	dialog = lookup_widget(GTK_WIDGET(button), "window6");
	gtk_widget_destroy(dialog);
}


void
on_okbutton4_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dialog;
	dialog = lookup_widget(GTK_WIDGET(button), "window6");
	insert_poi(dialog);
	

	global_show_pois = TRUE; 
	repaint_all();
}

void
on_cancelbutton5_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dialog = lookup_widget(GTK_WIDGET(button), "dialog6");
	gtk_widget_hide(dialog);
}

/* called when pressing OK in POI select dialog */
void
on_okbutton5_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dialog, *combobox;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	dialog = lookup_widget(GTK_WIDGET(button), "dialog6");
	combobox = lookup_widget(GTK_WIDGET(button), "combobox4");
	
	global_poi_cat = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));

	gtk_widget_hide(dialog);

	get_pois();
}

gboolean
on_dialog6_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

  gtk_widget_hide (widget);
  return TRUE;
}


void
on_combobox2_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	on_combobox_cat_changed(combobox);
}

void
on_button22_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *window;
	window = lookup_widget(GTK_WIDGET(button), "window5");
	gtk_widget_destroy(window);
}



void
on_entry16_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;		

	
	global_ffupdate_interval_minutes = atof(gtk_entry_get_text(GTK_ENTRY(editable)));
	global_ffupdate_interval = global_ffupdate_interval_minutes * 60000;
	
	if (global_ffupdate_interval < 30000)
		global_ffupdate_interval = 30000;

	if(global_fftimer_running)
	{
		if(friendfinder_timer) {
			g_source_remove(friendfinder_timer);
			friendfinder_timer = 0;
		}
		
		if(msg_timer) {
			g_source_remove(msg_timer);
			msg_timer = 0;
		}
		
		friendfinder_timer = g_timeout_add_seconds(global_ffupdate_interval/1000, update_position, NULL);
		msg_timer	   = g_timeout_add_seconds(global_ffupdate_interval/1000, send_message, NULL);
	}
	
	
	success = gconf_client_set_float(
			global_gconfclient, 
			GCONF"/ffupdate_interval_minutes",
			global_ffupdate_interval_minutes,
			error);

}


void
on_radiobutton14_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_speed_unit = (gtk_toggle_button_get_active(togglebutton)) ? 0 : global_speed_unit;

	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/speed_unit",
				global_speed_unit,
				error);
}


void
on_radiobutton15_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_speed_unit = (gtk_toggle_button_get_active(togglebutton)) ? 1 : global_speed_unit;
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/speed_unit",
				global_speed_unit,
				error);
}


void
on_radiobutton16_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_speed_unit = (gtk_toggle_button_get_active(togglebutton)) ? 2 : global_speed_unit;
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/speed_unit",
				global_speed_unit,
				error);
}


void
on_radiobutton17_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_alt_unit = (gtk_toggle_button_get_active(togglebutton)) ? 0 : global_alt_unit;
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/alt_unit",
				global_alt_unit,
				error);
}


void
on_radiobutton18_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_alt_unit = (gtk_toggle_button_get_active(togglebutton)) ? 1 : global_alt_unit;

	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/alt_unit",
				global_alt_unit,
				error);
}


void
on_radiobutton19_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_latlon_unit = (gtk_toggle_button_get_active(togglebutton)) ? 0 : global_latlon_unit;
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/latlon_unit",
				global_latlon_unit,
				error);
}


void
on_radiobutton20_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;
	
	global_latlon_unit = (gtk_toggle_button_get_active(togglebutton)) ? 1 : global_latlon_unit;

	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/latlon_unit",
				global_latlon_unit,
				error);
}

void
on_radiobutton21_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean success = FALSE;
	GError **error = NULL;

	global_latlon_unit = (gtk_toggle_button_get_active(togglebutton)) ? 2 : global_latlon_unit;
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/latlon_unit",
				global_latlon_unit,
				error);
}


gboolean
on_button11_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
	return FALSE;
}



void
on_button26_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *window;
	window = lookup_widget(GTK_WIDGET(button), "window8");
	gtk_widget_destroy(window);
}

gboolean
on_window2_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_hide_on_delete       (widget);
	distance_mode = FALSE;
	set_cursor(GDK_HAND2);
	repaint_all();
  return TRUE;
}

void
on_button27_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *window;
	waypoint_t *wp;
	
	printf("* %s()\n", __PRETTY_FUNCTION__);

	wp = user_data;
	set_current_wp(wp->lat, wp->lon);
	
	window = lookup_widget(GTK_WIDGET(button), "window5");
	gtk_widget_destroy(window);
	//printf("hello, world, %f %f\n", wp->lat, wp->lon);
}



gboolean
on_window3_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_hide_on_delete       (widget);
  return TRUE;
}

void
on_item19_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gboolean active;

	active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));

	set_friends_show(active);
	
	if(active && !global_fftimer_running) {
		GtkWidget *widget = NULL;
		
		widget = lookup_widget(window1, "button11");
		gtk_button_clicked(GTK_BUTTON(widget));
	}
}

void
on_item20_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gboolean active;
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
	
	set_pois_show(active);
}

void
on_item9_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gboolean active;
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
	
	set_photos_show(active);
}

void
repaint_all()
{
	printf("* %s() deprecated\n", __PRETTY_FUNCTION__);
}

void
on_button29_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *window;
	waypoint_t *wp;
	
	printf("* %s()\n", __PRETTY_FUNCTION__);

	wp = user_data;
	set_current_wp(wp->lat, wp->lon);

	window = lookup_widget(GTK_WIDGET(button), "window3");
	gtk_widget_hide(window);
}

void
on_button28_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry14, *entry15;

	char buf[64];

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	
	if(gpsdata && gpsdata->fix.latitude !=0)
	{
		entry14 = lookup_widget(GTK_WIDGET(button), "entry14");
		entry15 = lookup_widget(GTK_WIDGET(button), "entry15");
		
		g_sprintf(buf, "%f", gpsdata->fix.latitude);
		gtk_entry_set_text(GTK_ENTRY(entry14), buf);
		g_sprintf(buf, "%f", gpsdata->fix.longitude);
		gtk_entry_set_text(GTK_ENTRY(entry15), buf);
	}
	
}

void
on_button33_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GladeXML *gladexml;

	GtkWidget *widget;
	GtkTextBuffer *tbuffer;
	GtkWidget *window;

	poi_t *p;
	
	p = user_data;
	tbuffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_insert_at_cursor(tbuffer, "p->desc", -1);
	gtk_text_buffer_set_text(tbuffer, "p->desc", -1);
	
	
	
        
	gladexml = glade_xml_new (gladefile, "window10", GETTEXT_PACKAGE);
	glade_xml_signal_autoconnect (gladexml);
	window = glade_xml_get_widget (gladexml, "window10");
	g_signal_connect_swapped (window, "destroy",
				  G_CALLBACK (g_object_unref), gladexml);
	gtk_widget_show(window);
	
	
	
	widget = lookup_widget(window, "entry17");
	gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%f",p->lat));
	
	widget = lookup_widget(window, "entry18");
	gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%f",p->lon));

	widget = lookup_widget(window, "entry19");
	gtk_entry_set_text(GTK_ENTRY(widget), p->keywords);
	
	widget = lookup_widget(window, "textview2");
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(widget), tbuffer);
	gtk_text_buffer_set_text(tbuffer, p->desc, -1);

	widget = lookup_widget(window, "label126");
	
	if (p->idmd5==NULL)
		gtk_label_set_label(GTK_LABEL(widget), "<span foreground='#ff0000'>POI has no ID -> see website for help!</span>");
	else
		gtk_label_set_text(GTK_LABEL(widget), p->idmd5);

	
	widget = lookup_widget(GTK_WIDGET(button), "window5");
	gtk_widget_destroy(widget);
}

void
on_cancelbutton5a_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*dialog;

	dialog = lookup_widget(GTK_WIDGET(button), "window10");
	gtk_widget_destroy(dialog);
}



void
on_okbutton5a_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	widget = lookup_widget(GTK_WIDGET(button), "window10");
	update_poi(widget);
	
	

	
	repaint_all();
}

void
on_button30_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *entry17, *entry18;

	char buf[64];

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	
	if(gpsdata && gpsdata->fix.latitude !=0)
	{
		entry17 = lookup_widget(GTK_WIDGET(button), "entry17");
		entry18 = lookup_widget(GTK_WIDGET(button), "entry18");
		
		g_sprintf(buf, "%f", gpsdata->fix.latitude);
		gtk_entry_set_text(GTK_ENTRY(entry17), buf);
		g_sprintf(buf, "%f", gpsdata->fix.longitude);
		gtk_entry_set_text(GTK_ENTRY(entry18), buf);
	}
	
}

gboolean
on_map_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	GtkComboBox *source_combobox = GTK_COMBO_BOX(lookup_widget(window1, "combobox1"));

	switch(event->keyval)
	{
	case GDK_Page_Up:
	case GDK_KP_Page_Up:
	case 'i':
		on_toolbar_button_zoom_in_clicked(NULL, NULL);
		return TRUE;
	case GDK_Page_Down:
	case GDK_KP_Page_Down:
	case 'o':
		on_toolbar_button_zoom_out_clicked(NULL, NULL);
		return TRUE;
	case GDK_F11:
	case GDK_space:
		on_button1_clicked(GTK_BUTTON(lookup_widget(window1, "button1")), NULL);
		return TRUE;
	case 'a':
		on_toolbar_button_autocenter_clicked(GTK_TOGGLE_TOOL_BUTTON(lookup_widget(window1,"button3")), NULL);
		return TRUE;
	case 'm':
		on_button76_clicked(NULL, NULL);
		return TRUE;
	case 'r':
		on_item23_button_release_event(NULL, NULL, NULL);
		return TRUE;
	case 'p':
		geo_photos_open_dialog_photo_correlate();
		return TRUE;
	case 't':
		tracks_open_tracks_dialog();
		return TRUE;
	case GDK_1:
		gtk_combo_box_set_active(source_combobox, 0);
		return TRUE;
	case GDK_2:
		gtk_combo_box_set_active(source_combobox, 1);
		return TRUE;
	case GDK_3:
		gtk_combo_box_set_active(source_combobox, 2);
		return TRUE;
	case GDK_4:
		gtk_combo_box_set_active(source_combobox, 3);
		return TRUE;
	case GDK_5:
		gtk_combo_box_set_active(source_combobox, 4);
		return TRUE;
	case GDK_6:
		gtk_combo_box_set_active(source_combobox, 5);
		return TRUE;
	case GDK_7:
		gtk_combo_box_set_active(source_combobox, 6);
		return TRUE;
	case GDK_8:
		gtk_combo_box_set_active(source_combobox, 7);
		return TRUE;
	case GDK_9:
		gtk_combo_box_set_active(source_combobox, 8);
		return TRUE;
	case GDK_0:
		gtk_combo_box_set_active(source_combobox, 9);
		return TRUE;
	default:
		return FALSE;
	}
}

void
on_button34_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GladeXML *gladexml;
	GtkWidget *widget, *widget2;
	poi_t *p;

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	p = user_data;
	gladexml = glade_xml_new (gladefile,
				  "dialog7",
				  GETTEXT_PACKAGE);
	glade_xml_signal_autoconnect (gladexml);
	widget = glade_xml_get_widget (gladexml, "dialog7");
	g_signal_connect_swapped (widget, "destroy",
				  G_CALLBACK (g_object_unref), gladexml);
	gtk_widget_show(widget);
	
	widget2 = lookup_widget(widget, "okbutton6");
	g_signal_connect (	(gpointer) widget2, "clicked",
				G_CALLBACK (on_okbutton6_clicked),
				(gpointer) p);
}


gboolean
on_dialog7_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	return FALSE;
}


void
on_cancelbutton6_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	widget = lookup_widget(GTK_WIDGET(button), "dialog7");
	gtk_widget_destroy(widget);
}


void
on_okbutton6_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	poi_t *p;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	p = user_data;
	
	gtk_widget_destroy(p->widget); 
	delete_poi(p);
	
	widget = lookup_widget(GTK_WIDGET(button), "dialog7");
	gtk_widget_destroy(widget);
}

void
on_button35_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	widget = lookup_widget(window1, "vbox48");
	
	if(msg_pane_visible)
	{	
		gtk_widget_hide(widget);
		msg_pane_visible = FALSE;
		gtk_button_set_label(button, "Show Messages");
	}
	else
	{
		gtk_widget_show(widget);
		msg_pane_visible = TRUE;
		gtk_button_set_label(button, "Hide Messages");
	}
}

void
on_entry3_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	const char *entry;
	
	widget = lookup_widget(window1, "entry3");
	entry = gtk_entry_get_text(GTK_ENTRY(widget));
	
	gconf_client_set_string(	global_gconfclient, 
					GCONF"/gpsd_host",
					entry,
					NULL);
}


void
on_entry4_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	const char *entry;
	
	widget = lookup_widget(window1, "entry4");
	entry = gtk_entry_get_text(GTK_ENTRY(widget));
	
	gconf_client_set_string(	global_gconfclient, 
					GCONF"/gpsd_port",
					entry,
					NULL);
}

void
on_button36_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	tracks_open_tracks_dialog();
}

gboolean
on_window12_delete_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	GtkWidget *window, *vbox;
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	window = lookup_widget(widget, "window12");
	gtk_widget_hide(window);

	vbox = lookup_widget(window, "vbox39");		
	gtk_container_foreach (GTK_CONTAINER (vbox),
			       (GtkCallback) gtk_widget_destroy,
			       NULL);

	return TRUE;
}

void
on_button37_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget, *vbox;
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	widget = lookup_widget(GTK_WIDGET(button), "window12");
	gtk_widget_hide(widget);

	vbox = lookup_widget(widget, "vbox39");		
	gtk_container_foreach (GTK_CONTAINER (vbox),
			       (GtkCallback) gtk_widget_destroy,
			       NULL);
}

void
on_button38_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*entry_repo;
	GtkWidget	*entry_uri;
	GtkWidget	*entry_dir;
	GtkWidget	*togglebutton;
	repo_t		*repo;
		
	dialog8 = glade_xml_get_widget (gladexml, "dialog8");
		
	entry_repo = lookup_widget(dialog8, "entry24");
	entry_uri = lookup_widget(dialog8, "entry25");
	entry_dir = lookup_widget(dialog8, "entry26");
	togglebutton = lookup_widget(dialog8, "checkbutton13");

	repo = global_curr_repo->data;
	gtk_entry_set_text( GTK_ENTRY(entry_repo), repo->name );
	gtk_entry_set_text( GTK_ENTRY(entry_uri), repo->uri );
	gtk_entry_set_text( GTK_ENTRY(entry_dir), repo->dir );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(togglebutton), repo->inverted_zoom);


	
	gtk_widget_show(dialog8);
}


void
on_cancelbutton7_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*widget;

	widget = lookup_widget(GTK_WIDGET(button), "dialog8");
	
	gtk_widget_hide(dialog8);
}


void
on_okbutton7_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget	*entry_repo;
	GtkComboBox	*combobox;
	GtkWidget	*entry_uri;
	GtkWidget	*entry_dir;
	GtkWidget	*togglebutton;
	const gchar	*reponame, *uri, *dir;
	gboolean	reversed;
	repo_t 		*repo;
	
	repo = global_curr_repo->data;
	
	entry_repo = lookup_widget(dialog8, "entry24");
	entry_uri = lookup_widget(dialog8, "entry25");
	entry_dir = lookup_widget(dialog8, "entry26");
	togglebutton = lookup_widget(dialog8, "checkbutton13");
	combobox = GTK_COMBO_BOX(lookup_widget(window1, "combobox1"));

	reponame = gtk_entry_get_text(GTK_ENTRY(entry_repo));
	uri = gtk_entry_get_text(GTK_ENTRY(entry_uri));
	dir = gtk_entry_get_text(GTK_ENTRY(entry_dir));
	reversed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
	
	repo->name = g_strdup(reponame);
	repo->dir = g_strdup(dir);
	repo->uri = g_strdup(uri);
	repo->inverted_zoom = reversed;
	
	global_curr_reponame = g_strdup(reponame);
	
	gtk_combo_box_remove_text(combobox, gtk_combo_box_get_active(combobox));
	gtk_combo_box_prepend_text (combobox, g_strdup(repo->name));
	gtk_combo_box_set_active(combobox, 0);

	printf("*** %s(): new repo: %s %s\n",__PRETTY_FUNCTION__, repo->name, global_curr_reponame);
	gconf_set_repolist();

	gtk_widget_hide(dialog8);

}

void
on_entry5_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget	*widget;
	const char	*txt1, *txt2, *txt3;

	
	widget = lookup_widget(GTK_WIDGET(editable), "entry5");
	txt1 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry20");
	txt2 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry21");
	txt3 = gtk_entry_get_text(GTK_ENTRY(widget));

	
	widget = lookup_widget(GTK_WIDGET(editable), "okbutton1");
	
	if(strlen(txt1) && strlen(txt2) && strlen(txt3))
		gtk_widget_set_sensitive (widget, TRUE);
	else
		gtk_widget_set_sensitive (widget, FALSE);
	
}


void
on_entry20_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget	*widget;
	const char	*txt1, *txt2, *txt3;

	
	widget = lookup_widget(GTK_WIDGET(editable), "entry5");
	txt1 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry20");
	txt2 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry21");
	txt3 = gtk_entry_get_text(GTK_ENTRY(widget));

	
	widget = lookup_widget(GTK_WIDGET(editable), "okbutton1");
	
	if(strlen(txt1) && strlen(txt2) && strlen(txt3))
		gtk_widget_set_sensitive (widget, TRUE);
	else
		gtk_widget_set_sensitive (widget, FALSE);
	
}


void
on_entry21_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget	*widget;
	const char	*txt1, *txt2, *txt3;
	const gchar	*dir;
	int result;
	
	dir = gtk_entry_get_text(GTK_ENTRY(editable));

	result = strncmp(dir, "~", 1);
	if(!result)
		gtk_entry_set_text(GTK_ENTRY(editable), g_strdup(getenv("HOME")));


	
	widget = lookup_widget(GTK_WIDGET(editable), "entry5");
	txt1 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry20");
	txt2 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry21");
	txt3 = gtk_entry_get_text(GTK_ENTRY(widget));

	printf("strlen: %d \n", (int)strlen(txt1));
	
	widget = lookup_widget(GTK_WIDGET(editable), "okbutton1");
	
	if(strlen(txt1) && strlen(txt2) && strlen(txt3))
		gtk_widget_set_sensitive (widget, TRUE);
	else
		gtk_widget_set_sensitive (widget, FALSE);
	
}


void
on_checkbutton12_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_entry24_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget	*widget;
	const char	*txt1, *txt2, *txt3;

	
	widget = lookup_widget(GTK_WIDGET(editable), "entry24");
	txt1 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry25");
	txt2 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry26");
	txt3 = gtk_entry_get_text(GTK_ENTRY(widget));

	
	widget = lookup_widget(GTK_WIDGET(editable), "okbutton7");
	
	if(strlen(txt1) && strlen(txt2) && strlen(txt3))
		gtk_widget_set_sensitive (widget, TRUE);
	else
		gtk_widget_set_sensitive (widget, FALSE);
	
}


void
on_entry25_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget	*widget;
	const char	*txt1, *txt2, *txt3;

	
	widget = lookup_widget(GTK_WIDGET(editable), "entry24");
	txt1 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry25");
	txt2 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry26");
	txt3 = gtk_entry_get_text(GTK_ENTRY(widget));

	
	widget = lookup_widget(GTK_WIDGET(editable), "okbutton7");
	
	if(strlen(txt1) && strlen(txt2) && strlen(txt3))
		gtk_widget_set_sensitive (widget, TRUE);
	else
		gtk_widget_set_sensitive (widget, FALSE);
	
}


void
on_entry26_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkWidget	*widget;
	const char	*txt1, *txt2, *txt3;
	const gchar	*dir;
	int result;
	
	dir = gtk_entry_get_text(GTK_ENTRY(editable));

	result = strncmp(dir, "~", 1);
	if(!result)
		gtk_entry_set_text(GTK_ENTRY(editable), g_strdup(getenv("HOME")));


	
	widget = lookup_widget(GTK_WIDGET(editable), "entry24");
	txt1 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry25");
	txt2 = gtk_entry_get_text(GTK_ENTRY(widget));
	widget = lookup_widget(GTK_WIDGET(editable), "entry26");
	txt3 = gtk_entry_get_text(GTK_ENTRY(widget));

	printf("strlen: %d \n", (int)strlen(txt1));
	
	widget = lookup_widget(GTK_WIDGET(editable), "okbutton7");
	
	if(strlen(txt1) && strlen(txt2) && strlen(txt3))
		gtk_widget_set_sensitive (widget, TRUE);
	else
		gtk_widget_set_sensitive (widget, FALSE);
	
}


void
on_checkbutton13_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}

void
on_entry21_activate                    (GtkEntry        *entry,
                                        gpointer         user_data)
{
	printf("%s(): \n",__PRETTY_FUNCTION__);
}

gboolean
on_eventbox1_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	printf("%s(): \n",__PRETTY_FUNCTION__);
	
	GtkWidget	*window;
	
	window = lookup_widget(widget, "drawingarea2");

	
		
	gdk_draw_rectangle (
		pixmap_photo,
		widget->style->white_gc,
		TRUE,
		0, 0,
		widget->allocation.width,
		widget->allocation.height);
					
	gtk_widget_queue_draw_area (
		widget, 
		0,0,widget->allocation.width,widget->allocation.height);
	
	gtk_widget_hide(window3);
	
	return FALSE;
}

gboolean
on_eventbox2_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *window;
	
	window = lookup_widget(widget, "win13_biggeo");
	
	gtk_widget_destroy(window);
	
  return FALSE;
}

gboolean
on_drawingarea3_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	

	
	if (!pixmap_photo_big)
	pixmap_photo = gdk_pixmap_new (
			widget->window,
			widget->allocation.width,
			widget->allocation.height,
			-1);

	if(pixmap_photo_big)
		printf("pixmap_photo NOT NULL");
	else
		printf("aieee: pixmap_photo NULL\n");

	
	
	
	
	
	
	return FALSE;
}


gboolean
on_drawingarea3_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
	printf("** D2: expose event\n");
	
	gdk_draw_drawable (
		widget->window,
		widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		pixmap_photo,
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);
	
	
	return FALSE;
}

gboolean
on_itemgeocode1_activate               (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	geo_photos_open_dialog_photo_correlate();
	return FALSE;
}


void
on_button39_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	geo_photos_geocode_track_select_dialog(button, user_data);
}


void
on_button40_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	geo_photos_geocode_dir_select_dialog(button, user_data);
}


void
on_cancelbutton8_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	geo_photo_cancel_dialog_photo_correlate();
}


void
on_okbutton8_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	geo_photo_close_dialog_photo_correlate();
}

void
on_button44_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	geo_photos_open_dialog_image_data();
}


void
on_button45_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget = lookup_widget(GTK_WIDGET(button), "dialog_image_data");
	geo_photo_dialog_image_data_next(widget, user_data, GEOPHOTO_PREV);
}


void
on_button46_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget = lookup_widget(GTK_WIDGET(button), "dialog_image_data");
	geo_photo_dialog_image_data_next(widget, user_data, GEOPHOTO_NEXT);
}

void
on_button47_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget = lookup_widget(GTK_WIDGET(button), "dialog_image_data");
	geo_photo_dialog_image_data_next(widget, user_data, GEOPHOTO_FIRST);
}


void
on_button48_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget = lookup_widget(GTK_WIDGET(button), "dialog_image_data");
	geo_photo_dialog_image_data_next(widget, user_data, GEOPHOTO_LAST);
}

void
on_okbutton9_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	printf("%s \n", __PRETTY_FUNCTION__);
	geo_photo_close_dialog_image_data();
}


void
on_button49_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	geo_photo_cancel_dialog_image_data();
}


gboolean
on_eventbox3_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	
	
	return FALSE;
}

void
on_checkbutton14_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	geo_photo_set_add_to_database(togglebutton);
}


void
on_combobox7_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
	geo_photo_set_timezone(combobox);
}


gboolean
on_dialog_geocode_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	geo_photo_cancel_dialog_photo_correlate();
	return TRUE;
}


gboolean
on_dialog_image_data_delete_event      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
printf("I am deleted\n");
	geo_photo_close_dialog_image_data();

  return TRUE;
}

gboolean
on_eventbox4_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *window;
	window = lookup_widget(widget, "window2");
	gtk_widget_hide(window);
	distance_mode = FALSE;
	set_cursor(GDK_HAND2);
	repaint_all();
	
	return FALSE;
}

void
on_entry28_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	geo_photo_correction_entry_cb(editable);
}


void
on_button50_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *window = lookup_widget(GTK_WIDGET(button), "dialog_image_data");
	geo_photo_dialog_image_data_next(window, user_data, GEOPHOTO_FULLSIZE);	
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

void
on_closebutton2_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget = lookup_widget(GTK_WIDGET(button), "dialog_geocode_result");	
	gtk_widget_hide(widget);
}

void
on_radiobutton27_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	global_ff_mode = FUN_MODE;
}


void
on_radiobutton28_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	global_ff_mode = FRIEND_MODE;
}


void
on_radiobutton29_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	global_ff_mode = PRIVATE_MODE;
}


void
on_button51_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	set_cursor(GDK_CROSSHAIR);
	
	distance_mode = TRUE;
}


void
on_button52_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{

}

void
do_distance()
{
	printf("distance_mode\n");
	on_item4_activate(NULL, NULL);
}

gboolean
on_item17_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	float lat, lon;

	osm_gps_map_screen_to_geographic(OSM_GPS_MAP(mapwidget),
			mouse_x, mouse_y,
			&lat, &lon);
	
	set_current_wp(lat, lon);
	
    return FALSE;
}

gboolean
on_item18_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	set_current_wp(0,0);
	return FALSE;
}

void
on_button57_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *combobox;
	
	combobox = lookup_widget(window1, "combobox1");

	if(global_repo_cnt-1 > global_repo_nr)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), global_repo_nr+1);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
		
}


void
on_button58_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *combobox;
	
	combobox = lookup_widget(window1, "combobox1");

	if (global_repo_nr > 0)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), global_repo_nr-1);
	}
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), global_repo_cnt-1);
}

void
on_entry30_activate                    (GtkEntry        *entry,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	printf("* %s()\n",__PRETTY_FUNCTION__);

	widget = lookup_widget(GTK_WIDGET(entry), "okbutton10");
	gtk_button_clicked(GTK_BUTTON(widget));
}


void
on_cancelbutton9_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
			
	widget = lookup_widget(GTK_WIDGET(button), "dialog9");
	gtk_widget_destroy(widget);
}

gboolean
on_eventbox5_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	printf("* %s()\n",__PRETTY_FUNCTION__);
	if(global_new_msg) 
	{
		GtkWidget *widget;
		
		if(!global_infopane_visible)
		{
			widget = lookup_widget(window1, "button76");
			gtk_button_clicked(GTK_BUTTON(widget));
		}
		
		gtk_widget_hide(global_infopane_current->data);
	
		gtk_widget_show(g_list_nth_data(global_infopane_widgets, FRIENDS_PAGE));
		global_infopane_current = g_list_nth(global_infopane_widgets, FRIENDS_PAGE);
		
		widget = lookup_widget(window1, "button35");
		
		if(!msg_pane_visible)
			gtk_button_clicked(GTK_BUTTON(widget));
		
		global_new_msg = FALSE;
	}
	return FALSE;
}


gboolean
on_item23_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *label, *button, *entry, *cbox;
	
	if (!dialog10)
		dialog10 = glade_xml_get_widget (gladexml, "dialog10");

	gtk_widget_show(dialog10);
	
	label = lookup_widget(dialog10, "label190");
	gtk_label_set_label(GTK_LABEL(label),"");

	button = lookup_widget(dialog10, "okbutton11");
	gtk_widget_set_sensitive(button, TRUE);
	
	cbox = lookup_widget(GTK_WIDGET(button), "combobox8");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);	
	
	if (gpsdata && gpsdata->fix.latitude !=0)
	{
		button = lookup_widget(dialog10, "button61");
		gtk_widget_set_sensitive(button, TRUE);
	}

	return FALSE;
}


void
on_button59_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	
	widget = lookup_widget(GTK_WIDGET(button), "dialog10");
	
	gtk_widget_hide(widget);
	
	pickpoint_mode = TRUE;
	pickpoint = 1;
}


void
on_button60_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{		
	gtk_widget_hide(dialog10);
	
	pickpoint_mode = TRUE;
	pickpoint = 2;
}


void
on_button61_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget = NULL;
	
	widget = lookup_widget(dialog10, "entry31");
	
	gtk_entry_set_text(GTK_ENTRY(widget), 
		g_strdup_printf("%f,%f",gpsdata->fix.latitude,gpsdata->fix.longitude));
}


void
on_cancelbutton10_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	
	widget = lookup_widget(GTK_WIDGET(button), "dialog10");
	gtk_widget_hide(widget);
}


void
on_okbutton11_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	char *start=NULL, *end=NULL;
	int service;
	
	gtk_widget_set_sensitive(GTK_WIDGET(button),FALSE);
	
	widget = lookup_widget(GTK_WIDGET(button), "label190");
	
	gtk_label_set_label(GTK_LABEL(widget),"<b><i>Connecting...</i></b>");
	
	widget = lookup_widget(GTK_WIDGET(button), "entry31");
	start = g_strdup( gtk_entry_get_text(GTK_ENTRY(widget)) );
	
	widget = lookup_widget(GTK_WIDGET(button), "entry32");
	end   = g_strdup( gtk_entry_get_text(GTK_ENTRY(widget)) );
	
	widget = lookup_widget(GTK_WIDGET(button), "combobox8");
	service = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	
	fetch_track(dialog10, service, start, end);
}

void
do_pickpoint()
{
	GtkWidget *widget = NULL;
	float lat=0, lon=0;
	printf("%s():\n",__PRETTY_FUNCTION__);

	osm_gps_map_screen_to_geographic(OSM_GPS_MAP(mapwidget), mouse_x, mouse_y, &lat, &lon);
	
	if (pickpoint == 1)
		widget = lookup_widget(dialog10, "entry31");
	if (pickpoint == 2)
		widget = lookup_widget(dialog10, "entry32");
	
	gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%f,%f",lat,lon));
	
	if (pickpoint == 2)
		gtk_widget_grab_focus(widget);
	
	gtk_widget_show(dialog10);

	pickpoint_mode = FALSE;
}

gboolean
on_dialog10_delete_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	
	gtk_widget_hide_on_delete(widget);
	return TRUE;
}

void
on_entry32_activate                    (GtkEntry        *entry,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	
	printf("ACTIVATED\n");
	
	widget = lookup_widget(dialog10, "okbutton11");
	gtk_button_clicked(GTK_BUTTON(widget));
}

void
on_combobox8_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
	
	
	
}


void
on_button76_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	

	GtkWidget *widget, *widget1, *toolbar;

	widget  = lookup_widget(window1, "vbox53");  
	widget1 = lookup_widget(window1, "hbox52");

	if(!global_infopane_visible)
	{
		if(window1->allocation.width <= 480)
		{
			gtk_widget_hide(widget1);
			gtk_widget_set_size_request(widget, window1->allocation.width, -1);
		}
		else
			gtk_widget_set_size_request(widget, 360, -1);
		
		gtk_widget_show(widget);
		
		if(!global_landscape) {
			toolbar = lookup_widget(window1, "toolbar1");
			gtk_widget_hide(toolbar);
		}
		
		if(!global_infopane_current) {
			gtk_widget_show((GtkWidget *) global_infopane_widgets->data);
			global_infopane_current = global_infopane_widgets;
		}

		global_infopane_visible = TRUE;
	}
	else {
		gtk_widget_hide(widget);
		gtk_widget_show(widget1);
		gtk_widget_grab_focus(mapwidget);

		if(!global_landscape) {
			toolbar = lookup_widget(window1, "toolbar1");
			gtk_widget_show(toolbar);
		}
		
		global_infopane_visible = FALSE;		
	}
}


void
on_button69_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_hide(global_infopane_current->data);
	
	if(global_infopane_current->prev) {
		gtk_widget_show(global_infopane_current->prev->data);
		global_infopane_current = global_infopane_current->prev;
	}
	else {
		gtk_widget_show(g_list_last(global_infopane_widgets)->data);
		global_infopane_current = g_list_last(global_infopane_widgets);
	}

}




void
on_button70_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{

	gtk_widget_hide(global_infopane_current->data);
	
	if(global_infopane_current->next) {
		gtk_widget_show(global_infopane_current->next->data);
		global_infopane_current = global_infopane_current->next;
	}
	else {
		gtk_widget_show(global_infopane_widgets->data);
		global_infopane_current = global_infopane_widgets;
	}
		
}

void
on_checkbutton15_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *widget = NULL;
	gboolean active;
	
	active = gtk_toggle_button_get_active(togglebutton);

	widget = lookup_widget(window1, "entry8");
	gtk_entry_set_visibility (GTK_ENTRY (widget), active);

}

void
on_button78_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_checkbutton16_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	trip_livelog_on = gtk_toggle_button_get_active(togglebutton);
}

void
on_checkbutton17_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	trip_logger_on = gtk_toggle_button_get_active(togglebutton);

	
	if(trip_logger_on)
	{
		track_log_open();
		gconf_client_set_bool(
				global_gconfclient, 
				GCONF"/tracklog_on",
				TRUE, NULL);
	}
	else
	{
		track_log_close();
		gconf_client_set_bool(
				global_gconfclient, 
				GCONF"/tracklog_on",
				FALSE, NULL);
	}
}


void
on_button79_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{

}

void
on_entry29_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
	const char *me_msg;

	me_msg = gtk_entry_get_text(GTK_ENTRY(editable));
	gconf_client_set_string(global_gconfclient, GCONF"/me_msg", me_msg, NULL);
}

void
set_map_detail_menuitem_sensitivity (GtkMenuItem *zoomout, GtkMenuItem *menu)
{
	gtk_widget_set_sensitive (GTK_WIDGET (zoomout),
				  global_detail_zoom != 0);
}

void
activate_more_map_details (GtkMenuItem *menu_item, gpointer user_data)
{
	GError *error = NULL;
	gboolean success = FALSE;

	printf ("shrink details\n");

	if (global_detail_zoom > 0) {
		global_detail_zoom--;

	}

	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_detail_zoom",
				global_detail_zoom,
				&error);

	repaint_all ();
}

void
activate_larger_map_details (GtkMenuItem *larger_item, GtkMenuItem *more_item)
{
	GError *error = NULL;
	gboolean success = FALSE;

	printf ("enlarge details\n");

	global_detail_zoom++;

	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_detail_zoom",
				global_detail_zoom,
				&error);

	repaint_all ();
}

void
on_tiles_queued_changed (GtkWidget *widget, GParamSpec *paramSpec)
{
	gint tiles_queued;
	g_object_get(G_OBJECT(widget), "tiles-queued", &tiles_queued, NULL);

	global_tiles_in_dl_queue = tiles_queued;

	printf("* TILES QUEUED: %d\n", tiles_queued);
}

void
on_zoom_changed (GtkWidget *widget, GParamSpec *paramSpec)
{
	gint zoom;
	g_object_get(G_OBJECT(widget), "zoom", &zoom, NULL);

	GtkWidget *vscale = lookup_widget(window1, "vscale1");
	gtk_range_set_value(GTK_RANGE(vscale), (double)zoom);
}

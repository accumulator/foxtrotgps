
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <osm-gps-map.h>

#include "globals.h"
#include "tracks.h"
#include "interface.h"
#include "support.h"
#include "converter.h"
#include "util.h"

GSList *loaded_track = NULL;
GSList *loaded_track_ogm = NULL;

GtkWidget *window12;
GtkWidget *dialog10;

FILE *fp = NULL;


GtkWidget *	make_file_label(const char *file, char *full_file);

bbox_t		get_track_bbox(GSList *track);

GSList *	load_log_file_into_list(char *file);
GSList *	load_gpx_file_into_list(char *file);
GSList *	parse_nodes(xmlNode *node);

void *		fetch_track_thread(void *ptr);


void
track_log()
{
	gchar buffer[256];
	gchar data[256];
	time_t time_sec;
	struct tm *ts;
	
	if(gpsdata->valid)
	{
		time_sec = (time_t)gpsdata->fix.time;
		ts = localtime(&time_sec);
		
		strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", ts);
		
		sprintf(data, "%f,%f,%.1f,%.1f,%.1f,%.1f,%s\n",
				gpsdata->fix.latitude,
				gpsdata->fix.longitude,
				gpsdata->fix.altitude,
				gpsdata->fix.speed,
				gpsdata->fix.heading,
				gpsdata->hdop,
				buffer);
		
		if (fp) fprintf(fp, "%s", data);
	}
}

void
track_log_open()
{
	time_t time_epoch_sec;
	struct tm  *tm_struct;
	gchar buffer[256];
	gchar *filename = NULL;
	GtkLabel *label76;
	gchar *labeltext;
	
	label76 = GTK_LABEL(lookup_widget(window1, "label76"));
	
	time_epoch_sec = time(NULL);
	tm_struct = localtime(&time_epoch_sec);

	strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S.log", tm_struct);
	
	filename = g_strconcat(global_track_dir, buffer,NULL);
	
	printf("*** %s(): %s\n",__PRETTY_FUNCTION__,filename);
	
	if(fp==NULL && trip_logger_on)
	{
		fp = fopen(filename,"w");
		if(!fp)
		{
			printf("oops: %s \n",strerror(errno));
			perror("Triplog open failed: ");
			gtk_label_set_label(label76,"<span foreground='#ff0000'>Error opening logfile</span>");
		}
		else 
		{
			labeltext = g_strconcat("<b><span foreground='#0000ff'>Log: ",buffer,"</span></b>",NULL);
			gtk_label_set_label(label76,labeltext);
			g_free(labeltext);	
		}
	}
	
	g_free(filename);
}

void
track_log_close()
{
	int ret;
	GtkLabel *label76;
	label76 = GTK_LABEL(lookup_widget(window1, "label76"));
	gtk_label_set_label(label76,"");
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	if(fp) {
		printf("closing FP\n");
		ret = fclose(fp);
		fp = NULL;
	
		if(ret) printf("ERROR closing file\n");
	}
}

void
tracks_open_tracks_dialog()
{
	GDir *dir;
	GError *err = NULL;
	const char *file;
	gboolean isfile;
	GList *list = NULL;

	GtkWidget *label, *vbox, *eventbox;

	dir = g_dir_open(global_track_dir, 0, &err);
	
	if (err != NULL)
	{
		fprintf (stderr, "Problem opening directory: %s\n", err->message);
		g_error_free (err);
		return;
	}
	
	window12 = glade_xml_get_widget(gladexml, "window12");
	gtk_widget_show(window12);
	
	vbox = lookup_widget(window12, "vbox39");		
	
	file = g_dir_read_name(dir);

	while (file)
	{
		isfile = g_file_test(g_strconcat(global_track_dir,file,NULL), G_FILE_TEST_IS_REGULAR);
		
		if(isfile)
			list = g_list_insert_sorted( list, g_strdup(file), (GCompareFunc) g_strcmp0 );

		file = g_dir_read_name(dir);
	}

	for(; list!=NULL; list=list->next)
	{
		char *file = list->data;
		
		printf("%s \n", file);
		eventbox = gtk_event_box_new ();
		gtk_widget_set_events (eventbox, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK);
		gtk_widget_show (eventbox);

		gtk_box_pack_start (GTK_BOX (vbox), eventbox, FALSE, FALSE, 0);
		
		label = make_file_label(file, g_strconcat(global_track_dir,file,NULL));
		gtk_widget_show(label);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
		gtk_misc_set_padding (GTK_MISC (label), 3, 2);

		gtk_container_add (GTK_CONTAINER (eventbox), label);
		
		g_signal_connect ((gpointer) eventbox, "button_release_event",
				G_CALLBACK (tracks_on_file_button_release_event),
				g_strconcat(global_track_dir,file,NULL));
	}
	
	g_list_free(list);
	g_dir_close(dir);
}


GtkWidget *
make_file_label(const char *file, char *full_file)
{
	GtkWidget *label;
	
	label = gtk_label_new (g_strdup(file));
	
	return label;
}

/*
 * Convert the foxtrotgps track to OsmGpsMap track
 */
GSList *
track_convert_to_ogm(GSList *foxtrot_track)
{
	GSList *ogm_track = NULL;
	GSList *list;

	for(list = foxtrot_track; list != NULL; list = list->next)
	{
		trackpoint_t *tp = list->data;

		coord_t *coord = g_new0(coord_t,1);

		coord->rlat = tp->lat;
		coord->rlon = tp->lon;

		printf("*** trackpoint (%f,%f)\n", tp->lat, tp->lon);

		ogm_track = g_slist_append(ogm_track, coord);
	}

	return ogm_track;
}

gboolean
load_track(char *file)
{
	GSList * old_track_ogm = NULL;

	if (loaded_track)
	{
		g_slist_free(loaded_track);
		loaded_track = NULL;
	}

	if (loaded_track_ogm)
	{
		old_track_ogm = loaded_track_ogm;
		g_slist_free(loaded_track_ogm);
		loaded_track_ogm = NULL;
	}

	if (g_strrstr(file,".gpx") ||
	    g_strrstr(file,".GPX") ||
	    g_strrstr(file,".Gpx") )
	{
		loaded_track = load_gpx_file_into_list(file);
	}
	else
	{
		loaded_track = load_log_file_into_list(file);
	}

	if (loaded_track)
	{
		loaded_track_ogm = track_convert_to_ogm(loaded_track);
	}

	if (loaded_track_ogm || old_track_ogm)
	{
		osm_gps_map_replace_track(OSM_GPS_MAP(mapwidget), old_track_ogm, loaded_track_ogm);
	}

	return loaded_track ? TRUE : FALSE;
}


gboolean
tracks_on_file_button_release_event   (	GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *range, *vbox;
	int track_zoom, width, height;
	char *file;
	bbox_t bbox;
	
	width  = mapwidget->allocation.width;
	height = mapwidget->allocation.height;
	
	file = (char *) user_data;
	
	gtk_widget_hide(window12);

	vbox = lookup_widget(window12, "vbox39");		
	gtk_container_foreach (GTK_CONTAINER (vbox),
			       (GtkCallback) gtk_widget_destroy,
			       NULL);

	load_track(file);
	
	if (loaded_track) {
		bbox = get_track_bbox(loaded_track);

		track_zoom = get_zoom_covering(width, height, bbox.lat1, bbox.lon1, bbox.lat2, bbox.lon2);

		osm_gps_map_set_mapcenter(OSM_GPS_MAP(mapwidget),
			RAD2DEG((bbox.lat1+bbox.lat2)/2), RAD2DEG((bbox.lon1+bbox.lon2)/2), track_zoom);

		range = lookup_widget(window1, "vscale1");
		gtk_range_set_value(GTK_RANGE(range), (double) track_zoom);
	}
	
	return FALSE;	
}



bbox_t
get_track_bbox(GSList *track)
{
	GSList *list;
	bbox_t bbox;
	double lat, lon;
	
	bbox.lat1 =  -90;
	bbox.lon1 =  180;
	bbox.lat2 =   90;
	bbox.lon2 = -180;
	
	
	for(list = track; list != NULL; list = list->next)
	{
		trackpoint_t *tp = list->data;
	
		lat = tp->lat;
		lon = tp->lon;
		bbox.lat1 = (lat > bbox.lat1) ? lat : bbox.lat1;
		bbox.lat2 = (lat < bbox.lat2) ? lat : bbox.lat2;
		bbox.lon1 = (lon < bbox.lon1) ? lon : bbox.lon1;
		bbox.lon2 = (lon > bbox.lon2) ? lon : bbox.lon2;
	}
	
	return bbox;
}


GSList *
load_log_file_into_list(char *file)
{
	GSList *list = NULL;
	float lat, lon;
	char line[121];
	char **arr;
	FILE *fd;

	fd = fopen(file, "r");
	while(fgets(line,120, fd))
	{
		trackpoint_t *tp = g_new0(trackpoint_t,1);
		
		arr = g_strsplit(line, ",", 2);
		if (arr[0] == NULL || arr[1] == NULL)
			continue;
		
		lat = atof(arr[0]);
		lon = atof(arr[1]);
		
		tp->lat = DEG2RAD(lat);
		tp->lon = DEG2RAD(lon);

		list = g_slist_append(list, tp);
	}
	
	return list;
}


GSList *
load_gpx_file_into_list(char *file)
{
	GSList *list = NULL;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	
	LIBXML_TEST_VERSION
	
	doc = xmlReadFile(file, NULL, 0);
	
	if (doc == NULL) {
		printf("error: could not parse file %s\n", file);
	}
	
	root_element = xmlDocGetRootElement(doc);
	list = parse_nodes(root_element);
	
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
	return list;
}

GSList *
load_gpx_string_into_list(char *gpx_string)
{
	GSList *list = NULL;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	
	if(!gpx_string) return NULL;
	
	LIBXML_TEST_VERSION
	
	doc = xmlReadMemory(gpx_string, strlen(gpx_string), "noname.xml", NULL, 0);
	
	if (doc == NULL) {
		fprintf(stderr, "Failed to parse document\n");
	}
	
	root_element = xmlDocGetRootElement(doc);
	list = parse_nodes(root_element);

	xmlFreeDoc(doc);	
	xmlCleanupParser();
	
	return list;
}


GSList *
parse_nodes(xmlNode *node)
{
	xmlNode *cur_node = NULL;
	GSList *list = NULL;


	for (cur_node = node; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)			
		{
			if (xmlStrEqual(cur_node->name, BAD_CAST "trkpt"))
			{
				double lat, lon;
				trackpoint_t *tp = g_new0(trackpoint_t,1);
				
				lat = atof((char *)xmlGetProp(cur_node, BAD_CAST "lat"));
				lon = atof((char *)xmlGetProp(cur_node, BAD_CAST "lon"));
				
				tp->lat = deg2rad(lat);
				tp->lon = deg2rad(lon);		
		
				list = g_slist_append(list, tp);
			}
			else
			{
				printf("  OTHER XML ELEMENT: %s \n", cur_node->name);
			}
		}
		list = g_slist_concat(list, parse_nodes(cur_node->children));
	}
	
	return list;
}


void
fetch_track(GtkWidget *widget, int service, char *start, char *end)
{
	char *url;
	
	dialog10 = widget;
	printf("%s(): %s, %s\n",__PRETTY_FUNCTION__, start, end);
	
	url = g_strdup_printf("www.tangogps.org/friends/navtrack.php?service=%d&start=%s&end=%s",service,start,end);
	
	if (!g_thread_create(&fetch_track_thread, (void *)url, FALSE, NULL) != 0)
		g_warning("### can't create route thread\n");

}


void *
fetch_track_thread(void *ptr)
{
	postreply_t *reply = NULL;
	char *url;
	GtkWidget *range;
	int track_zoom, width, height;
	bbox_t bbox;
	
	width  = mapwidget->allocation.width;
	height = mapwidget->allocation.height;
	
	url = ptr;
	
	printf("URL ROUTE %s \n", url);
	
	reply = mycurl__do_http_get(url, NULL); 

	printf("HTTP-GET: size: %d, statuscode %d \n", (int)reply->size, (int)reply->status_code);

	loaded_track = load_gpx_string_into_list(reply->data);
	
	if(loaded_track)
	{
		FILE *fp = NULL;
		time_t time_epoch_sec;
		struct tm  *tm_struct;
		gchar buffer[256];
		gchar *filename = NULL;
		
		
		time_epoch_sec = time(NULL);
		tm_struct = localtime(&time_epoch_sec);
		strftime(buffer, sizeof(buffer), "nav%Y%m%d_%H%M%S.gpx", tm_struct);
		
		filename = g_strconcat(global_track_dir, buffer,NULL);
	
		fp = fopen(filename,"w");
		if(!fp)
		{
			printf("oops: %s \n",strerror(errno));
			perror("navtrack open failed: ");
		}
		else
		{
			fprintf(fp,"%s", reply->data);
			fclose(fp);			
		}
		
		g_free(filename);
		
		
		bbox = get_track_bbox(loaded_track);
		
		track_zoom = get_zoom_covering(width, height, bbox.lat1, bbox.lon1, bbox.lat2, bbox.lon2);
		track_zoom = (track_zoom > 15) ? 15 : track_zoom;
	
		gdk_threads_enter();
		{
			
			if(loaded_track)
				osm_gps_map_set_mapcenter(OSM_GPS_MAP(mapwidget),
					rad2deg((bbox.lat1+bbox.lat2)/2), rad2deg((bbox.lon1+bbox.lon2)/2), track_zoom);
		
			//paint_loaded_track();
			gtk_widget_hide(dialog10);
			
			
			
			range = lookup_widget(window1, "vscale1");
			gtk_range_set_value(GTK_RANGE(range), (double) global_zoom);
		}
		gdk_threads_leave();
	}
	else
	{
		const char *err_msg;
		
		
		
		if(reply->status_code == 200)
			err_msg = g_strdup("<span color='#aa0000'><b>Oops! No Route found</b></span>\nTry with another Start/End");
		
		
		else if(reply->status_code == 203)
			err_msg = g_strdup(reply->data);
		
		
		else if (reply->status_code)
			err_msg = g_strdup("<span color='#aa0000'><b>Duh! A Server Error</b></span>\nMaybe try later again...");
		
		
		else
			err_msg = g_strdup("<span color='#aa0000'><b>Oh! A Network Error</b></span>\nCheck the internet!");
		
		gdk_threads_enter();
		{
			GtkWidget *widget;

			widget = lookup_widget(dialog10, "label190");
			gtk_label_set_label(GTK_LABEL(widget), err_msg);
			
			widget = lookup_widget(dialog10, "okbutton11");
			gtk_widget_set_sensitive(widget, TRUE);			
		}
		gdk_threads_leave();
	
	}
	
	
	return NULL;
}

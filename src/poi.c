
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "support.h"
#include "callbacks.h"

#include "globals.h"
#include "poi.h"
#include "converter.h"
#include "interface.h"
#include "util.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <osm-gps-map.h>

void pois_list_updated(void);

#define POI_DB "poi.db"

#define POI_DB_CREATE  "CREATE TABLE poi (	\
		idmd5 TEXT, lat REAL, lon REAL, visibility REAL, cat REAL, subcat REAL, \
		keywords TEXT, desc TEXT, price_range REAL, extended_open REAL,	\
		creator TEXT, bookmarked REAL, user_rating REAL, rating REAL, user_comment TEXT);"

enum {
	COLUMN_STRING,
	COLUMN_INT,
	COLUMN_BOOLEAN,
	N_COLUMNS
};

static GdkPixbuf *poi_icon = NULL;
static GtkWidget *combobox_subcat = NULL;
static gboolean new_dialog = TRUE;
static int num_pois_added = 0;

GtkListStore *list_store;



static int
sql_cb__poi_get(void *unused, int colc, char **colv, char **col_names)
{
	poi_t *poi = g_new0(poi_t,1);

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	poi->idmd5			= g_strdup(colv[0]);
	poi->lat			= atof(colv[1]);
	poi->lon			= atof(colv[2]);
	poi->visibility		= atoi(colv[3]);
	poi->category		= atoi(colv[4]);
	poi->subcategory	= atoi(colv[5]);
	poi->keywords		= g_strdup(colv[6]);
	poi->desc			= g_strdup(colv[7]);
	poi->price_range	= atoi(colv[8]);
	poi->extended_open	= atoi(colv[9]);
	
	poi_list = g_slist_prepend(poi_list, poi);

	return 0;
}







void
set_pois_show(gboolean show)
{
	GError *error = NULL;

	if (global_show_pois == show)
		return;

	global_show_pois = show;

	if(!poi_icon)
	{
		poi_icon = gdk_pixbuf_new_from_file_at_size (
			PACKAGE_PIXMAPS_DIR "/" PACKAGE "-poi.png", 24,24,
			&error);
		// minimum refcount of one, keeps it allocated, despite pois_list_updated()
		// TODO we need to unref it somewhere too, or does it get automatically unreffed at mainloop exit?
		g_object_ref(poi_icon);
	}

	if (!poi_icon)
	{
		printf("Could not load POI icon\n");
		return;
	}

	if (show && !poi_list)
		get_pois();

	pois_list_updated();
}

GtkListStore *
create_combobox_list_store(char *list)
{
	GtkListStore	*list_store;
	GtkTreeIter	iter;
	int		i=0;
	char		**array;
	
	list_store = gtk_list_store_new (N_COLUMNS,
				   G_TYPE_STRING,
				   G_TYPE_INT,
				   G_TYPE_BOOLEAN);

	array = g_strsplit(list,"|",-1);
	
	while(array[i])
	{		
		
		
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				  COLUMN_STRING, array[i],
				  COLUMN_INT, i,
				  COLUMN_BOOLEAN,  FALSE,
				  -1);
		
		i++;
	}
	
	g_strfreev(array);
	return list_store;
}	



void
on_combobox_subcat_changed (GtkComboBox *combobox, gpointer user_data)
{
	
	
	
	GtkTreeIter iter;
	gchar *str_data;
	gint   int_data;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	gtk_combo_box_get_active_iter(combobox, &iter);
	
	gtk_tree_model_get (GTK_TREE_MODEL(list_store), &iter, 
                          COLUMN_STRING, &str_data,
                          COLUMN_INT, &int_data,
                          -1);
	printf("Entry: (%s,%d)\n", str_data, int_data);
}


void set_combobox_subcat(GtkWidget *widget, int choice)
{
	
	GtkCellRenderer *renderer;
	GtkWidget *vbox;
	char *subcat_lists[15];

	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	subcat_lists[0] = "---"; 
	subcat_lists[1] = "- Please choose -|Hotel|Motel|B&B|Hostel|Camping";
	subcat_lists[2] = "- Please choose -|Bank / Exchange / ATM|Post Office|Real Estate Agency|Travel Agency|Other";
	subcat_lists[3] = "- Please choose -|Parking|Gas Station|Repair Shop|Rental|Sharing|Dealer|Radar - Speed Trap|My Car";
	subcat_lists[4] = "- Please choose -|Cinema|Theatre|Concert Hall|Opera|Casino";
	subcat_lists[5] = "- Please choose -|Pharmacy|Hospital|Doctor";
	subcat_lists[6] = "- Please choose -|Cafe|Pub|Lounge Bar|Club|Dancing|Internet Cafe|Wifi Hot Spot";
	subcat_lists[7] = "- Please choose -|Church|Mosque|Synagoge|Temple|Cemetary";
	subcat_lists[8] = "- Please choose -|Bus|Metro|Tram|Taxi|Train Station|Bike|Port|Airport";
	subcat_lists[9] = "- Please choose -|Local Food|European|Asian|American|African|Pizza|Fast Food|Take Away|Barbeque|Italian|Mexican|Indian|Japanese|French";
	subcat_lists[10] = "- Please choose -|Wifi Hotspot|ATM-Money Dispenser|Post Office/Letter Box|Laundry|Hairdresser|Other";
	subcat_lists[11] = "- Please choose -|Tourist Info|Monument|Museum|Zoo|Viewpoint|Relief Map|Other";
	subcat_lists[12] = "- Please choose -|Supermarket|Shopping Center|Clothes|Shoes|Food|Baker|Butcher|DoItYourself|Other";
	subcat_lists[13] = "- Please choose -|Arena/Stadium|Swimming Pool|Freeclimbing|Ice Skating|Golf|Geo Cache|Other";
	subcat_lists[14] = "- Please choose -|Friend|Other Cool Place";

	vbox = lookup_widget(widget, "vbox28");
	list_store = create_combobox_list_store(subcat_lists[choice]);
	
	
	if((!combobox_subcat || new_dialog ) && choice)
	{
		combobox_subcat = gtk_combo_box_new_with_model (GTK_TREE_MODEL(list_store));
		gtk_widget_show (combobox_subcat);
		gtk_box_pack_start (GTK_BOX (vbox),combobox_subcat, FALSE, TRUE, 0);
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox_subcat), renderer, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox_subcat), renderer, "text", 0, NULL);
		g_signal_connect ((gpointer) combobox_subcat, "changed",
					G_CALLBACK (on_combobox_subcat_changed),
					NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_subcat), 0);
		printf("reorder\n");
		gtk_box_reorder_child(GTK_BOX(vbox),combobox_subcat,6);
		new_dialog = FALSE;
	}
	else if (choice)
	{
		gtk_combo_box_set_model(GTK_COMBO_BOX(combobox_subcat), GTK_TREE_MODEL(list_store));
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_subcat), 0);
	}
}

void
on_combobox_cat_changed (GtkComboBox *combobox)
{
	int choice;
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	choice = gtk_combo_box_get_active(combobox);
	
	set_combobox_subcat(GTK_WIDGET(combobox),choice);
}

void
show_window6()
{
	GladeXML *gladexml;
	GtkWidget *dialog;
	GtkWidget *entry14, *entry15, *combobox2;

	float lat, lon;
	char buf[64];

	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	gladexml = glade_xml_new (gladefile, "window6", GETTEXT_PACKAGE);
	glade_xml_signal_autoconnect (gladexml);
	dialog = glade_xml_get_widget (gladexml, "window6");
	g_signal_connect_swapped (dialog, "destroy",
				  G_CALLBACK (g_object_unref), gladexml);
	gtk_widget_show(dialog);
	new_dialog = TRUE;
	
	osm_gps_map_screen_to_geographic(OSM_GPS_MAP(mapwidget), mouse_x, mouse_y, &lat, &lon);

	entry14 = lookup_widget(dialog, "entry14");
	entry15 = lookup_widget(dialog, "entry15");
	
	g_sprintf(buf, "%f", lat);
	gtk_entry_set_text(GTK_ENTRY(entry14), buf);
	g_sprintf(buf, "%f", lon);
	gtk_entry_set_text(GTK_ENTRY(entry15), buf);
	
	combobox2 = lookup_widget(dialog, "combobox2");
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox2), 0);
}

void
insert_poi(GtkWidget *dialog)
{
	GtkComboBox *combo_box;
	GtkTextView *text_view;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	GtkWidget *entry, *radiobutton;

	const char *keyword_raw;
	char *desc, *desc_raw, *keyword;
	char *sql;
	char *db;
	int visibility, price_range = 0, extended_open;
	int category, subcategory;
	float lat, lon;
	int res;
	double rand1, rand2;

	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	entry = lookup_widget(dialog, "entry14");
	lat = parse_degrees(gtk_entry_get_text(GTK_ENTRY(entry)));
	entry = lookup_widget(dialog, "entry15");
	lon = parse_degrees(gtk_entry_get_text(GTK_ENTRY(entry)));
	
	radiobutton = lookup_widget(dialog, "radiobutton11");
	visibility = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton))) ? 1 : 0;	
	combo_box = GTK_COMBO_BOX(lookup_widget(dialog, "combobox2"));
	category = gtk_combo_box_get_active(combo_box);
	subcategory = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox_subcat));
	entry = lookup_widget(dialog, "entry13");
	keyword_raw = gtk_entry_get_text(GTK_ENTRY(entry));
	keyword = my_strescape(keyword_raw, NULL);
	text_view = GTK_TEXT_VIEW(lookup_widget(dialog, "textview1"));
	buffer = gtk_text_view_get_buffer(text_view);
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	desc_raw = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
	desc = my_strescape(desc_raw, NULL);
	radiobutton = lookup_widget(dialog, "radiobutton8");
	price_range = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton))) ? 1 : price_range;
	radiobutton = lookup_widget(dialog, "radiobutton9");
	price_range = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton))) ? 3 : price_range;
	radiobutton = lookup_widget(dialog, "radiobutton10");
	price_range = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton))) ? 5 : price_range;
	radiobutton = lookup_widget(dialog, "checkbutton10");
	extended_open = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton))) ? 1 : 0;
	
	rand1 = g_random_double_range (100000000,1000000000);
	rand2 = g_random_double_range (100000000,1000000000);
	
	db = g_strconcat(foxtrotgps_dir, "/", POI_DB, NULL);	
	
	sql = g_strdup_printf( 	
			"INSERT INTO poi "
			"(idmd5, lat, lon, visibility, cat, subcat, keywords, desc, price_range, extended_open) "
			"VALUES ('%.0f%.0f',%f,%f,%d,%d,%d,'%s','%s',%d,%d)",
			rand1, rand2, lat, lon, visibility, category, subcategory, 
			keyword, desc, price_range, extended_open);
		  
	printf("Inserting POI : %s\n", sql);

	res = sql_execute(db, sql, NULL);
	
	if(res==1)
	{
		sql_execute(db, POI_DB_CREATE, NULL);
		sql_execute(db, sql, NULL);
	}

	g_free(desc);
	g_free(desc_raw);
	g_free(sql);
	gtk_widget_destroy(dialog);

	// TODO : be smart about adding a single POI, instead of reloading all
	get_pois();
}

void
update_poi(GtkWidget *dialog)
{
	GtkTextView *text_view;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	GtkWidget *entry, *widget; 

	const char *keyword_raw, *idmd5;
	char *desc, *desc_raw, *keyword;
	char *sql;
	char *db;
	float lat, lon;
	int res;

	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	widget = lookup_widget(dialog, "label126");
	idmd5 = gtk_label_get_text(GTK_LABEL(widget));
	
	entry = lookup_widget(dialog, "entry17");
	lat = parse_degrees(gtk_entry_get_text(GTK_ENTRY(entry)));
	entry = lookup_widget(dialog, "entry18");
	lon = parse_degrees(gtk_entry_get_text(GTK_ENTRY(entry)));

	entry = lookup_widget(dialog, "entry19");
	keyword_raw = gtk_entry_get_text(GTK_ENTRY(entry));
	keyword = my_strescape(keyword_raw, NULL);
	text_view = GTK_TEXT_VIEW(lookup_widget(dialog, "textview2"));
	buffer = gtk_text_view_get_buffer(text_view);
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	desc_raw = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
	desc = my_strescape(desc_raw, NULL);

	db = g_strconcat(foxtrotgps_dir, "/", POI_DB, NULL);	

	sql = g_strdup_printf( 	
			"UPDATE poi "
			"SET lat=%f, lon=%f, keywords='%s', desc='%s'"
			"WHERE idmd5='%s'"
			, lat, lon,	keyword, desc,
			idmd5);

	printf("*** Updating POI : %s\n", sql);

	res = sql_execute(db, sql, NULL);
	
	if(res==1)
	{
		sql_execute(db, POI_DB_CREATE, NULL);
		sql_execute(db, sql, NULL);
	}

	g_free(desc);
	g_free(desc_raw);
	g_free(sql);
	
	gtk_widget_destroy(dialog);

	// TODO : be smart about updating a single POI, instead of reloading all
	get_pois();
}

void
delete_poi(poi_t *p)
{
	char *db;
	char *sql;
	int res;

	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	db = g_strconcat(foxtrotgps_dir, "/", POI_DB, NULL);	
	
	sql = g_strdup_printf( 	
			"DELETE FROM poi "
			"WHERE idmd5='%s'",
			p->idmd5
			);

	printf("*** Delete POI : %s\n",sql);

	res = sql_execute(db, sql, NULL);

	// TODO : be smart about deleting a single POI, instead of reloading all
	get_pois();
}

void
get_pois()
{
	get_pois_for_bbox(-90.0, -180.0, 90.0, 180.0);
}

void
get_pois_for_bbox(float lat1, float lon1, float lat2, float lon2)
{
	char sql[256];
	char *db;
		
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	db = g_strconcat(foxtrotgps_dir, "/", POI_DB, NULL);
	
	if(poi_list)
		g_slist_free(poi_list);
	poi_list = NULL;
	
	if (global_poi_cat==0)
	{
		g_snprintf(sql, 256, 
				"SELECT * FROM poi "
				"WHERE lat>%f AND lat<%f AND lon>%f AND lon<%f "
				"LIMIT 1000;",
				lat1, lat2, lon1, lon2
				);
		printf("*** POIs get all categories : %s \n", sql);
	}
	else if (global_poi_cat>=1)
	{
		g_snprintf(sql, 256, 
				"SELECT * FROM poi "
				"WHERE cat=%d AND lat>%f AND lat<%f AND lon>%f AND lon<%f "
				"LIMIT 1000;",
				global_poi_cat,
				lat1, lat2, lon1, lon2
				);		
		printf("*** POIs get cat %d : %s \n", global_poi_cat, sql);
	}

	sql_execute(db, sql, sql_cb__poi_get);

	pois_list_updated();
}

void
create_pois_window(GSList *pois)
{
	GtkWidget *window, *widget;
	GtkWidget *label;
	gchar *buffer = NULL, *buffer2 = NULL;
	float distance=0;
	poi_t *p, *this_poi = NULL;

	waypoint_t *wp = g_new0(waypoint_t, 1);
	
	printf("*** %s(): \n",__PRETTY_FUNCTION__);

	GladeXML *gladexml = glade_xml_new (gladefile,
					    "window5",
					    GETTEXT_PACKAGE);
	glade_xml_signal_autoconnect (gladexml);
	window = glade_xml_get_widget (gladexml, "window5");
	g_signal_connect_swapped (window, "destroy",
				  G_CALLBACK (g_object_unref), gladexml);
	
	if (pois)
	{
		// TODO : foxtrotGPS supports displaying only one POI, so just take the first in the list for now
		p = pois->data;

		printf("FOUND POI: %f %f %s\n",p->lat, p->lon, my_strescape_back(p->keywords,NULL));

		if (gpsdata && gpsdata->valid)
		{
			distance = get_distance(gpsdata->fix.latitude, gpsdata->fix.longitude, p->lat, p->lon);
		}
		else if (global_myposition.lat != 0 || global_myposition.lon != 0)
		{
			distance = get_distance(global_myposition.lat, global_myposition.lon, p->lat, p->lon);
		}

		buffer = g_strdup_printf("<b>%s</b> ", my_strescape_back(p->keywords,NULL));
		buffer2 = g_strdup_printf("%s \n\nDistance: %.3fkm ", my_strescape_back(p->desc,NULL), distance);

		wp->lat = p->lat;
		wp->lon = p->lon;

		this_poi = p;
	}
	
	label = lookup_widget(window,"label110");
	gtk_label_set_label(GTK_LABEL(label), buffer);
	label = lookup_widget(window,"label111");
	gtk_label_set_label(GTK_LABEL(label), buffer2);

	widget = lookup_widget(window, "button27");
	g_signal_connect (	(gpointer) widget, "clicked",
				G_CALLBACK (on_button27_clicked),
				(gpointer) wp);
	
	if (this_poi != NULL)
	{
		this_poi->widget = window;

		widget = lookup_widget(window, "button33");
		g_signal_connect (	(gpointer) widget, "clicked",
					G_CALLBACK (on_button33_clicked),
					(gpointer) this_poi);
		
		widget = lookup_widget(window, "button34");
		g_signal_connect (	(gpointer) widget, "clicked",
					G_CALLBACK (on_button34_clicked),
					(gpointer) this_poi);
	}
	
	gtk_widget_show(window);
}

void
pois_list_updated(void)
{
	GSList * list;

	if (!poi_icon)
		return;

	// OGM IDs the image by pixbuf pointer, but also uses refcounting of the pixbuf,
	// so we can reuse a single pixbuf as long as we remove all of them before
	int i;
	for (i = 0; i < num_pois_added; i++)
	{
		osm_gps_map_remove_image(OSM_GPS_MAP(mapwidget), poi_icon);
	}

	num_pois_added = 0;

	if (global_show_pois)
	{
		for (list = poi_list; list != NULL; list = list->next)
		{
			poi_t *p = list->data;
			osm_gps_map_add_image(OSM_GPS_MAP(mapwidget), p->lat, p->lon, poi_icon);
			num_pois_added++;
		}
	}
}

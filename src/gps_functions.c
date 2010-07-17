

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <osm-gps-map.h>

#include "gps_functions.h"
#include "globals.h"
#include "support.h"
#include "converter.h"
#include "wp.h"
#include "tracks.h"

#define BUFSIZE 512
char * distance2scale(float distance, float *factor);
void * get_gps_thread(void *ptr);
gboolean cb_gps_timer();

gint timer;

void
gps_provider_init()
{
	_gps_provider_init();
	timer = g_timeout_add(1000, cb_gps_timer, NULL);
}

void
gps_provider_close()
{
	_gps_provider_close();
	// clear timer?
}

gboolean
cb_gps_timer()
{
	static float lat_tmp=0, lon_tmp=0;
	float trip_delta=0;

	static double trip_time_accumulated = 0;
	static gboolean trip_counter_got_stopped = FALSE;
	
	if(gpsdata)
	{
		trackpoint_t *tp = g_new0(trackpoint_t,1);
		
		if(gpsdata->seen_valid)
		{
			int hand_wp_x, hand_wp_y;
			double bearing;
			
			osm_gps_map_draw_gps(OSM_GPS_MAP(mapwidget),
					gpsdata->fix.latitude, gpsdata->fix.longitude,
					(gpsdata->fix.speed > 0.3 ? gpsdata->fix.heading : OSM_GPS_MAP_INVALID));
			
			if(global_wp_on && gpsdata->valid)
			{
				bearing = get_bearing(gpsdata->fix.latitude, gpsdata->fix.longitude, global_wp.lat, global_wp.lon);
				gpsdata->fix.bearing = bearing;

				hand_wp_x =  25 * sinf(DEG2RAD(bearing));
				hand_wp_y = -25 * cosf(DEG2RAD(bearing));
					
				/* TODO : draw wp bearing */
					
				osd_wp();
			}
		}
		
		if(global_autocenter)
		{
			int map_width = mapwidget->allocation.width;
			int map_height = mapwidget->allocation.height;
			int x, y;
			osm_gps_map_geographic_to_screen(OSM_GPS_MAP(mapwidget),
					gpsdata->fix.latitude, gpsdata->fix.longitude,
					&x, &y);
			if((x < (map_width / 2 - map_width / 8) ||
				x > (map_width / 2 + map_width / 8) ||
				y < (map_height / 2 - map_height / 8) ||
				y > (map_height / 2 + map_height / 8) ) &&
				
				isnan(gpsdata->fix.latitude) ==0 &&
				isnan(gpsdata->fix.longitude)==0 &&
				gpsdata->fix.latitude  !=0 &&
				gpsdata->fix.longitude !=0
				)
			{
				osm_gps_map_set_center(OSM_GPS_MAP(mapwidget),
						gpsdata->fix.latitude, gpsdata->fix.longitude);
			}
		}
		
		if (gpsdata->valid && lat_tmp != 0 && lon_tmp != 0)
		{
			trip_delta = get_distance(gpsdata->fix.latitude, gpsdata->fix.longitude, lat_tmp, lon_tmp);
			
			if(isnan(trip_delta))
				trip_delta = 0;
			
			if(trip_delta > TRIP_DELTA_MIN)
			{
				tp->time    = gpsdata->fix.time;
				tp->lat     = DEG2RAD(gpsdata->fix.latitude);
				tp->lon     = DEG2RAD(gpsdata->fix.longitude);
				tp->lat_deg = gpsdata->fix.latitude;
				tp->lat_deg = gpsdata->fix.longitude;
				tp->alt     = gpsdata->fix.altitude;
				tp->speed   = gpsdata->fix.speed;
				tp->head    = gpsdata->fix.heading;
				tp->hdop    = gpsdata->hdop;
				tp->heart   = 0;

				if (trip_delta > SEGMENT_DISTANCE)
					tp->hdop = 999;

				if (trackpoint_list->length > TRACKPOINT_LIST_MAX_LENGTH)
					g_free(g_queue_pop_head(trackpoint_list));
					
				g_queue_push_tail(trackpoint_list, tp);
			}
		}
			
		if(trip_counter_on)
		{
			trip_distance += trip_delta;

			if(gpsdata->valid && gpsdata->fix.speed > trip_maxspeed)
				trip_maxspeed = gpsdata->fix.speed;
			
			if(trip_time == 0) 
				trip_time_accumulated = 0;
			
			if(trip_counter_got_stopped)
			{
				printf("counter had been stopped \n");
				trip_counter_got_stopped = FALSE;
				trip_time_accumulated = trip_time;
				trip_starttime = 0;
			}
			
			if(trip_starttime == 0 && gpsdata->seen_valid)
			{
				trip_starttime = gpsdata->fix.time;
			}
			
			if(trip_starttime > 0 && gpsdata->seen_valid)
			{
				trip_time = gpsdata->fix.time - trip_starttime + trip_time_accumulated;				
			}
			
			if(trip_time < 0)
			{
				trip_time = 0;
				trip_starttime = 0;
				trip_distance = 0;
				trip_maxspeed = 0;
			}
		}
		else
		{
			printf("trip counter halted\n");
			trip_counter_got_stopped = TRUE;
			lat_tmp = lon_tmp = 0;
		}
		
		set_label();
		
		if(trip_logger_on && gpsdata->valid)
			track_log();
		
		if(gpsdata->valid)
		{	
			lat_tmp = gpsdata->fix.latitude;
			lon_tmp = gpsdata->fix.longitude;
		}
	}
	else 
	{
		set_label_nogps();
	}
	
	return TRUE; 
}

void
osd_speed(gboolean force_redraw)
{
	
	PangoContext		*context = NULL;
	PangoLayout		*layout  = NULL;
	PangoFontDescription	*desc    = NULL;
	
	GdkColor color;
	GdkGC *gc;
	
	gchar *buffer;
	static int x = 10, y = 10;
	static int width = 0, height = 0;

	static double speed_tmp = 0;
	
	double unit_conv = 1;
		
	//printf("* %s() deprecated\n", __PRETTY_FUNCTION__);
	return;

	if (gpsdata)
	{
		switch (global_speed_unit)
		{
			case 0:
				unit_conv = 1.0;
				break;
			case 1 :
				unit_conv = 1.0/1.609344;
				break;
			case 2 :
				unit_conv = 1.0 / 1.852;
				break;		
		}
		

	
		buffer = g_strdup_printf("%.0f", gpsdata->fix.speed*3.6*unit_conv);
		
		
		context = gtk_widget_get_pango_context (map_drawable);
		layout  = pango_layout_new (context);
		desc    = pango_font_description_new();
		
		pango_font_description_set_absolute_size (desc, 80 * PANGO_SCALE);
		pango_layout_set_font_description (layout, desc);
		pango_layout_set_text (layout, buffer, strlen(buffer));
	
	
		gc = gdk_gc_new (map_drawable->window);
	
		color.red = (gpsdata->fix.speed*3.6*unit_conv > 50) ? 0xffff : 0;
		color.green = 0;
		color.blue = 0;
		
		gdk_gc_set_rgb_fg_color (gc, &color);

		
		
		if(speed_tmp != floor(gpsdata->fix.speed*3.6*unit_conv) || force_redraw)
		{
			
			gdk_draw_drawable (
				map_drawable->window,
				map_drawable->style->fg_gc[GTK_WIDGET_STATE (map_drawable)],
				pixmap,
				0,0,
				0, 0,
				width+10,height+10);
		
			
			if(gpsdata->fix.speed>0.01 && gpsdata->valid) 
				gdk_draw_layout(map_drawable->window,
						gc,
						x, y,
						layout);
			
			
			pango_layout_get_pixel_size(layout, &width, &height);
		}
		
		speed_tmp = floor(gpsdata->fix.speed*3.6*unit_conv);
		
		g_free(buffer);
		pango_font_description_free (desc);
		g_object_unref (layout);
		g_object_unref (gc);
	}
}
void
set_label_nogps()
{
	static GtkLabel *label=NULL;
	static gchar buffer[BUFSIZE];
	int num_dl_threads = (global_tiles_in_dl_queue) ? 1 : 0;

	if(label == NULL)
		label   = GTK_LABEL(lookup_widget(window1, "label4"));
	
	if(num_dl_threads && !global_tiles_in_dl_queue)
	{	
		g_snprintf(buffer, BUFSIZE,
			"<b>no GPSD found</b> - <span foreground='#0000ff'><b>D%d</b></span>",
			num_dl_threads);
	}
	else if (num_dl_threads && global_tiles_in_dl_queue)
		g_snprintf(buffer, BUFSIZE,
			"<b>no GPSD found</b> - <span foreground='#0000ff'><b>D%d</b></span> - <b>[%d]</b>",
			num_dl_threads, global_tiles_in_dl_queue);
	else
		g_snprintf(buffer, BUFSIZE, "<b>no GPSD found</b>");
	
	if(global_new_msg)
		g_snprintf(buffer, BUFSIZE, "<span foreground='#ff0000'><b>New Message arrived. Click here.</b></span>");
	
	gtk_label_set_label(label, buffer);

}
void
set_label()
{
	static GtkLabel *label=NULL,   *label31, *label38, *label39;
	static GtkLabel *label41, *label42, *label43, *label45;
	static GtkLabel *label66, *label68, *label70;
	static gchar buffer[BUFSIZE];
	static gchar numdl_buf[64], dl_buf[64], ff_buf[64], tr_buf[64];
	static gchar speedunit[5], distunit[3], altunit[3];
	int trip_hours, trip_minutes, trip_seconds;
	time_t time_sec;
	struct tm  *ts;
	double unit_conv = 1, unit_conv_alt = 1;
	int num_dl_threads = (global_tiles_in_dl_queue) ? 1 : 0;
	
	if(global_speed_unit==1)
	{
		unit_conv = 1.0/1.609344;
		g_sprintf(speedunit, "%s","mph");
		g_sprintf(distunit, "%s", "m");
	}
	else if(global_speed_unit==2)
	{
		unit_conv = 1.0/1.852;
		g_sprintf(speedunit, "%s","kn");
		g_sprintf(distunit, "%s", "NM");
	}
	else
	{
		g_sprintf(speedunit, "%s","km/h");
		g_sprintf(distunit, "%s", "km");
	}
	
	
	if(global_alt_unit==1)
	{
		unit_conv_alt = 1.0/0.3048;
		g_sprintf(altunit, "%s", "ft");
	}
	else
		g_sprintf(altunit, "%s", "m");
	
	if(global_auto_download)
		g_sprintf(dl_buf, "%s", "");
	else
		g_sprintf(dl_buf, "%s", "<span foreground='#ff0000'><b>!</b></span>");
	
	if (global_fftimer_running)
		g_sprintf(ff_buf, "%s", "<span foreground='#00e000'><b>f</b></span>");
	else
		g_sprintf(ff_buf, "%s", "");
	
	if (trip_logger_on)
		g_sprintf(tr_buf, "%s", "<span foreground='#00e000'><b>t</b></span>");
	else
		g_sprintf(tr_buf, "%s", "");
	
	if(label == NULL)
	{
		label   = GTK_LABEL(lookup_widget(window1, "label4"));
		label45 = GTK_LABEL(lookup_widget(window1, "label45"));
		label41 = GTK_LABEL(lookup_widget(window1, "label41"));
		label31 = GTK_LABEL(lookup_widget(window1, "label31"));
		label38 = GTK_LABEL(lookup_widget(window1, "label38"));
		label39 = GTK_LABEL(lookup_widget(window1, "label39"));
		label42 = GTK_LABEL(lookup_widget(window1, "label42"));
		label43 = GTK_LABEL(lookup_widget(window1, "label43"));
		label66 = GTK_LABEL(lookup_widget(window1, "label66"));
		label68 = GTK_LABEL(lookup_widget(window1, "label68"));
		label70 = GTK_LABEL(lookup_widget(window1, "label70"));
	}
	
	if(num_dl_threads && !global_tiles_in_dl_queue)
		g_sprintf(numdl_buf, "<span foreground='#0000ff'><b>D%d</b></span> ", 
			  num_dl_threads);
	else if (num_dl_threads && global_tiles_in_dl_queue)
		g_sprintf(numdl_buf, "<span foreground='#0000ff'><b>D%d</b></span>-%d ", 
			  num_dl_threads, global_tiles_in_dl_queue);
	else
		g_sprintf(numdl_buf, "%s", "");
	

	g_snprintf(buffer, BUFSIZE,
		"%s%s%s%s<b>%4.1f</b>%s "
		"<small>trp </small><b>%.2f</b>%s "
		"<small>alt </small><b>%.0f</b>%s "
		"<small>hdg </small><b>%.0f</b>° "
		"<small></small>%d/%.1f",
		numdl_buf,
		dl_buf,
		tr_buf,
		ff_buf,
		gpsdata->fix.speed * 3.6 * unit_conv,	speedunit,
		trip_distance * unit_conv,		distunit,
		gpsdata->fix.altitude * unit_conv_alt,	altunit,
		gpsdata->fix.heading * unit_conv,
		gpsdata->satellites_used,
		gpsdata->hdop);

	if(global_new_msg)
		g_snprintf(buffer, BUFSIZE, "<span foreground='#ff0000'><b>New Message arrived. Click here.</b></span>");

	gtk_label_set_label(label, buffer);


	if(global_infopane_visible)
	{
		
		
		
		
		
		time_sec = (time_t)gpsdata->fix.time;
		ts = localtime(&time_sec);
		
		
		strftime(buffer, sizeof(buffer), "<big><b>%a %Y-%m-%d %H:%M:%S</b></big>", ts); 
		gtk_label_set_label(label41,buffer);
	
		
		switch (global_latlon_unit)
		{
			case 0:
				g_snprintf(buffer, BUFSIZE, "<big><b>%f - %f</b></big>", gpsdata->fix.latitude, gpsdata->fix.longitude);
				break;
			case 1:
				g_snprintf(buffer, BUFSIZE, "<big><b>%s   %s</b></big>", 
					  latdeg2latmin(gpsdata->fix.latitude),
					  londeg2lonmin(gpsdata->fix.longitude));
				break;
			case 2:
				g_snprintf(buffer, BUFSIZE, "<big><b>%s   %s</b></big>", 
					  latdeg2latsec(gpsdata->fix.latitude),
					  londeg2lonsec(gpsdata->fix.longitude));
		}
		gtk_label_set_label(label31,buffer);
		
		
		g_snprintf(buffer, BUFSIZE, 
			"<b><span foreground='#0000ff'><span font_desc='50'>%.1f</span></span></b> %s", 
			gpsdata->fix.speed*3.6*unit_conv, speedunit);
		gtk_label_set_label(label38,buffer);
	
		
		g_snprintf(buffer, BUFSIZE, "<big><b>%.1f %s</b></big>", gpsdata->fix.altitude * unit_conv_alt, altunit);
		gtk_label_set_label(label39,buffer);
		
		
		g_snprintf(buffer, BUFSIZE, "<big><b>%.1f°</b></big> ", gpsdata->fix.heading);
		gtk_label_set_label(label42,buffer);
		
		
		g_snprintf(buffer, BUFSIZE, "<big><b>%d</b>  <small>HDOP</small><b> %.1f</b></big>", 
				gpsdata->satellites_used, gpsdata->hdop);
		gtk_label_set_label(label43,buffer);
	
		
		
		
	
		
		g_snprintf(buffer, BUFSIZE, "<big><b>%.3f</b></big> <small>%s</small>", trip_distance*unit_conv,distunit);
		gtk_label_set_label(label45,buffer);
	
	
		
		trip_hours   = trip_time / 3600;
		trip_minutes = ((int)trip_time%3600)/60;
		trip_seconds = (int)trip_time % 60;
		
		if (trip_seconds < 10 && trip_minutes < 10)
		{
			g_sprintf(buffer, "<big><b>%d:0%d:0%d</b></big>",trip_hours,trip_minutes,trip_seconds);
		}
		else if (trip_seconds < 10)
			g_sprintf(buffer, "<big><b>%d:%d:0%d</b></big>",trip_hours,trip_minutes,trip_seconds);
		else if (trip_minutes < 10)
			g_sprintf(buffer, "<big><b>%d:0%d:%d</b></big>",trip_hours,trip_minutes,trip_seconds);
		else
			g_sprintf(buffer, "<big><b>%d:%d:%d</b></big>",trip_hours,trip_minutes,trip_seconds);
	
		gtk_label_set_label(label66,buffer);
	
		
		g_sprintf(buffer, "<big><b>%.1f</b></big><small> %s</small>", trip_distance*3600*unit_conv/(trip_time+2.0), speedunit);
		gtk_label_set_label(label68,buffer);
	
		
		g_sprintf(buffer, "<big><b>%.1f</b></big><small> %s</small>", trip_maxspeed*3.6*unit_conv, speedunit);
		gtk_label_set_label(label70,buffer);
	}
}


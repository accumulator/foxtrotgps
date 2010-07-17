#include <glib.h>
#include <gps.h>

#include "provider.h"

gboolean reset_gpsd_io();
void * get_gps_thread(void *ptr);

static GIOChannel *gpsd_io_channel =NULL;
static struct gps_data_t *libgps_gpsdata = NULL;

static guint sid1,  sid3; 
guint watchdog = 0;

extern tangogps_gps_data_t *gpsdata;

extern gchar		*global_server;
extern gchar		*global_port;

void
_gps_provider_close()
{
	if (gpsdata)
		g_free(gpsdata);
	gpsdata = NULL;
	if (watchdog)
		g_source_remove(watchdog);
	watchdog = 0;
	g_source_remove(sid1); 
	g_source_remove(sid3); 
	if (libgps_gpsdata)
		gps_close(libgps_gpsdata);
	libgps_gpsdata = NULL;
}

void
_gps_provider_init()
{
	if (libgps_gpsdata != NULL)
	{
		_gps_provider_close();
	}
	
	watchdog = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT_IDLE,60,reset_gpsd_io,NULL,NULL);
	
	g_thread_create(&get_gps_thread, NULL, FALSE, NULL);
}

gboolean
reset_gpsd_io()
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	_gps_provider_init();
	
	return FALSE;	
}




static gboolean
cb_gpsd_io_error(GIOChannel *src, GIOCondition condition, gpointer data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	_gps_provider_init();

	return FALSE; 
}



static gboolean
cb_gpsd_data(GIOChannel *src, GIOCondition condition, gpointer data)
{
	int ret;

	ret = gps_poll(libgps_gpsdata);
	if (ret == 0)
	{
		gpsdata->satellites_used = libgps_gpsdata->satellites_used;
		gpsdata->hdop = libgps_gpsdata->dop.hdop;
		gpsdata->fix.time = libgps_gpsdata->fix.time;
		if (isnan(gpsdata->fix.time))
		{
			gpsdata->fix.time = (time_t) 0;
		}
		gpsdata->valid = (libgps_gpsdata->status != STATUS_NO_FIX);
		if (gpsdata->valid)
		{
			gpsdata->seen_valid = TRUE;
			gpsdata->fix.latitude = libgps_gpsdata->fix.latitude;
			gpsdata->fix.longitude = libgps_gpsdata->fix.longitude;
			gpsdata->fix.speed = libgps_gpsdata->fix.speed;
			gpsdata->fix.heading = libgps_gpsdata->fix.track;
			gpsdata->fix.altitude = libgps_gpsdata->fix.altitude;
		}
		
		g_source_remove(watchdog);
		watchdog = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT_IDLE,60,reset_gpsd_io,NULL,NULL);
	}
	else
	{
		printf("gps_poll returned %d\n", ret);
	}
	return TRUE;
}

void *
get_gps_thread(void *ptr)
{
	libgps_gpsdata = gps_open(global_server, global_port);
	if (libgps_gpsdata)
	{
		fprintf(stderr, "connection to gpsd SUCCEEDED \n");
		
		if(!gpsdata)
		{
			gpsdata = g_new0(tangogps_gps_data_t,1);
		}
		
	
		gps_stream(libgps_gpsdata, WATCH_ENABLE | POLL_NONBLOCK, NULL);
		
		
		gpsd_io_channel = g_io_channel_unix_new(libgps_gpsdata->gps_fd);
		g_io_channel_set_flags(gpsd_io_channel, G_IO_FLAG_NONBLOCK, NULL);
		
		
		sid1 = g_io_add_watch_full(gpsd_io_channel, G_PRIORITY_HIGH_IDLE+200, G_IO_ERR | G_IO_HUP, cb_gpsd_io_error, NULL, NULL);
		
		
		sid3 = g_io_add_watch_full(gpsd_io_channel, G_PRIORITY_HIGH_IDLE+200, G_IO_IN | G_IO_PRI, cb_gpsd_data, NULL, NULL);
	
	} else {
		fprintf(stderr, "connection to gpsd FAILED \n");
	}
	
	return NULL;
}

#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

#include "provider.h"

static void cb_changed(LocationGPSDevice *device, gpointer user_data);

extern tangogps_gps_data_t *gpsdata;

LocationGPSDControl *control = NULL;
LocationGPSDevice *device = NULL;

gboolean had_first_fix;

void
_gps_provider_init()
{
	if (control || device)
		_gps_provider_close();

	control = location_gpsd_control_get_default();
	device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);

	g_object_set(G_OBJECT(control), "preferred-method", LOCATION_METHOD_USER_SELECTED, NULL);
	g_object_set(G_OBJECT(control), "preferred-interval", LOCATION_INTERVAL_1S, NULL);

	//g_signal_connect(control, "error-verbose", G_CALLBACK(cb_error), NULL);
	g_signal_connect(device, "changed", G_CALLBACK(cb_changed), NULL);

	gpsdata = g_new0(tangogps_gps_data_t,1);

	location_gpsd_control_start(control);

	had_first_fix = FALSE;
}

void
_gps_provider_close()
{
	if (control)
		location_gpsd_control_stop(control);

	if (device)
		g_object_unref(device);
	device = NULL;
	if (control)
		g_object_unref(control);
	control = NULL;
    if (gpsdata)
            g_free(gpsdata);
    gpsdata = NULL;
}

static void cb_changed(LocationGPSDevice *device, gpointer user_data)
{
	memset(gpsdata, 0, sizeof(tangogps_gps_data_t));

	if (!device)
		return;

	gpsdata->valid = FALSE;

	if (device->fix)
	{
		if (device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET)
		{
			if (!had_first_fix)
			{
				had_first_fix = TRUE;
				// skip first fix, is often bogus
				return;
			}

			gpsdata->fix.latitude = device->fix->latitude;
			gpsdata->fix.longitude = device->fix->longitude;
			printf("** lat, lon = %f, %f\n", device->fix->latitude, device->fix->longitude);

			gpsdata->valid = TRUE;
			gpsdata->seen_valid = TRUE;
		}
		if (device->fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET)
		{
			gpsdata->fix.altitude = device->fix->altitude;
			printf("** alt = %f\n", device->fix->altitude);
		}
		if (device->fix->fields & LOCATION_GPS_DEVICE_TIME_SET)
		{
			gpsdata->fix.time = (time_t) device->fix->time;
			printf("** time = %f\n", device->fix->time);
		}
		if (device->fix->fields & LOCATION_GPS_DEVICE_TRACK_SET)
		{
			gpsdata->fix.bearing = device->fix->track;
			printf("** track/bearing = %f\n", device->fix->track);
		}

		gpsdata->hdop = device->fix->eph/100;
	}

	gpsdata->satellites_inview = device->satellites_in_view;
	gpsdata->satellites_used = device->satellites_in_use;
}

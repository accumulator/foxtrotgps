#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

#include "provider.h"

static void cb_changed(LocationGPSDevice *device, gpointer user_data);

extern tangogps_gps_data_t *gpsdata;

LocationGPSDControl *control = NULL;
LocationGPSDevice *device = NULL;

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

	if (device->fix)
	{
		if (device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET)
		{
			gpsdata->fix.latitude = device->fix->latitude;
			gpsdata->fix.longitude = device->fix->longitude;
			g_debug("lat = %f, long = %f", device->fix->latitude, device->fix->longitude);
		}

		if (device->fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET)
		{
			gpsdata->fix.altitude = device->fix->altitude;
			g_debug("alt = %f", device->fix->altitude);
		}
		g_debug("horizontal accuracy: %f meters", device->fix->eph/100);
	}

	g_debug("Satellites in view: %d, in use: %d", device->satellites_in_view, device->satellites_in_use);
}

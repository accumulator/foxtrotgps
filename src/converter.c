#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gprintf.h>

#include "converter.h"

char *
latdeg2latmin(	float lat)
{
	static char latmin[20];
	int degrees;
	float minutes;
	
	degrees = abs( (lat>0) ? floor(lat) : ceil(lat) );
	minutes = (fabs(lat) - (float)degrees) * 60;
	
	g_sprintf(	latmin,
			"%d°%.3f' %s", 
			degrees,
			minutes,
			(lat>0) ? "N" : "S");
	
	return latmin;
}

char *
londeg2lonmin(	float lon)
{
	static char lonmin[20];
	int degrees;
	float minutes;
	
	degrees = abs( (lon>0) ? floor(lon) : ceil(lon) );
	minutes = (fabs(lon) - (float)degrees) * 60;
	
	g_sprintf(	lonmin,
			"%d°%.3f' %s",
			degrees,
			minutes,
			(lon>0) ? "E" : "W");
	
	return lonmin;
}

char *
latdeg2latsec(	float lat)
{
	static char latsec[20];
	int degrees, full_minutes;
	float minutes, seconds;
	
	degrees = abs( (lat>0) ? floor(lat) : ceil(lat) );
	minutes = (fabs(lat) - (float)degrees) * 60;
	full_minutes = floor(minutes);
	seconds = (minutes - (float)full_minutes) * 60;
	
	g_sprintf(	latsec,
			"%d°%d'%.2f\" %s", 
			degrees,
			full_minutes,
			seconds,
			(lat>0) ? "N" : "S");
	
	return latsec;
}

char *
londeg2lonsec(	float lon)
{
	static char lonmin[20];
	int degrees, full_minutes;
	float minutes, seconds;
	
	degrees = abs( (lon>0) ? floor(lon) : ceil(lon) );
	minutes = (fabs(lon) - (float)degrees) * 60;
	full_minutes = floor(minutes);
	seconds = (minutes - (float)full_minutes) * 60;
	
	g_sprintf(	lonmin,
			"%d°%d'%.2f\" %s",
			degrees,
			full_minutes,
			seconds,
			(lon>0) ? "E" : "W");
	
	return lonmin;
}


float
get_bearing(double lat1, double lon1, double lat2, double lon2)
{
	double bearing, tmp;

	tmp = atan2(sin(DEG2RAD(lon2)-DEG2RAD(lon1)) * cos(DEG2RAD(lat2)),
		    cos(DEG2RAD(lat1)) * sin(DEG2RAD(lat2)) -
		    sin(DEG2RAD(lat1)) * cos(DEG2RAD(lat2)) * cos(DEG2RAD(lon2)-DEG2RAD(lon1)) );

	bearing = (tmp < 0) ? tmp + M_PI*2 : tmp;

	return RAD2DEG(bearing);
}

float
get_distance(double lat1, double lon1, double lat2, double lon2)
{
	float distance = 0;
	double tmp;
	
	tmp = sin(DEG2RAD(lat1)) * sin(DEG2RAD(lat2)) + cos(DEG2RAD(lat1)) * cos(DEG2RAD(lat2)) * cos(DEG2RAD(lon2) - DEG2RAD(lon1));
	
	distance = 6371.0 * acos(tmp);
	
	return distance;
}

int
get_zoom_covering(int width, int height, double lat_max, double lon_min, double lat_min, double lon_max)
{
	int pixel_x1, pixel_y1, pixel_x2, pixel_y2;
	float zoom = 17.0;

	while (zoom>=2)
	{
		
		pixel_x1 = lon2pixel((float)zoom, lon_min);
		pixel_y1 = lat2pixel((float)zoom, lat_max);
		
		pixel_x2 = lon2pixel((float)zoom, lon_max);
		pixel_y2 = lat2pixel((float)zoom, lat_min);
		
		
		if( 	(pixel_x1+width)  > pixel_x2	&&
			(pixel_y1+height) > pixel_y2 )
		{
			return zoom;
		}
		zoom--;
	}	
	
	return zoom;
}

#define DEG2RAD(deg) (deg * M_PI / 180.0)
#define RAD2DEG(rad) (rad / M_PI * 180.0)

char *
latdeg2latmin(	float lat);

char *
londeg2lonmin(	float lon);

char *
latdeg2latsec(	float lat);

char *
londeg2lonsec(	float lon);

float
get_bearing(double lat1, double lon1, double lat2, double lon2);

float
get_distance(double lat1, double lon1, double lat2, double lon2);

int
get_zoom_covering(int width, int height, double lat_max, double lon_min, double lat_min, double lon_max);

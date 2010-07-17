typedef struct {
	double time;		/* Time of update, seconds since Unix epoch */
	int    mode;		/* Mode of fix */
	double latitude;	/* Latitude in degrees (valid if mode >= 2) */
	double longitude;	/* Longitude in degrees (valid if mode >= 2) */
	double altitude;	/* Altitude in meters (valid if mode == 3) */
	double heading;
	double speed;		/* Speed over ground, meters/sec */
	double bearing;		/* in radian, calculated by tangogps */
} tangogps_gps_fix_t;

typedef struct {
	tangogps_gps_fix_t fix;
	int satellites_used;
	int satellites_inview;
	double hdop;
	gboolean valid;
	gboolean seen_valid; /* ever had a valid fix? */
} tangogps_gps_data_t;

void
_gps_provider_init();

void
_gps_provider_close();


typedef struct {
	double lat1;
	double lon1;
	double lat2;
	double lon2;
} bbox_t;

int
update_thread_number (int change);

void
cb_download_maps(GtkWidget *dialog);

bbox_t
get_bbox();

bbox_t
get_bbox_deg();

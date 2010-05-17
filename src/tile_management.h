
typedef struct {
	double lat1;
	double lon1;
	double lat2;
	double lon2;
} bbox_t;

typedef struct {
	int x1;
	int y1;
	int x2;
	int y2;
} bbox_pixel_t;


typedef struct {
	int x;
	int y;
	int zoom;
	repo_t *repo;
} tile_t;

int
update_thread_number (int change);

void
cb_download_maps(GtkWidget *dialog);

bbox_t
get_bbox();

bbox_t
get_bbox_deg();

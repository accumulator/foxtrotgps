

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gstring.h>
#include <glib/gprintf.h>
#include <gconf/gconf.h>
#include <math.h>
#include "support.h"
#include "globals.h"
#include "map_management.h"
#include "converter.h"
#include "tile_management.h"
#include "callbacks.h"
#include "friends.h"
#include "gps_functions.h"
#include "geo_photos.h"
#include "poi.h"
#include "wp.h"
#include "tracks.h"

typedef struct {
	GdkPixbuf *pixbuf;
	gboolean reused;
} tile_hash_t;


static GdkGC		*gc_map = NULL;

static GtkWidget	*drawingarea11 = NULL;
static GHashTable	*hash_table = NULL;

void hash_destroy_key_func   (gpointer data);
void hash_destroy_value_func (gpointer data);
gboolean hash_sieve_func (gpointer key, gpointer value, gpointer user_data);

void
hash_destroy_key_func   (gpointer data)
{
	g_free(data);
}

void
hash_destroy_value_func (gpointer data)
{
	tile_hash_t *tile_hash = data;

	g_object_unref(tile_hash->pixbuf);
	g_free(data);
}

gboolean
hash_sieve_func (gpointer key, gpointer value, gpointer user_data)
{
	gboolean remove = TRUE;
	
	tile_hash_t *tile_hash = value;
	
	if(tile_hash->reused)
	{
		tile_hash->reused = FALSE;
		remove = FALSE;
	}

	return remove;
}



void
fill_tiles_pixel(	int pixel_x,
			int pixel_y,
			int zoom,
			gboolean force_refresh)
{
	GtkWidget *widget;
	int i,j, i_corrected, width, height, tile_x0, tile_y0, tiles_nx, tiles_ny;
	int max_pixel;
	int offset_xn = 0;
	int offset_yn = 0;
	int offset_x;
	int offset_y;
	gboolean success = FALSE;
	GError **error = NULL;
	repo_t *repo = g_new0(repo_t, 1);
	
	printf("* %s() deprecated\n", __PRETTY_FUNCTION__);
	return;
	
	if (!hash_table)
	{
		hash_table = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    hash_destroy_key_func,
						    hash_destroy_value_func);
	}
	
	if(force_refresh)
	{
		g_hash_table_remove_all(hash_table);	
	}
	
	widget = lookup_widget(window1,"drawingarea1");
	
	repo = global_curr_repo->data;

	
	max_pixel = (int) exp2(zoom) * TILESIZE; 

	if(pixel_x < 0)
		pixel_x += max_pixel;
	
	else if (pixel_x > max_pixel)
		pixel_x -= max_pixel;	

	
	offset_x = - pixel_x % TILESIZE;
	offset_y = - pixel_y % TILESIZE;
	if (offset_x > 0) offset_x -= 256;
	if (offset_y > 0) offset_y -= 256;
	
	global_x = pixel_x;
	global_y = pixel_y;
	global_zoom = zoom;
	
	offset_xn = offset_x; 
	offset_yn = offset_y;

	width  = map_drawable->allocation.width;
	height = map_drawable->allocation.height;

	tiles_nx = floor((width  - offset_x) / TILESIZE) + 1;
	tiles_ny = floor((height - offset_y) / TILESIZE) + 1;
	
	tile_x0 =  floor((float)pixel_x / (float)TILESIZE);
	tile_y0 =  floor((float)pixel_y / (float)TILESIZE);
	
	

	for (i=tile_x0; i<(tile_x0+tiles_nx); i++)
	{
		for (j=tile_y0;  j<(tile_y0+tiles_ny); j++)
		{
			
			

			
			if(j<0 || j>=exp2(zoom))
			{
				gdk_draw_rectangle (
					pixmap,
					widget->style->white_gc,
					TRUE,
					offset_xn, offset_yn,
					TILESIZE,
					TILESIZE);
				
				gtk_widget_queue_draw_area (
					widget, 
					offset_xn,offset_yn,
					TILESIZE,TILESIZE);
			}
			else
			{	
				i_corrected = (i>=exp2(zoom)) ? i-exp2(zoom) : i;
				
				load_tile(
					repo->dir,
					zoom,
					i_corrected,j,
					offset_xn,offset_yn);
			}
			offset_yn += TILESIZE;
		}
		offset_xn += TILESIZE;
		offset_yn = offset_y;
	}

	
	g_hash_table_foreach_remove (hash_table, hash_sieve_func, NULL);
	
	
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_x",
				global_x,
				error);
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_y",
				global_y,
				error);
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_zoom",
				global_zoom,
				error);
}

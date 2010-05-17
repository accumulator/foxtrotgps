/***************************************************************************
 *            wp.h
 *
 *  Wed Jun  4 21:07:49 2008
 *  Copyright  2008  Marcus Bauer 
 *  GPLv2
 *  Email marcus.bauer@gmail.com
 ****************************************************************************/

#include <osm-gps-map.h>

void
paint_wp();

void
paint_myposition();

void
do_paint_wp();

void
set_current_wp(coord_t *coord);

void
osd_wp();

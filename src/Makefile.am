## Process this file with automake to produce Makefile.in

if ENABLE_MAEMO5
gps_sources = gps/maemo5_provider.c
else
gps_sources = gps/gpsd_provider.c
endif

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR="\"$(datadir)\"" \
	-DPACKAGE_PIXMAPS_DIR="\"$(datadir)/pixmaps\"" \
	-DPACKAGE_LOCALE_DIR="\"$(localedir)\"" \
	\
	$(GTK_CFLAGS) \
	$(GLADE_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(LIBCURL_CFLAGS) \
	$(SQLITE_CFLAGS) \
	$(LIBEXIF_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(LIBGPS_CFLAGS) \
	$(LIBLOCATION_CFLAGS) \
	$(OSMGPSMAP_CFLAGS)

bin_PROGRAMS = foxtrotgps

foxtrotgps_SOURCES = \
	support.h\
	support.c\
	main.c\
	interface.h\
	interface.c\
	callbacks.h\
	callbacks.c\
	globals.c\
	globals.h\
	converter.c\
	converter.h\
	gps/provider.h \
	$(gps_sources) \
	gps_functions.c\
	gps_functions.h\
	init.c\
	init.h\
	friends.c\
	friends.h\
	gpsdstart.sh\
	geo_photos.c\
	geo_photos.h\
	poi.c\
	poi.h\
	util.c\
	util.h\
	wp.c\
	wp.h\
	tracks.c\
	tracks.h\
	exif.c\
	exif.h

foxtrotgps_LDFLAGS = -export-dynamic

foxtrotgps_LDADD =  \
	$(GTK_LIBS) \
	$(GLADE_LIBS) \
	$(GCONF_LIBS) \
	$(LIBCURL_LIBS) \
	$(SQLITE_LIBS) \
	$(LIBEXIF_LIBS) \
	$(LIBXML_LIBS) \
	$(LIBGPS_LIBS) \
	$(LIBLOCATION_LIBS) \
	$(OSMGPSMAP_LIBS) \
	-lm

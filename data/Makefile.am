## Process this file with automake to produce Makefile.in

foxtrotgps_datadir = $(datadir)/applications
foxtrotgps_data_in_files = foxtrotgps.desktop.in
dist_foxtrotgps_data_DATA = $(foxtrotgps_data_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@


pkgdata_DATA = foxtrotgps.glade

EXTRA_DIST = $(foxtrotgps_data_in_files)

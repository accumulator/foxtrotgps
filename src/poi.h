void
set_pois_show(gboolean show);

void
insert_poi(GtkWidget *dialog);

void
update_poi(GtkWidget *dialog);

void
delete_poi(poi_t *p);

void
show_window6();

void
on_combobox_cat_changed(GtkComboBox     *combobox);

void
get_pois();

void
get_pois_for_bbox(float lat1, float lon1, float lat2, float lon2);

void
create_pois_window();

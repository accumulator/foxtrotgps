void
track_log();

void
track_log_open();

void
track_log_close();

void
tracks_open_tracks_dialog();

gboolean
load_track(char *file);

gboolean
tracks_on_file_button_release_event      (	GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
fetch_track(GtkWidget *widget, int service, char *start, char *end);

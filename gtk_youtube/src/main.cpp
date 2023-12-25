#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "ytclient.h"

// Function to toggle fullscreen mode
void toggle_fullscreen(GtkWidget *widget, GdkEventKey *event, GtkWindow *window) {
    if (event->keyval == GDK_KEY_Escape) {
        GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
        GdkWindowState state = gdk_window_get_state(gdk_window);
        if (state & GDK_WINDOW_STATE_FULLSCREEN) {
            gtk_window_unfullscreen(window);
        }
    }
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "YouTube GTK WebView");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    GdkPixbuf *icon = gdk_pixbuf_new_from_file("./icon.png", NULL);
    if (icon != NULL) {
        gtk_window_set_icon(GTK_WINDOW(window), icon);
        g_object_unref(icon);
    }
    
    // Connect the key-release-event signal to the toggle_fullscreen function
    g_signal_connect(window, "key-release-event", G_CALLBACK(toggle_fullscreen), window);

    GtkYoutubeClient* client = new GtkYoutubeClient(window);
    gtk_main();

    return 0;
}

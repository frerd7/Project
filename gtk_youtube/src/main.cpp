/* Copyright (C) 2023 Frederick Valdez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

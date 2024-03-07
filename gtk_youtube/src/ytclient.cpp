#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "ytclient.h"
#include "gtkabout.h"

GtkYoutubeClient::GtkYoutubeClient(GtkWidget *window) :
    parent(window),
    webview (WEBKIT_WEB_VIEW(webkit_web_view_new())), /* webview param*/
    scrolledWindow (gtk_scrolled_window_new(NULL, NULL)), /* scroll param */
    box (gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)),  /* box param */   
    headerBar (gtk_header_bar_new()), /* headerBar */
    fullscreenButton (gtk_button_new_from_icon_name("gtk-fullscreen", GTK_ICON_SIZE_SMALL_TOOLBAR)), /* fullscreen param */
    aboutButtom (gtk_button_new_from_icon_name("info", GTK_ICON_SIZE_SMALL_TOOLBAR)),
    about (new YtAbout(window, aboutButtom))
{

  check_connection();
  
  load_url(webview); /* load url web */
  create_header(webview, window); /* create content */
  
  /* settings window */
  gtk_container_add(GTK_CONTAINER(window), box); 
  g_signal_connect(window, "destroy", G_CALLBACK(destroyWindow), NULL);
  gtk_widget_show_all(window);
}

void GtkYoutubeClient::load_url(WebKitWebView* webview)
{ 
  /* url web */ 
  webkit_web_view_load_uri(webview, "https://www.youtube.com");
  g_signal_connect(webview, "notify::title", G_CALLBACK(titleChangedCallback), parent);
  g_signal_connect(webview, "permission-request", G_CALLBACK(permission_request_callback), NULL);
}

void GtkYoutubeClient::check_connection()
{
    if (!check_internet_connection()) 
    {  
        const gchar *message = "No internet connection!";
        GtkMessageType type = GTK_MESSAGE_WARNING;
        GtkButtonsType buttons = GTK_BUTTONS_OK;
        GtkDialog *dialog = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, type, buttons, "%s", message));
        gtk_dialog_run(dialog);
        gtk_widget_destroy(GTK_WIDGET(dialog));
        exit(1);
    }
}

void GtkYoutubeClient::create_header(WebKitWebView* webview, GtkWidget *window)
{
   /* settings header */
   gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(webview));
   gtk_box_pack_start(GTK_BOX(box), scrolledWindow, TRUE, TRUE, 0);
   gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerBar), TRUE);
   gtk_header_bar_set_title(GTK_HEADER_BAR(headerBar), "YouTube Client"); /* title */
   gtk_window_set_titlebar(GTK_WINDOW(window), headerBar);
   g_signal_connect(fullscreenButton, "clicked", G_CALLBACK(Fullscreen), window); /* add bottom */        
   
   // options 
   gtk_widget_set_tooltip_text(fullscreenButton, "Mode Full Screen");   
   gtk_widget_set_tooltip_text(aboutButtom, "About");
   
   // pack object             
   gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), fullscreenButton);
   gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), aboutButtom);
}

GtkYoutubeClient::~GtkYoutubeClient()
{
   g_object_unref(webview);
}


#ifndef YTCLIENT_H
#define YTCLIENT_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <gio/gio.h>
#include "gtkabout.h"

class YtAbout;
class GtkYoutubeClient {
public:
    explicit GtkYoutubeClient(GtkWidget *window);
    ~GtkYoutubeClient();
    
    void load_url(WebKitWebView* webview);
    void create_header(WebKitWebView* webview, GtkWidget *window);
    void dark_mode(GtkWidget* widget, gpointer data);
    
    static void Fullscreen(GtkWidget* widget, gpointer data) 
    {
         GtkWindow* window = GTK_WINDOW(data);
         gtk_window_fullscreen(window);
    }
    
    static void destroyWindow(GtkWidget* widget, gpointer data) 
    {
         gtk_main_quit();
    }  
    
    static void titleChangedCallback(WebKitWebView* webView, GParamSpec* pspec, gpointer user_data) 
    {
    	const gchar* title = webkit_web_view_get_title(webView);
    	gtk_window_set_title(GTK_WINDOW(user_data), title);
    }

    static gboolean check_internet_connection() 
    {
    	GNetworkMonitor *monitor = g_network_monitor_get_default();
    	if (monitor) 
    	{
        	GNetworkConnectivity connectivity = g_network_monitor_get_connectivity(monitor);
        	g_object_unref(monitor);
        	return (connectivity != G_NETWORK_CONNECTIVITY_LOCAL &&
                	connectivity != G_NETWORK_CONNECTIVITY_LIMITED);
    	}
    	return FALSE;
    }

   void check_connection();
    
   static void permission_request_callback(WebKitPermissionRequest* request, gpointer user_data) 
   {
      GType request_type = G_TYPE_FROM_INSTANCE(request);
      if (g_type_is_a(request_type, WEBKIT_TYPE_PERMISSION_REQUEST)) 
       {
         webkit_permission_request_allow(request);
       } 
      else 
        {
          webkit_permission_request_deny(request);
        }
   }
   
private:
   WebKitWebView* webview;
   GtkWidget* scrolledWindow;
   GtkWidget* box;
   GtkWidget* headerBar;
   GtkWidget* fullscreenButton;
   GtkWidget* aboutButtom;
   GtkWidget* color;
   GtkWidget* parent;
   YtAbout* about;
};
#endif // YTCLIENT_H

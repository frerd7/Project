#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "gtkabout.h"

YtAbout::YtAbout(GtkWidget *widget, GtkWidget* aboutButtom) :
      parentWidget(widget),
      about (GTK_ABOUT_DIALOG(gtk_about_dialog_new()))
{
  // Constructor
  g_signal_connect(aboutButtom, "clicked", 
  		    G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                       static_cast<YtAbout*>(data)->show_about(widget, data);
                    }), this);
  
                    
}

void YtAbout::show_about(GtkWidget *widget, gpointer data)
{
   if(about != NULL)
   {
   	gtk_about_dialog_set_program_name(about, "Youtube Demo");
   	gtk_about_dialog_set_version(about, "v1.0");
   	gtk_about_dialog_set_comments(about, "This program is a trial demo. I have no intention of"                                      "publishing it as a final program, as it is simply a demo. I'm testing adaptive technologies.");
   
   	GdkPixbuf *icon = gdk_pixbuf_new_from_file("./icon.png", NULL);
   	GdkPixbuf *scaledIcon = gdk_pixbuf_scale_simple(icon, 64, 64, GDK_INTERP_BILINEAR);
   	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), scaledIcon);
   
   	const gchar *autor[] = {"Frederick Valdez", NULL};
   
   	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), autor);
   	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), "© 2023 Free Software Foundation, Inc.");
    
   	g_signal_connect(about, "response", G_CALLBACK(on_response), NULL);
   	gtk_widget_show_all(GTK_WIDGET(about));
   }
}

YtAbout::~YtAbout()
{
  // Destructor
  g_object_unref(about);
  gtk_widget_destroy(parentWidget);
}

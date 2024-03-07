#ifndef GtkAbout_H
#define GtkAbout_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

class YtAbout {
public:
	explicit YtAbout (GtkWidget *widget, GtkWidget* aboutButtom);
	~YtAbout();
	
	void show_about(GtkWidget *widget, gpointer data);
	static void on_response(GtkWidget *widget, gpointer data) 
	{
    		gtk_widget_destroy(widget);
	}
	
private:
	GtkAboutDialog *about;
	GtkWidget *parentWidget;
};

#endif // GtkAbout_H

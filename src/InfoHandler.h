#ifndef INFOHANDLER_H_FFA18220_6DEF_11E4_9C9B_10FEED04CD1C
#define INFOHANDLER_H_FFA18220_6DEF_11E4_9C9B_10FEED04CD1C

#include "ISdl2App.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

class InfoHandler {
public:
	InfoHandler(ISdl2App * app, GtkWidget * window, GtkBuilder * builder);
	~InfoHandler();

	void showInfo();

private:
	ISdl2App   * mxApp;
	GtkWidget  * mxWindow;

	GtkWidget  * mpWidgetXPosition;
	GtkWidget  * mpWidgetYPosition;
	GtkWidget  * mpWidgetZPosition;

	GtkWidget  * mpWidgetXSpeed;
	GtkWidget  * mpWidgetYSpeed;
	GtkWidget  * mpWidgetZSpeed;
};

#endif // SHOWINFO_H_FFA18220_6DEF_11E4_9C9B_10FEED04CD1C


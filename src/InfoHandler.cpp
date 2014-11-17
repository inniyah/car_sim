#include "InfoHandler.h"
#include "InfoTypes.h"
#include "Common.h"

#include <cstdio>
#include <cstdarg>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

InfoHandler::InfoHandler(ISdl2App * app, GtkWidget * window, GtkBuilder * builder) : mxApp(app), mxWindow(window) {
	mpWidgetXPosition = GTK_WIDGET(gtk_builder_get_object(builder, "position_x"));
	mpWidgetYPosition = GTK_WIDGET(gtk_builder_get_object(builder, "position_y"));
	mpWidgetZPosition = GTK_WIDGET(gtk_builder_get_object(builder, "position_z"));
	mpWidgetXSpeed    = GTK_WIDGET(gtk_builder_get_object(builder, "speed_x"));
	mpWidgetYSpeed    = GTK_WIDGET(gtk_builder_get_object(builder, "speed_y"));
	mpWidgetZSpeed    = GTK_WIDGET(gtk_builder_get_object(builder, "speed_z"));
	printf("InfoHandler Created\n");
}

InfoHandler::~InfoHandler() {
	printf("InfoHandler Destroyed\n");
}

void InfoHandler::showInfo() {
	float pos[3];
	char  buff[16];
	if (mxApp->getInfo(pos, INFO_POSITION_3F, 0)) {
		snprintf(buff, sizeof(buff), "X: %.2f", pos[0]);
		gtk_label_set_text(GTK_LABEL(mpWidgetXPosition), buff);
		snprintf(buff, sizeof(buff), "Y: %.2f", pos[1]);
		gtk_label_set_text(GTK_LABEL(mpWidgetYPosition), buff);
		snprintf(buff, sizeof(buff), "Z: %.2f", pos[2]);
		gtk_label_set_text(GTK_LABEL(mpWidgetZPosition), buff);
	}
	if (mxApp->getInfo(pos, INFO_SPEED_3F, 0)) {
		snprintf(buff, sizeof(buff), "X: %.2f", pos[0]);
		gtk_label_set_text(GTK_LABEL(mpWidgetXSpeed), buff);
		snprintf(buff, sizeof(buff), "Y: %.2f", pos[1]);
		gtk_label_set_text(GTK_LABEL(mpWidgetYSpeed), buff);
		snprintf(buff, sizeof(buff), "Z: %.2f", pos[2]);
		gtk_label_set_text(GTK_LABEL(mpWidgetZSpeed), buff);
	}
}

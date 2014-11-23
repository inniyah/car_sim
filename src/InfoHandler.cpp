#include "InfoHandler.h"
#include "InfoTypes.h"
#include "Common.h"

#include <cstdio>
#include <cstdarg>
#include <cmath>

#ifndef M_PI
#define M_PI 3.141592654
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

InfoHandler::InfoHandler(ISdl2App * app, GtkWidget * window, GtkBuilder * builder) : mxApp(app), mxWindow(window) {
	mpWidgetXPosition          = GTK_WIDGET(gtk_builder_get_object(builder, "position_x"));
	mpWidgetYPosition          = GTK_WIDGET(gtk_builder_get_object(builder, "position_y"));
	mpWidgetZPosition          = GTK_WIDGET(gtk_builder_get_object(builder, "position_z"));
	mpWidgetXSpeed             = GTK_WIDGET(gtk_builder_get_object(builder, "speed_x"));
	mpWidgetYSpeed             = GTK_WIDGET(gtk_builder_get_object(builder, "speed_y"));
	mpWidgetZSpeed             = GTK_WIDGET(gtk_builder_get_object(builder, "speed_z"));
	mpWidgetYawAngle           = GTK_WIDGET(gtk_builder_get_object(builder, "angle_yaw"));
	mpWidgetPitchAngle         = GTK_WIDGET(gtk_builder_get_object(builder, "angle_pitch"));
	mpWidgetRollAngle          = GTK_WIDGET(gtk_builder_get_object(builder, "angle_roll"));
	mpWidgetLastCheckpoint     = GTK_WIDGET(gtk_builder_get_object(builder, "checkpoint_last"));
	mpWidgetCurrentCheckpoint  = GTK_WIDGET(gtk_builder_get_object(builder, "checkpoint_current"));
	printf("InfoHandler Created\n");
}

InfoHandler::~InfoHandler() {
	printf("InfoHandler Destroyed\n");
}

void InfoHandler::showInfo() {
	float pos[3];
	float spd[3];
	float ang[3];
	int   chk[2];
	char  buff[16];

	if (mxApp->getInfo(pos, INFO_POSITION_3F, 0)) {
		snprintf(buff, sizeof(buff), "X: %.3f cm", 1.0 * pos[0]);
		gtk_label_set_text(GTK_LABEL(mpWidgetXPosition), buff);
		snprintf(buff, sizeof(buff), "Y: %.3f cm", 1.0 * pos[1]);
		gtk_label_set_text(GTK_LABEL(mpWidgetYPosition), buff);
		snprintf(buff, sizeof(buff), "Z: %.3f cm", 1.0 * pos[2]);
		gtk_label_set_text(GTK_LABEL(mpWidgetZPosition), buff);
	}
	if (mxApp->getInfo(spd, INFO_SPEED_3F, 0)) {
		snprintf(buff, sizeof(buff), "X: %.2f", spd[0]);
		gtk_label_set_text(GTK_LABEL(mpWidgetXSpeed), buff);
		snprintf(buff, sizeof(buff), "Y: %.2f", spd[1]);
		gtk_label_set_text(GTK_LABEL(mpWidgetYSpeed), buff);
		snprintf(buff, sizeof(buff), "Z: %.2f", spd[2]);
		gtk_label_set_text(GTK_LABEL(mpWidgetZSpeed), buff);
	}
	if (mxApp->getInfo(ang, INFO_ANGLES_3F, 0)) {
		snprintf(buff, sizeof(buff), "Yaw: %.2f", ang[0] * 360.0 / (2 * M_PI) );
		gtk_label_set_text(GTK_LABEL(mpWidgetYawAngle), buff);
		snprintf(buff, sizeof(buff), "Pitch: %.2f", ang[1] * 360.0 / (2 * M_PI) );
		gtk_label_set_text(GTK_LABEL(mpWidgetPitchAngle), buff);
		snprintf(buff, sizeof(buff), "Roll: %.2f", ang[2] * 360.0 / (2 * M_PI) );
		gtk_label_set_text(GTK_LABEL(mpWidgetRollAngle), buff);
	}
	if (mxApp->getInfo(chk, INFO_CHECKPOINT_2I, 0)) {
		snprintf(buff, sizeof(buff), "Curr: %d", chk[0] );
		gtk_label_set_text(GTK_LABEL(mpWidgetCurrentCheckpoint), buff);
		snprintf(buff, sizeof(buff), "Last: %d", chk[1] );
		gtk_label_set_text(GTK_LABEL(mpWidgetLastCheckpoint), buff);
	}
}

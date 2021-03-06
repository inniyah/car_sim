#include "MainGtk3App.h"
#include "ISdl2App.h"
#include "InfoHandler.h"
#include "Common.h"

#include <gamepad/Gamepad.h>

#include <cstdio>
#include <cstdarg>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <SDL2/SDL.h>
#include "SDL2/SDL_events.h"

#define FPS 60

#define UI_FILE "data/app.ui"

#define ID_TOP_WINDOW   "window"
#define ID_SDL_WIDGET   "sdl"
#define ID_LOG_WIDGET   "log"

#define LOG_MAXLENGTH     256

G_BEGIN_DECLS

#define MAIN_APP_TYPE                         (main_app_get_type ())
#define MAIN_APP_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAIN_APP_TYPE,  MainApp))
#define MAIN_APP_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  MAIN_APP_TYPE,  MainAppClass))
#define MAIN_APP_IS_APPLICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAIN_APP_TYPE))
#define MAIN_APP_IS_APPLICATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAIN_APP_TYPE))
#define MAIN_APP_APPLICATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAIN_APP_TYPE,  MainAppClass))

struct MainAppClass {
	GtkApplicationClass parent_class;
};

struct MainAppPrivateData;

struct MainApp {
	GtkApplication parent_instance;
	MainAppPrivateData *priv;

	static gboolean draw(gpointer user_data);
	static void setup(GApplication * app);
	static void cleanup(GtkApplication * app);
	static void open(GApplication * application, GFile ** files, gint n_files, const gchar * hint );
	static void activate(GApplication *application);
	static void finalize(GObject * object);
	static void createFromFile(GApplication * app, GFile * file);
};

struct Sdl2Area {
	static SDL_Keycode GdkKeyToSdlKeycode(guint gdk_key);

	static gboolean eventWindowState (GtkWidget * widget, GdkEventWindowState * winstate, gpointer user_data);
	static gboolean eventConfigure   (GtkWidget * widget, GdkEventConfigure   * config,   gpointer user_data);
	static gboolean eventMotion      (GtkWidget * widget, GdkEventMotion      * motion,   gpointer user_data);
	static gboolean eventScroll      (GtkWidget * widget, GdkEventScroll      * scroll,   gpointer user_data);
	static gboolean eventButton      (GtkWidget * widget, GdkEventButton      * button,   gpointer user_data);
	static gboolean eventKey         (GtkWidget * widget, GdkEventKey         * key,      gpointer user_data);
	static gboolean eventEnter       (GtkWidget * widget, GdkEventCrossing    * crossing, gpointer user_data);
	static gboolean eventLeave       (GtkWidget * widget, GdkEventCrossing    * crossing, gpointer user_data);
	static gboolean eventFocus       (GtkWidget * widget, GdkEventFocus       * focus,    gpointer user_data);
};

struct Gamepad {
	static void onButtonDown(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context);
	static void onButtonUp(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context);
	static void onAxisMoved(struct Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp, void * context);
	static void onDeviceAttached(struct Gamepad_device * device, void * context);
	static void onDeviceRemoved(struct Gamepad_device * device, void * context);
	static void initGamepad(void * context);
	static void shutdownGamepad();
};

GType main_app_get_type(void) G_GNUC_CONST;
MainApp * main_app_new(void);

G_END_DECLS

G_DEFINE_TYPE(MainApp, main_app, GTK_TYPE_APPLICATION);

/* Define the private structure in the .c file */
#define MAIN_APP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MAIN_APP_TYPE, MainAppPrivateData))

struct MainAppPrivateData {
	GtkWidget *    sdl_widget;
	GtkWidget *    log_widget;

	SDL_Window *   sdl_window;
	ISdl2App   *   sdl_app;

	InfoHandler *  info_handler;

	guint idle_handler;
};

gboolean MainApp::draw(gpointer user_data) {
	MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(G_APPLICATION (user_data));
	Gamepad_processEvents();
	priv->sdl_app->processEvents();
	priv->sdl_app->update();
	priv->sdl_app->draw();
	priv->info_handler->showInfo();
	return TRUE;
}

static void appendTextToWidget(LogType type, GtkWidget * widget, gchar * text, bool new_line = true) {
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;

	/* Returns the GtkTextBuffer being displayed by this text view. */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

	/* Returns the mark that represents the cursor (insertion point). */
	mark = gtk_text_buffer_get_insert(buffer);

	/* Initializes iter with the current position of mark. */
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

	/* Inserts buffer at position iter. */
	gtk_text_buffer_insert(buffer, &iter, text, -1);

	if (new_line) {
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	}

	/* Scrolls text_view the minimum distance such that mark is contained within the visible area of the widget. */
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(widget), mark);  
}

GtkWidget * global_log_widget = NULL;

// https://developer.gnome.org/gtk3/stable/GtkTextView.html

// Note: see http://zetcode.com/tutorials/gtktutorial/gtktextview/

void printLog(LogType type, const char* fmt, ...) {
	char buff[LOG_MAXLENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);
	buff[sizeof(buff) - 1] = '\0';

	puts(buff);
	appendTextToWidget(type, global_log_widget, buff);
}

// https://git.gnome.org/browse/gtk+/plain/gdk/gdkkeysyms.h
// https://wiki.libsdl.org/SDL_Keycode

SDL_Keycode Sdl2Area::GdkKeyToSdlKeycode(guint gdk_key) {
	//printf("Key Code: 0x%X\n", gdk_key);
	switch (gdk_key) {
		case GDK_KEY_A:              return SDLK_a;
		case GDK_KEY_B:              return SDLK_b;
		case GDK_KEY_C:              return SDLK_c;
		case GDK_KEY_D:              return SDLK_d;
		case GDK_KEY_E:              return SDLK_e;
		case GDK_KEY_F:              return SDLK_f;
		case GDK_KEY_G:              return SDLK_g;
		case GDK_KEY_H:              return SDLK_h;
		case GDK_KEY_I:              return SDLK_i;
		case GDK_KEY_J:              return SDLK_j;
		case GDK_KEY_K:              return SDLK_k;
		case GDK_KEY_L:              return SDLK_l;
		case GDK_KEY_M:              return SDLK_m;
		case GDK_KEY_N:              return SDLK_n;
		case GDK_KEY_O:              return SDLK_o;
		case GDK_KEY_P:              return SDLK_p;
		case GDK_KEY_Q:              return SDLK_q;
		case GDK_KEY_R:              return SDLK_r;
		case GDK_KEY_S:              return SDLK_s;
		case GDK_KEY_T:              return SDLK_t;
		case GDK_KEY_U:              return SDLK_u;
		case GDK_KEY_V:              return SDLK_v;
		case GDK_KEY_W:              return SDLK_w;
		case GDK_KEY_X:              return SDLK_x;
		case GDK_KEY_Y:              return SDLK_y;
		case GDK_KEY_Z:              return SDLK_z;

		case GDK_KEY_a:              return SDLK_a;
		case GDK_KEY_b:              return SDLK_b;
		case GDK_KEY_c:              return SDLK_c;
		case GDK_KEY_d:              return SDLK_d;
		case GDK_KEY_e:              return SDLK_e;
		case GDK_KEY_f:              return SDLK_f;
		case GDK_KEY_g:              return SDLK_g;
		case GDK_KEY_h:              return SDLK_h;
		case GDK_KEY_i:              return SDLK_i;
		case GDK_KEY_j:              return SDLK_j;
		case GDK_KEY_k:              return SDLK_k;
		case GDK_KEY_l:              return SDLK_l;
		case GDK_KEY_m:              return SDLK_m;
		case GDK_KEY_n:              return SDLK_n;
		case GDK_KEY_o:              return SDLK_o;
		case GDK_KEY_p:              return SDLK_p;
		case GDK_KEY_q:              return SDLK_q;
		case GDK_KEY_r:              return SDLK_r;
		case GDK_KEY_s:              return SDLK_s;
		case GDK_KEY_t:              return SDLK_t;
		case GDK_KEY_u:              return SDLK_u;
		case GDK_KEY_v:              return SDLK_v;
		case GDK_KEY_w:              return SDLK_w;
		case GDK_KEY_x:              return SDLK_x;
		case GDK_KEY_y:              return SDLK_y;
		case GDK_KEY_z:              return SDLK_z;

		case GDK_KEY_1:              return SDLK_1;
		case GDK_KEY_2:              return SDLK_2;
		case GDK_KEY_3:              return SDLK_3;
		case GDK_KEY_4:              return SDLK_4;
		case GDK_KEY_5:              return SDLK_5;
		case GDK_KEY_6:              return SDLK_6;
		case GDK_KEY_7:              return SDLK_7;
		case GDK_KEY_8:              return SDLK_8;
		case GDK_KEY_9:              return SDLK_9;
		case GDK_KEY_0:              return SDLK_0;

		case GDK_KEY_Return:         return SDLK_RETURN;
		case GDK_KEY_Escape:         return SDLK_ESCAPE;
		case GDK_KEY_BackSpace:      return SDLK_BACKSPACE;
		case GDK_KEY_Tab:            return SDLK_TAB;
		case GDK_KEY_space:          return SDLK_SPACE;

		case GDK_KEY_plus:           return SDLK_PLUS;
		case GDK_KEY_minus:          return SDLK_MINUS;
		case GDK_KEY_equal:          return SDLK_EQUALS;
		case GDK_KEY_bracketleft:    return SDLK_LEFTBRACKET;
		case GDK_KEY_bracketright:   return SDLK_RIGHTBRACKET;
		case GDK_KEY_backslash:      return SDLK_BACKSLASH;
		case GDK_KEY_colon:          return SDLK_COLON;
		case GDK_KEY_semicolon:      return SDLK_SEMICOLON;
		case GDK_KEY_apostrophe:     return SDLK_QUOTE;
		case GDK_KEY_grave:          return SDLK_BACKQUOTE;
		case GDK_KEY_comma:          return SDLK_COMMA;
		case GDK_KEY_period:         return SDLK_PERIOD;
		case GDK_KEY_slash:          return SDLK_SLASH;
		case GDK_KEY_Caps_Lock:      return SDLK_CAPSLOCK;

		case GDK_KEY_F1:             return SDLK_F1;
		case GDK_KEY_F2:             return SDLK_F2;
		case GDK_KEY_F3:             return SDLK_F3;
		case GDK_KEY_F4:             return SDLK_F4;
		case GDK_KEY_F5:             return SDLK_F5;
		case GDK_KEY_F6:             return SDLK_F6;
		case GDK_KEY_F7:             return SDLK_F7;
		case GDK_KEY_F8:             return SDLK_F8;
		case GDK_KEY_F9:             return SDLK_F9;
		case GDK_KEY_F10:            return SDLK_F10;
		case GDK_KEY_F11:            return SDLK_F11;
		case GDK_KEY_F12:            return SDLK_F12;

		case GDK_KEY_Print:          return SDLK_PRINTSCREEN;
		case GDK_KEY_Scroll_Lock:    return SDLK_SCROLLLOCK;
		case GDK_KEY_Pause:          return SDLK_PAUSE;
		case GDK_KEY_Insert:         return SDLK_INSERT;
		case GDK_KEY_Home:           return SDLK_HOME;
		case GDK_KEY_Page_Up:        return SDLK_PAGEUP;
		case GDK_KEY_Delete:         return SDLK_DELETE;
		case GDK_KEY_End:            return SDLK_END;
		case GDK_KEY_Page_Down:      return SDLK_PAGEDOWN;
		case GDK_KEY_Right:          return SDLK_RIGHT;
		case GDK_KEY_Left:           return SDLK_LEFT;
		case GDK_KEY_Down:           return SDLK_DOWN;
		case GDK_KEY_Up:             return SDLK_UP;

		case GDK_KEY_Num_Lock:       return SDLK_NUMLOCKCLEAR;
		case GDK_KEY_Clear:          return SDLK_NUMLOCKCLEAR;

		case GDK_KEY_KP_Divide:      return SDLK_KP_DIVIDE;
		case GDK_KEY_KP_Multiply:    return SDLK_KP_MULTIPLY;
		case GDK_KEY_KP_Subtract:    return SDLK_KP_MINUS;
		case GDK_KEY_KP_Add:         return SDLK_KP_PLUS;
		case GDK_KEY_KP_Enter:       return SDLK_KP_ENTER;
		case GDK_KEY_KP_Decimal:     return SDLK_KP_PERIOD;

		case GDK_KEY_KP_1:           return SDLK_KP_1;
		case GDK_KEY_KP_2:           return SDLK_KP_2;
		case GDK_KEY_KP_3:           return SDLK_KP_3;
		case GDK_KEY_KP_4:           return SDLK_KP_4;
		case GDK_KEY_KP_5:           return SDLK_KP_5;
		case GDK_KEY_KP_6:           return SDLK_KP_6;
		case GDK_KEY_KP_7:           return SDLK_KP_7;
		case GDK_KEY_KP_8:           return SDLK_KP_8;
		case GDK_KEY_KP_9:           return SDLK_KP_9;
		case GDK_KEY_KP_0:           return SDLK_KP_0;

		case GDK_KEY_Control_L:      return SDLK_LCTRL;
		case GDK_KEY_Shift_L:        return SDLK_LSHIFT;
		case GDK_KEY_Alt_L:          return SDLK_LALT;
		case GDK_KEY_Control_R:      return SDLK_RCTRL;
		case GDK_KEY_Shift_R:        return SDLK_RSHIFT;
		case GDK_KEY_Alt_R:          return SDLK_RALT;

		case GDK_KEY_Meta_L:         return SDLK_LGUI;
		case GDK_KEY_Super_L:        return SDLK_LGUI;
		case GDK_KEY_Meta_R:         return SDLK_RGUI;
		case GDK_KEY_Super_R:        return SDLK_RGUI;
		case GDK_KEY_Menu:           return SDLK_MENU;

		case GDK_KEY_parenleft:      return SDLK_LEFTPAREN;
		case GDK_KEY_parenright:     return SDLK_RIGHTPAREN;
		case GDK_KEY_braceleft:      return SDLK_KP_LEFTBRACE;
		case GDK_KEY_braceright:     return SDLK_KP_RIGHTBRACE;

		case GDK_KEY_less:           return SDLK_LESS;
		case GDK_KEY_greater:        return SDLK_GREATER;
		case GDK_KEY_bar:            return SDLK_KP_VERTICALBAR;
		case GDK_KEY_exclam:         return SDLK_EXCLAIM;
		case GDK_KEY_quotedbl:       return SDLK_QUOTEDBL;
		case GDK_KEY_numbersign:     return SDLK_HASH;
		case GDK_KEY_percent:        return SDLK_PERCENT;
		case GDK_KEY_dollar:         return SDLK_DOLLAR;
		case GDK_KEY_ampersand:      return SDLK_AMPERSAND;
		case GDK_KEY_asterisk:       return SDLK_ASTERISK;
		case GDK_KEY_question:       return SDLK_QUESTION;
		case GDK_KEY_at:             return SDLK_AT;
		case GDK_KEY_periodcentered: return SDLK_UNKNOWN;
		case GDK_KEY_caret:          return SDLK_CARET;
		case GDK_KEY_underscore:     return SDLK_UNDERSCORE;

		default:                     return SDLK_UNKNOWN;
	}
}

// https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html

gboolean Sdl2Area::eventWindowState(GtkWidget * widget, GdkEventWindowState * winstate, gpointer user_data) {
	//printf("W\n");
	return TRUE;
}

gboolean Sdl2Area::eventConfigure(GtkWidget * widget, GdkEventConfigure *config, gpointer user_data) {
	//printf("C\n");

	GApplication * app = G_APPLICATION(user_data);
	MainAppPrivateData * priv = MAIN_APP_GET_PRIVATE(app);
	SDL_SetWindowSize(priv->sdl_window, config->width, config->height);

	SDL_WindowEvent event;
	memset(&event, 0, sizeof(event));
	event.type = SDL_WINDOWEVENT;
	event.event = SDL_WINDOWEVENT_SIZE_CHANGED; // followed by SDL_WINDOWEVENT_RESIZED if size was changed by external event
	event.data1 = config->width; // width
	event.data2 = config->height; // height
	event.timestamp = SDL_GetTicks();
	SDL_PushEvent((SDL_Event*)&event);
	event.event = SDL_WINDOWEVENT_RESIZED; // always preceded by SDL_WINDOWEVENT_SIZE_CHANGED
	event.timestamp = SDL_GetTicks();
	SDL_PushEvent((SDL_Event*)&event);
	return TRUE;
}

gboolean Sdl2Area::eventMotion(GtkWidget *widget, GdkEventMotion *motion, gpointer user_data) {
	//printf("M\n");
	SDL_MouseMotionEvent event;
	memset(&event, 0, sizeof(event));
	event.type = SDL_MOUSEMOTION;
	event.x = motion->x;
	event.y = motion->y;
	event.xrel = 0;
	event.yrel = 0;
	event.state = 0;
	SDL_PushEvent((SDL_Event*)&event);
	return TRUE;
}

gboolean Sdl2Area::eventButton(GtkWidget *widget, GdkEventButton *button, gpointer user_data) {
	//printf("B\n");
	SDL_MouseButtonEvent event;
	memset(&event, 0, sizeof(event));
	switch (button->type) {
		case GDK_BUTTON_PRESS:
			gtk_widget_grab_focus(widget);
			event.type = SDL_MOUSEBUTTONDOWN;
			break;
		case GDK_BUTTON_RELEASE:
			event.type = SDL_MOUSEBUTTONUP;
			break;
		case GDK_2BUTTON_PRESS:
		case GDK_3BUTTON_PRESS:
		default:
			return TRUE;
	}
	switch (button->button) {
		case 1:
			event.button = SDL_BUTTON_LEFT;
			break;
		case 2:
			event.button = SDL_BUTTON_MIDDLE;
			break;
		case 3:
			event.button = SDL_BUTTON_RIGHT;
			break;
		case 4:
			event.button = SDL_BUTTON_X1;
			break;
		case 5:
			event.button = SDL_BUTTON_X2;
			break;
		default:
			return TRUE;
	}
	event.x = button->x;
	event.y = button->y;
	event.state = 0;
	SDL_PushEvent((SDL_Event*)&event);
	return TRUE;
}

gboolean Sdl2Area::eventScroll(GtkWidget *widget, GdkEventScroll *scroll, gpointer user_data) {
	//printf("S\n");
	SDL_MouseWheelEvent event;
	memset(&event, 0, sizeof(event));
	event.type = SDL_MOUSEWHEEL;
	event.which = 0;
	event.windowID = 0;
	event.x = scroll->x;
	event.y = scroll->y;
	event.timestamp = SDL_GetTicks();
	SDL_PushEvent((SDL_Event*)&event);
	return TRUE;
}

gboolean Sdl2Area::eventKey(GtkWidget *widget, GdkEventKey *key, gpointer user_data) {
	//printf("K\n");
	SDL_KeyboardEvent event;
	memset(&event, 0, sizeof(event));
	switch (key->type) {
		case GDK_KEY_PRESS:
			event.type = SDL_KEYDOWN;
			event.state = SDL_PRESSED;
			break;
		case GDK_KEY_RELEASE:
			event.type = SDL_KEYUP;
			event.state = SDL_RELEASED;
			break;
		default:
			return TRUE;
	}
	event.keysym.sym = GdkKeyToSdlKeycode(key->keyval); // SDL_SCANCODE_TO_KEYCODE(event.keysym.scancode);
	if (SDLK_UNKNOWN != event.keysym.sym) {
		event.timestamp = SDL_GetTicks();
		event.windowID = 0;
		event.repeat = (key->send_event == TRUE ? 0 : 1) ;
		event.keysym.scancode = SDL_GetScancodeFromKey(event.keysym.sym);
		event.keysym.mod = KMOD_NONE;
		SDL_PushEvent((SDL_Event*)&event);
	}
	return TRUE;
}

gboolean Sdl2Area::eventEnter(GtkWidget *widget, GdkEventCrossing *crossing, gpointer user_data) {
	//printf("E\n");
	return TRUE;
}

gboolean Sdl2Area::eventLeave(GtkWidget *widget, GdkEventCrossing *crossing, gpointer user_data) {
	//printf("L\n");
	return TRUE;
}

gboolean Sdl2Area::eventFocus(GtkWidget *widget, GdkEventFocus *focus, gpointer user_data) {
	//printf("F\n");
	return TRUE;
}

void Gamepad::onButtonDown(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context) {
	//MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(G_APPLICATION (context));
	//printf("Button %u down on device %u at %f with context %p\n", buttonID, device->deviceID, timestamp, context);
	SDL_JoyButtonEvent event;
	memset(&event, 0, sizeof(event));
	event.timestamp = SDL_GetTicks();
	event.type = SDL_JOYBUTTONDOWN;
	event.which = device->deviceID;
	event.button = buttonID;
	event.state = SDL_PRESSED;
	SDL_PushEvent((SDL_Event*)&event);
}

void Gamepad::onButtonUp(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context) {
	//MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(G_APPLICATION (context));
	//printf("Button %u up on device %u at %f with context %p\n", buttonID, device->deviceID, timestamp, context);
	SDL_JoyButtonEvent event;
	memset(&event, 0, sizeof(event));
	event.timestamp = SDL_GetTicks();
	event.type = SDL_JOYBUTTONUP;
	event.which = device->deviceID;
	event.button = buttonID;
	event.state = SDL_RELEASED;
	SDL_PushEvent((SDL_Event*)&event);
}

void Gamepad::onAxisMoved(struct Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp, void * context) {
	//MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(G_APPLICATION (context));
	//printf("Axis %u moved from %f to %f on device %u at %f with context %p\n", axisID, lastValue, value, device->deviceID, timestamp, context);
	SDL_JoyAxisEvent event;
	memset(&event, 0, sizeof(event));
	event.timestamp = SDL_GetTicks();
	event.type = SDL_JOYAXISMOTION;
	event.which = device->deviceID;
	event.axis = axisID;
	event.value = roundl(value * 32767);
	SDL_PushEvent((SDL_Event*)&event);
}

void Gamepad::onDeviceAttached(struct Gamepad_device * device, void * context) {
	//MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(G_APPLICATION (context));
	//printf("Device ID %u attached (vendor = 0x%X; product = 0x%X) with context %p\n", device->deviceID, device->vendorID, device->productID, context);
}

void Gamepad::onDeviceRemoved(struct Gamepad_device * device, void * context) {
	//MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(G_APPLICATION (context));
	//printf("Device ID %u removed with context %p\n", device->deviceID, context);
}

void Gamepad::initGamepad(void * context) {
	Gamepad_deviceAttachFunc(onDeviceAttached, context);
	Gamepad_deviceRemoveFunc(onDeviceRemoved, context);
	Gamepad_buttonDownFunc(onButtonDown, context);
	Gamepad_buttonUpFunc(onButtonUp, context);
	Gamepad_axisMoveFunc(onAxisMoved, context);
	Gamepad_init();
}

void Gamepad::shutdownGamepad() {
	Gamepad_shutdown();
}

void MainApp::setup(GApplication * app) {
	MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(app);

	GdkWindow *gdk_window = gtk_widget_get_window(priv->sdl_widget);
	Window x11_window = gdk_x11_window_get_xid(GDK_X11_WINDOW(gdk_window));
	priv->sdl_window = SDL_CreateWindowFrom((const void*)x11_window);

	const gchar * log_text = "Starting system...\n";
	GtkTextBuffer * log_buffer;
	// disable the text view while loading the buffer with the text
	gtk_widget_set_sensitive(priv->log_widget, FALSE);
	log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(priv->log_widget));
	gtk_text_buffer_set_text(log_buffer, log_text, -1);
	gtk_text_buffer_set_modified(log_buffer, FALSE);
	gtk_widget_set_sensitive(priv->log_widget, TRUE);

	Gamepad::initGamepad((void *)priv);

	priv->sdl_app->init(
		priv->sdl_window,
		gtk_widget_get_allocated_width(priv->sdl_widget),
		gtk_widget_get_allocated_height(priv->sdl_widget)
	);

	priv->idle_handler = g_timeout_add(1000/FPS, MainApp::draw, (gpointer)app);
}

void MainApp::cleanup(GtkApplication * app) {
	MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(app);

	if (NULL != priv->info_handler) {
		delete priv->info_handler;
		priv->info_handler = NULL;
	}

	priv->sdl_app->destroy();

	Gamepad::shutdownGamepad();
}

void MainApp::createFromFile(GApplication * app, GFile * file) {
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(app);

	builder = gtk_builder_new(); /* Load UI from file */
	if (!gtk_builder_add_from_file(builder, UI_FILE, &error)) {
		g_critical("Couldn't load builder file: %s", error->message);
		g_error_free(error);
	}

	/* Auto-connect signal handlers */
	gtk_builder_connect_signals(builder, app);

	/* Get the window object from the ui file */
	window = GTK_WIDGET(gtk_builder_get_object(builder, ID_TOP_WINDOW));
	if (!window) {
		g_critical("Widget \"%s\" is missing in file %s.",
			ID_TOP_WINDOW,
			UI_FILE);
	}

	priv->sdl_widget = GTK_WIDGET(gtk_builder_get_object(builder, ID_SDL_WIDGET));
	priv->log_widget = GTK_WIDGET(gtk_builder_get_object(builder, ID_LOG_WIDGET));

	global_log_widget = priv->log_widget;

	/* set the log view font */
	PangoFontDescription * font_desc = pango_font_description_from_string ("monospace 10");
	gtk_widget_override_font(priv->log_widget, font_desc);     
	pango_font_description_free (font_desc); 

	GtkWidget * sdl_widget = GTK_WIDGET(priv->sdl_widget);
	GObject   * sdl_object = G_OBJECT(priv->sdl_widget);
	GdkWindow * sdl_window = gtk_widget_get_window(priv->sdl_widget);

	// https://developer.gnome.org/gtk3/stable/GtkWidget.html

	if ( g_signal_connect(sdl_object, "window-state-event",   G_CALLBACK(Sdl2Area::eventWindowState), app) <= 0 )
		printf("Could not connect window-state-event signal.\n");
	if ( g_signal_connect(sdl_object, "configure-event",      G_CALLBACK (Sdl2Area::eventConfigure),   app) <= 0 )
		printf("Could not connect configure-event signal.\n");
	if ( g_signal_connect(sdl_object, "motion-notify-event",  G_CALLBACK (Sdl2Area::eventMotion),      app) <= 0 )
		printf("Could not connect motion-notify-event signal.\n");
	if ( g_signal_connect(sdl_object, "button-press-event",   G_CALLBACK (Sdl2Area::eventButton),      app) <= 0 )
		printf("Could not connect button-press-event signal.\n");
	if ( g_signal_connect(sdl_object, "button-release-event", G_CALLBACK (Sdl2Area::eventButton),      app) <= 0 )
		printf("Could not connect button-release-event signal.\n");
	if ( g_signal_connect(sdl_object, "scroll-event",         G_CALLBACK (Sdl2Area::eventScroll),      app) <= 0 )
		printf("Could not connect scroll-event signal.\n");
	if ( g_signal_connect(sdl_object, "key-press-event",      G_CALLBACK (Sdl2Area::eventKey),         app) <= 0 )
		printf("Could not connect key-press-event signal.\n");
	if ( g_signal_connect(sdl_object, "key-release-event",    G_CALLBACK (Sdl2Area::eventKey),         app) <= 0 )
		printf("Could not connect key-release-event signal.\n");
	if ( g_signal_connect(sdl_object, "enter-notify-event",   G_CALLBACK (Sdl2Area::eventEnter),       app) <= 0 )
		printf("Could not connect enter-notify-event signal.\n");
	if ( g_signal_connect(sdl_object, "leave-notify-event",   G_CALLBACK (Sdl2Area::eventLeave),       app) <= 0 )
		printf("Could not connect leave-notify-event signal.\n");
	if ( g_signal_connect(sdl_object, "focus-in-event",       G_CALLBACK (Sdl2Area::eventFocus),       app) <= 0 )
		printf("Could not connect focus-in-event signal.\n");
	if ( g_signal_connect(sdl_object, "focus-out-event",      G_CALLBACK (Sdl2Area::eventFocus),       app) <= 0 )
		printf("Could not connect focus-out-event signal.\n");

	// GdkEventMask is defined in /usr/include/gtk-3.0/gdk/gdktypes.h
	// http://www.linuxtopia.org/online_books/gui_toolkit_guides/gtk+_gnome_application_development/sec-gdkevent_1.html

	priv->info_handler = new InfoHandler(priv->sdl_app, window, builder);

	gtk_widget_add_events(sdl_widget, GDK_ALL_EVENTS_MASK);
	gtk_widget_set_can_focus(sdl_widget, true);
	gtk_widget_grab_focus(sdl_widget);

	// fail if we're not on X11, as SDL2 does not support
	// anything else (aka Wayland) on Linux AFAIK
	g_assert(GDK_IS_X11_WINDOW(sdl_window));

	MainApp::setup(app);

	g_object_unref(builder);

	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
	if (file != NULL) {
		/* TODO: Add code here to open the file in the new window */
	}
	gtk_widget_show_all(GTK_WIDGET(window));
}

void MainApp::activate(GApplication *application) {
	MainApp::createFromFile(application, NULL);
}

void MainApp::open(GApplication * application, GFile ** files, gint n_files, const gchar * hint ) {
	gint i;
	for (i = 0; i < n_files; i++) {
		MainApp::createFromFile(application, files[i]);
	}
}

void MainApp::finalize(GObject * object) {
	G_OBJECT_CLASS(main_app_parent_class)->finalize(object);
}

static void main_app_init(MainApp * object) {
	MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(object);
	priv->sdl_window = NULL;
	priv->idle_handler = 0;
}

static void main_app_class_init(MainAppClass *klass) {
	G_APPLICATION_CLASS(klass)->activate = MainApp::activate;
	G_APPLICATION_CLASS(klass)->open = MainApp::open;
	g_type_class_add_private(klass, sizeof(MainAppPrivateData));
	G_OBJECT_CLASS(klass)->finalize = MainApp::finalize;
}

MainApp * main_app_new(void) {
#if !GLIB_CHECK_VERSION(2,35,1)
	g_type_init();
#endif

	MainApp * result = (MainApp *)( g_object_new(main_app_get_type(), "application-id", "race.main_app", "flags", G_APPLICATION_HANDLES_OPEN, NULL) );
	g_signal_connect(G_OBJECT(result), "shutdown", G_CALLBACK(MainApp::cleanup), NULL);
	return result;
}

extern "C" {

int startMainGtk3App(int argc, char *argv[], ISdl2App * sdl_app) {
	MainApp *app;
	int status = EXIT_SUCCESS;
	app = main_app_new();

	MainAppPrivateData *priv = MAIN_APP_GET_PRIVATE(app);
	priv->sdl_app = sdl_app;

	status = g_application_run(G_APPLICATION(app), argc, argv);

	if (NULL != priv->sdl_app) {
		delete priv->sdl_app;
		priv->sdl_app = NULL;
	}

	g_object_unref(app);

	return status;
}

}

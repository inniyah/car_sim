#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "gamepad/Gamepad.h"
#include "shell/Shell.h"
#include <string.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#endif

bool onButtonDown(void * sender, const char * eventID, void * eventData, void * context) {
	struct Gamepad_buttonEvent * event;
	
	event = eventData;
	return true;
}

bool onButtonUp(void * sender, const char * eventID, void * eventData, void * context) {
	struct Gamepad_buttonEvent * event;
	
	event = eventData;
	return true;
}

bool onAxisMoved(void * sender, const char * eventID, void * eventData, void * context) {
	struct Gamepad_axisEvent * event;
	
	event = eventData;
	return true;
}

bool onDeviceAttached(void * sender, const char * eventID, void * eventData, void * context) {
	struct Gamepad_device * device;
	
	device = eventData;
	device->eventDispatcher->registerForEvent(device->eventDispatcher, GAMEPAD_EVENT_BUTTON_DOWN, onButtonDown, device);
	device->eventDispatcher->registerForEvent(device->eventDispatcher, GAMEPAD_EVENT_BUTTON_UP, onButtonUp, device);
	device->eventDispatcher->registerForEvent(device->eventDispatcher, GAMEPAD_EVENT_AXIS_MOVED, onAxisMoved, device);
	return true;
}

bool onDeviceRemoved(void * sender, const char * eventID, void * eventData, void * context) {
	return true;
}

const char * Target_getName() {
	return "Gamepad test harness";
}

static unsigned int windowWidth = 800, windowHeight = 600;

void Target_init(int argc, char ** argv) {
	Gamepad_init();
	Gamepad_eventDispatcher()->registerForEvent(Gamepad_eventDispatcher(), GAMEPAD_EVENT_DEVICE_ATTACHED, onDeviceAttached, NULL);
	Gamepad_eventDispatcher()->registerForEvent(Gamepad_eventDispatcher(), GAMEPAD_EVENT_DEVICE_REMOVED, onDeviceRemoved, NULL);
	
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	
	Shell_mainLoop();
}

static void drawGlutString(int rasterPosX, int rasterPosY, const char * string) {
	size_t length, charIndex;
	
	glRasterPos2i(rasterPosX, rasterPosY);
	length = strlen(string);
	for (charIndex = 0; charIndex < length; charIndex++) {
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string[charIndex]);
	}
}

void Target_draw() {
	unsigned int gamepadIndex;
	struct Gamepad_device * device;
	unsigned int axisIndex;
	unsigned int buttonIndex;
	float axisState;
	
	Gamepad_detectDevices();
	Gamepad_processEvents();
	
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	for (gamepadIndex = 0; gamepadIndex < Gamepad_numDevices(); gamepadIndex++) {
		glLoadIdentity();
		glTranslatef(5.0f, 20.0f + 60.0f * gamepadIndex, 0.0f);
		
		device = Gamepad_deviceAtIndex(gamepadIndex);
		
		glColor3f(0.0f, 0.0f, 0.0f);
		drawGlutString(0, 0, device->description);
		
		glPushMatrix();
		for (axisIndex = 0; axisIndex < device->numAxes; axisIndex++) {
			axisState = device->axisStates[axisIndex];
			
			glBegin(GL_QUADS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(2.0f, 5.0f);
			glVertex2f(48.0f, 5.0f);
			glVertex2f(48.0f, 15.0f);
			glVertex2f(2.0f, 15.0f);
			glColor3f(0.5f, 1.0f, 0.5f);
			glVertex2f(24.0f + axisState * 21, 6.0f);
			glVertex2f(26.0f + axisState * 21, 6.0f);
			glVertex2f(26.0f + axisState * 21, 14.0f);
			glVertex2f(24.0f + axisState * 21, 14.0f);
			glEnd();
			glTranslatef(50.0f, 0.0f, 0.0f);
		}
		glPopMatrix();
		glTranslatef(0.0f, 20.0f, 0.0f);
		
		for (buttonIndex = 0; buttonIndex < device->numButtons; buttonIndex++) {
			glBegin(GL_QUADS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(2.0f, 2.0f);
			glVertex2f(18.0f, 2.0f);
			glVertex2f(18.0f, 18.0f);
			glVertex2f(2.0f, 18.0f);
			if (device->buttonStates[buttonIndex]) {
				glColor3f(0.5f, 1.0f, 0.5f);
				glVertex2f(3.0f, 3.0f);
				glVertex2f(17.0f, 3.0f);
				glVertex2f(17.0f, 17.0f);
				glVertex2f(3.0f, 17.0f);
			}
			glEnd();
			glTranslatef(20.0f, 0.0f, 0.0f);
		}
	}
	
	Shell_redisplay();
}

void Target_keyDown(int charCode, int keyCode) {
}

void Target_keyUp(int charCode, int keyCode) {
}

void Target_mouseDown(int buttonNumber, float x, float y) {
}

void Target_mouseUp(int buttonNumber, float x, float y) {
}

void Target_mouseMoved(float x, float y) {
}

void Target_mouseDragged(int buttonMask, float x, float y) {
}

void Target_resized(int newWidth, int newHeight) {
	windowWidth = newWidth;
	windowHeight = newHeight;
	glViewport(0, 0, newWidth, newHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
}

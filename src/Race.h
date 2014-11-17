/*
 * Copyright (C) 2014  Miriam Ruiz <miriam@debian.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RACE_H_A71ADAE4_6CB3_11E4_93E0_10FEED04CD1C
#define RACE_H_A71ADAE4_6CB3_11E4_93E0_10FEED04CD1C

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.141592654
#endif

class Car {
	static const float COEFF = 1.;

public:

	int lastcheck, lap;
	int lapflag, crashflag;
	int color;

	void resetTimer() {
		time_ms = 0;
	}
	void updateTimer(unsigned int milliseconds) {
		time_ms += milliseconds;
	}
	void setSize(int w, int h) {
		width  = w;
		height = h;
	}
	void setPosition(float x, float y, float a) {
		x_pos = x;
		y_pos = y;
		angle = a;
	}
	void computeNewPosition(unsigned int milliseconds) {
		speed *= 0.995;
		x_pos -= cos(angle) * speed;
		y_pos -= sin(angle) * speed;
	}
	void turnLeft(float ch) {
		if (speed < 0) {
			angle += ch * COEFF;
		} else {
			angle -= ch * COEFF;
		}
		fixAngle();
	}
	void turnRight(float ch) {
		if (speed < 0) {
			angle -= ch * COEFF;
		} else {
			angle += ch * COEFF;
		}
		fixAngle();
	}
	void setSpeed(float s) {
		speed = s;
	}
	void incSpeed(float is) {
		speed += is * COEFF ;
	}
	void decSpeed(float ds) {
		speed -= ds * COEFF ;
	}
	void decSpeedByFactor(float fs) {
		speed -= speed * fs;
	}
	void backupPosition() {
		old_x = x_pos;
		old_y = y_pos;
		old_angle = angle;
	}
	void restorePosition() {
		x_pos = old_x;
		y_pos = old_y;
		angle = old_angle;
	}
	float getX() {
		return x_pos;
	}
	float getY() {
		return y_pos;
	}
	float getH() {
		return height;
	}
	float getW() {
		return width;
	}
	float getAngle() {
		return angle;
	}
	float getSpeed() {
		return speed;
	}
	unsigned int getTimer() {
		return time_ms;
	}

	void drawBrakeLights(SDL_Renderer * renderer);
	void drawReversingLights(SDL_Renderer * renderer);
	void drawWarningLights(SDL_Renderer * renderer);
	void drawPositionLights(SDL_Renderer * renderer);

private:
	void fixAngle() { // limit angle between 0 and 2*pi
		if ( angle < 0. ) {
			angle += 2. * M_PI;
		}
		if ( angle > 2. * M_PI ) {
			angle -= 2. * M_PI;
		}
	}

	static void drawRawLight(SDL_Renderer * renderer, int x, int y, int r);

	int width, height;

	float x_pos;
	float y_pos;
	float angle;
	float speed;

	float old_x;
	float old_y;
	float old_angle;

	unsigned int time_ms;
};

struct Track {
	const char *filename;
	int start_x;
	int start_y;
	int start_a;
	const char *name;
	const char *author;
};

class Race {
public:
	Race();
	~Race();

	bool draw();
	unsigned int update(unsigned int milliseconds);

	void setUp(SDL_Renderer * renderer);
	void startTrack(int id);

	bool eventHandlerKeyboard(SDL_Event & event);
	bool eventHandlerMouse(SDL_Event & event);
	bool eventHandlerJoystick(SDL_Event & event);
	bool eventHandlerUser(SDL_Event & event);

	bool getInfo(void * dest, unsigned int type, intptr_t param);

private:
	static const size_t MAXLINELENGTH = 80;
	static const int DELAY = 7;
	static const int NB_CARS = 16;
	static const int MAX_TRACKS = 16;

	static const int SCREEN_WIDTH  = 1024;
	static const int SCREEN_HEIGHT = 768;

	// SDL interprets each pixel as a 32-bit number, so our masks must depend on the endianness (byte order) of the machine
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	static const Uint32 RMASK = 0xff000000;
	static const Uint32 GMASK = 0x00ff0000;
	static const Uint32 BMASK = 0x0000ff00;
	static const Uint32 AMASK = 0x000000ff;
#else
	static const Uint32 RMASK = 0x000000ff;
	static const Uint32 GMASK = 0x0000ff00;
	static const Uint32 BMASK = 0x00ff0000;
	static const Uint32 AMASK = 0xff000000;
#endif

	SDL_Renderer * mxSdlRenderer;

	SDL_Texture * mpSdlTextureCircuit;
	SDL_Surface * mpSdlSurfaceCircuit;
	SDL_Surface * mpSdlSurfaceFunction;

	int miTrackId;
	static const Track track[MAX_TRACKS];

	int miCarId;
	Car car;
	bool show_tires;
	SDL_Surface * mpaSdlSurfaceCars[NB_CARS][256];

	bool mUpKey;
	bool mDownKey;
	bool mLeftKey;
	bool mRightKey;

	void generateCars();
	void moveCar(unsigned int milliseconds);
	void darkenTrack(SDL_Surface * surface, float coef = 0.3);
};

#endif // RACE_H_A71ADAE4_6CB3_11E4_93E0_10FEED04CD1C

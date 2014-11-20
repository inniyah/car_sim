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
#include <cmath>

#ifndef M_PI
#define M_PI 3.141592654
#endif

class Car {
public:

	int lastcheck, lap;
	int lapflag, crashflag;
	int color;

	Car() : position_lights(true) {
	}

	void resetTimer() {
		global_time_ms = 0;
		inc_time_ms    = 0;
	}
	void setSize(int l, int w) {
		length        = l;
		width         = w;
	}
	void setPosition(float x, float y, float azimut) {
		now.pos_x     = x;
		now.pos_y     = y;
		now.ang_yaw   = azimut;
	}
	void setZ(float z, float pitch, float roll) {
		now.pos_z     = z;
		now.ang_pitch = pitch;
		now.ang_roll  = roll;
	}
	void computeNewPosition(unsigned int milliseconds) {
		inertia_coef *= 0.995;
		now.pos_x -= cos(now.ang_yaw) * inertia_coef;
		now.pos_y -= sin(now.ang_yaw) * inertia_coef;
	}
	void incYaw(float ch) {
		now.ang_yaw += ch;
		fixAngles();
	}
	void turnLeft(float ch) {
		if (inertia_coef < 0) {
			now.ang_yaw += ch;
		} else {
			now.ang_yaw -= ch;
		}
		fixAngles();
	}
	void turnRight(float ch) {
		if (inertia_coef < 0) {
			now.ang_yaw -= ch;
		} else {
			now.ang_yaw += ch;
		}
		fixAngles();
	}
	void setInertiaCoef(float s) {
		inertia_coef = s;
	}
	void incInertiaCoef(float is) {
		inertia_coef += is ;
	}
	void decInertiaCoef(float ds) {
		inertia_coef -= ds ;
	}
	void decInertiaCoefByFactor(float fs) {
		inertia_coef -= inertia_coef * fs;
	}
	void backupPosition() {
		before = now;
	}
	void restorePosition() {
		now = before;
	}
	float getPosX() {
		return now.pos_x;
	}
	float getPosY() {
		return now.pos_y;
	}
	float getPosZ() {
		return now.pos_z;
	}
	float getSpeedX() {
		return now.spd_x;
	}
	float getSpeedY() {
		return now.spd_y;
	}
	float getSpeedZ() {
		return now.spd_z;
	}
	float getWidth() {
		return width;
	}
	float getLength() {
		return length;
	}
	float getYaw() {
		return now.ang_yaw;
	}
	float getPitch() {
		return now.ang_pitch;
	}
	float getRoll() {
		return now.ang_roll;
	}
	float getInertiaCoef() {
		return inertia_coef;
	}
	unsigned int getTimer() {
		return global_time_ms;
	}

	void updateTimer(unsigned int milliseconds);
	void drawBrakeLights(SDL_Renderer * renderer);
	void drawReversingLights(SDL_Renderer * renderer);
	void drawWarningLights(SDL_Renderer * renderer);
	void drawPositionLights(SDL_Renderer * renderer);

private:
	void fixAngles() { // limit angle between 0 and 2*pi
		if ( now.ang_yaw < 0. ) {
			now.ang_yaw += 2. * M_PI;
		}
		if ( now.ang_yaw > 2. * M_PI ) {
			now.ang_yaw -= 2. * M_PI;
		}
	}

	static void drawRawLight(SDL_Renderer * renderer, int x, int y, int r);

	int length;
	int width;

	struct State {
		float pos_x;
		float pos_y;
		float pos_z;
		float spd_x;
		float spd_y;
		float spd_z;
		float acc_x;
		float acc_y;
		float acc_z;

		float ang_yaw; // rotation around vertical axis
		float ang_pitch; // rotation around axis orthogonal to movement
		float ang_roll; // rotation around axis in direction of movement

		State() :
			pos_x(0),
			pos_y(0),
			pos_z(0),
			spd_x(0),
			spd_y(0),
			spd_z(0),
			acc_x(0),
			acc_y(0),
			acc_z(0),
			ang_yaw(0),
			ang_pitch(0),
			ang_roll(0)
		{
		}
	};

	State now;
	State before;

	float inertia_coef;
	bool position_lights;
	unsigned int global_time_ms;
	unsigned int inc_time_ms;
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

	static const float XY_UNIT_TO_M = 0.01;
	static const float  Z_UNIT_TO_M = 0.01;

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
	bool mSdlSurfaceFunctionIsDirty;

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

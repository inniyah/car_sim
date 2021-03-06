/*
 * Copyright (C) 2014  Miriam Ruiz <miriam@debian.org>
 * 
 * Based on zeRace 0.7 ('a funny retro racing game')
 * http://royale.zerezo.com/zerace/
 * 
 * Copyright (C) 2004  Antoine Jacquet <royale@zerezo.com>
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

#include "Race.h"
#include "InfoTypes.h"
#include "Common.h"

#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>
#include <sys/stat.h>

const Track Race::track[] = {
	{ "car",      450, 655, 180,   "Car",                            "ICFP Programming Contest" },
	{ "first",    435, 215, 180,   "First circuit for this game...", "Royale" },
	{ "icy",      435, 215, 180,   "Same as First, but in winter!",  "Royale" },
	{ "hairpins", 505, 665, 0,     "Hairpins",                       "ICFP Programming Contest" },

	{ "simple",   585, 565, 0,     "Simple",                         "ICFP Programming Contest" },
	{ "loop",     678, 686, 0,     "Loop",                           "Royale" },
	{ "bio",      930, 500, 270,   "Bio",                            "Jujucece" },
	{ "city",     106, 358, 90,    "City",                           "Jujucece" },

	{ "desert",   680, 487, 80,    "Desert",                         "Jujucece" },
	{ "http",     520, 70, 180,    "HTTP"                            "Jujucece" },
	{ "kart",     370, 725, 0,     "Kart",                           "Jujucece" },
	{ "wave",     630, 380, 180,   "Wave",                           "Jujucece" },

	{ "wave2",     630, 380, 180,  "Wave 2",                         "Miriam" },
	{ "formula",  350, 330, 220,   "Formula",                        "Ju" },
	{ NULL,       0,   0,   0,     "",                               "" },
	{ NULL,       0,   0,   0,     "",                               "" },
};

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static Uint32 sdlGetPixel(SDL_Surface *surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to retrieve
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			return *p;
		case 2:
			return *(Uint16 *)p;
		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
		case 4:
			return *(Uint32 *)p;
		default:
			return 0; // shouldn't happen, but avoids warnings
    }
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
static void sdlPutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to set
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			*p = pixel;
			break;
		case 2:
			*(Uint16 *)p = pixel;
			break;
		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			} else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;
		case 4:
			*(Uint32 *)p = pixel;
			break;
		default:
			return; // shouldn't happen, but avoids warnings
	}
}

Race::Race() :
	mxSdlRenderer(NULL),
	mpSdlTextureCircuit(NULL),
	mpSdlSurfaceCircuit(NULL),
	mpSdlSurfaceFunction(NULL),
	mSdlSurfaceFunctionIsDirty(true),
	miTrackId(0),
	miCarId(0),
	show_tires(true),
	mLeftRightJoyAxis(0),
	mUpDownJoyAxis(0),
	mUpKey(false),
	mDownKey(false),
	mLeftKey(false),
	mRightKey(false)
{
	generateCars();
}

Race::~Race() {
	if (NULL != mpSdlTextureCircuit) {
		SDL_DestroyTexture(mpSdlTextureCircuit);
		mpSdlTextureCircuit = NULL;
	}
	if (NULL != mpSdlSurfaceCircuit) {
		SDL_FreeSurface(mpSdlSurfaceCircuit);
		mpSdlSurfaceCircuit = NULL;
	}
	if (NULL != mpSdlSurfaceFunction) {
		SDL_FreeSurface(mpSdlSurfaceFunction);
		mpSdlSurfaceFunction = NULL;
	}
}

void Race::setUp(SDL_Renderer * renderer) {
	mxSdlRenderer = renderer;
}

// load the car sprite and rotate it for every angles
void Race::generateCars() {
	int i,j;
	SDL_Surface *car;
	char temp[20]="sprites/carX.png";
	for (i = 0; i < NB_CARS; i++) {
		temp[11]='A'+i;
		car = IMG_Load(temp); // load the car sprite
		for (j=0;j<256;j++) { // and rotate it for all available angles
			float x,y;
			float tcos,tsin;
			if ((mpaSdlSurfaceCars[i][j]=SDL_CreateRGBSurface(SDL_SWSURFACE, 30, 30, 32, RMASK, GMASK, BMASK, AMASK))==NULL) {
				fprintf(stderr,"CreateRGBSurface failed: %s\n",SDL_GetError());
				exit(1);
			};
			tcos=cos(2*M_PI*j/256);
			tsin=sin(2*M_PI*j/256);
			for (x=0;x<mpaSdlSurfaceCars[i][j]->w;x++) for (y=0;y<mpaSdlSurfaceCars[i][j]->h;y++) {
				int x2,y2;
				x2 = (x - mpaSdlSurfaceCars[i][j]->w/2.0) * tcos + (y - mpaSdlSurfaceCars[i][j]->h/2.0) * tsin + car->w/2.0;
				y2 = (x - mpaSdlSurfaceCars[i][j]->w/2.0) * tsin - (y - mpaSdlSurfaceCars[i][j]->h/2.0) * tcos + car->h/2.0;
				if (x2>0 && x2<car->w && y2>0 && y2<car->h)
				sdlPutPixel(mpaSdlSurfaceCars[i][j], x, y, sdlGetPixel(car,x2,y2));
			}
		}
		SDL_FreeSurface(car);
	}
}

void Race::darkenTrack(SDL_Surface *surface, float coef) {
	SDL_Rect pos;
	for (pos.x = 0; pos.x < surface->w; pos.x++) {
		for (pos.y = 0; pos.y < surface->h; pos.y++) {
			Uint32 c;
			Uint8 r,g,b;
			c = sdlGetPixel(surface, pos.x, pos.y);
			SDL_GetRGB(c, surface->format, &r, &g, &b);
			r *= coef;
			g *= coef;
			b *= coef;
			sdlPutPixel(surface, pos.x, pos.y, SDL_MapRGB(surface->format, r, g, b));
		}
	}
}

void Race::startTrack(int id) {
	if (id >= 0 && id < MAX_TRACKS) {
		miTrackId = id;
	}
	char circname[128];
	sprintf(circname, "tracks/%s.png", track[miTrackId].filename);
	mpSdlSurfaceCircuit = IMG_Load(circname);

	char funcname[128];
	sprintf(funcname, "tracks/%s_function.png", track[miTrackId].filename);
	mpSdlSurfaceFunction = IMG_Load(funcname);

	for (int x = 0; x < mpSdlSurfaceCircuit->w; ++x) {
		Uint8 prev_b = 0;
		for (int y = 0; y < mpSdlSurfaceCircuit->h; ++y) {
			Uint32 c = sdlGetPixel(mpSdlSurfaceFunction, x, y);
			Uint8 r, g, b;
			SDL_GetRGB(c, mpSdlSurfaceFunction->format, &r, &g, &b);
			if (0 != y) {
				if ( (prev_b / 4) != (b / 4) ) {
					sdlPutPixel(mpSdlSurfaceCircuit, x, y, SDL_MapRGB(mpSdlSurfaceCircuit->format, b, b, b));
				}
			}
			prev_b = b;
		}
	}

	for (int y = 0; y < mpSdlSurfaceCircuit->h; ++y) {
		Uint8 prev_b = 0;
		for (int x = 0; x < mpSdlSurfaceCircuit->w; ++x) {
			Uint32 c = sdlGetPixel(mpSdlSurfaceFunction, x, y);
			Uint8 r, g, b;
			SDL_GetRGB(c, mpSdlSurfaceFunction->format, &r, &g, &b);
			if (0 != x) {
				if ( (prev_b / 4) != (b / 4) ) {
					sdlPutPixel(mpSdlSurfaceCircuit, x, y, SDL_MapRGB(mpSdlSurfaceCircuit->format, b, b, b));
				}
			}
			prev_b = b;
		}
	}

	mpSdlTextureCircuit = SDL_CreateTextureFromSurface(mxSdlRenderer, mpSdlSurfaceCircuit);

	mLeftRightJoyAxis = 0;
	mUpDownJoyAxis = 0;

	mUpKey = false;
	mDownKey = false;
	mLeftKey = false;
	mRightKey = false;

	car.setSize( mpaSdlSurfaceCars[0][0]->w, mpaSdlSurfaceCars[0][0]->h);
	car.setPosition( track[miTrackId].start_x, track[miTrackId].start_y, track[miTrackId].start_a * 2. * M_PI / 360. );
	car.setInertiaCoef(0);
	car.resetTimer();
	car.backupPosition();

	car.cleanCheckpoints();
	car.lapflag = 0;
}

void Car::updateTimer(unsigned int milliseconds) {
	inc_time_ms     = milliseconds;
	global_time_ms += milliseconds;
	float elapsed_time_s = inc_time_ms / 1000.0;
	now.spd_x = (now.pos_x - before.pos_x) / elapsed_time_s;
	now.spd_y = (now.pos_y - before.pos_y) / elapsed_time_s;
	now.spd_z = (now.pos_z - before.pos_z) / elapsed_time_s;
	now.acc_x = (now.spd_x - before.spd_x) / elapsed_time_s;
	now.acc_y = (now.spd_y - before.spd_y) / elapsed_time_s;
	now.acc_z = (now.spd_z - before.spd_z) / elapsed_time_s;
}

void Car::drawRawLight(SDL_Renderer * renderer, int x, int y, int r) {
	SDL_RenderDrawPoint(renderer, x, y);
	if (r>1) {
		SDL_RenderDrawPoint(renderer, x-1, y   );
		SDL_RenderDrawPoint(renderer, x+1, y   );
		SDL_RenderDrawPoint(renderer, x,   y-1 );
		SDL_RenderDrawPoint(renderer, x,   y+1 );
	}
	if (r>2) {
		SDL_RenderDrawPoint(renderer, x-2, y   );
		SDL_RenderDrawPoint(renderer, x+2, y   );
		SDL_RenderDrawPoint(renderer, x,   y-2 );
		SDL_RenderDrawPoint(renderer, x,   y+2 );
		SDL_RenderDrawPoint(renderer, x-1, y-1 );
		SDL_RenderDrawPoint(renderer, x-1, y+1 );
		SDL_RenderDrawPoint(renderer, x+1, y-1 );
		SDL_RenderDrawPoint(renderer, x+1, y+1 );
	}
}

void Car::drawBrakeLights(SDL_Renderer * renderer) {
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
	drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 - sin(now.ang_yaw)*4, now.pos_y + sin(now.ang_yaw) * width/3 + cos(now.ang_yaw)*4, 3);
	drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 + sin(now.ang_yaw)*4, now.pos_y + sin(now.ang_yaw) * width/3 - cos(now.ang_yaw)*4, 3);
}

void Car::drawReversingLights(SDL_Renderer * renderer) {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
	drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 - sin(now.ang_yaw)*4, now.pos_y + sin(now.ang_yaw) * width/3 + cos(now.ang_yaw)*4, 3);
	drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 + sin(now.ang_yaw)*4, now.pos_y + sin(now.ang_yaw) * width/3 - cos(now.ang_yaw)*4, 3);
}

void Car::drawWarningLights(SDL_Renderer * renderer) {
	SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255); // Orange
	drawRawLight(renderer, now.pos_x - cos(now.ang_yaw) * length/3 - sin(now.ang_yaw)*5, now.pos_y - sin(now.ang_yaw) * width/3 + cos(now.ang_yaw)*5, 2);
	drawRawLight(renderer, now.pos_x - cos(now.ang_yaw) * length/3 + sin(now.ang_yaw)*5, now.pos_y - sin(now.ang_yaw) * width/3 - cos(now.ang_yaw)*5, 2);
	drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 - sin(now.ang_yaw)*5, now.pos_y + sin(now.ang_yaw) * width/3 + cos(now.ang_yaw)*5, 2);
	drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 + sin(now.ang_yaw)*5, now.pos_y + sin(now.ang_yaw) * width/3 - cos(now.ang_yaw)*5, 2);
}

void Car::drawPositionLights(SDL_Renderer * renderer) {
	if (position_lights) {
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
		drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 - sin(now.ang_yaw)*3, now.pos_y + sin(now.ang_yaw) * width/3 + cos(now.ang_yaw)*4, 2);
		drawRawLight(renderer, now.pos_x + cos(now.ang_yaw) * length/3 + sin(now.ang_yaw)*3, now.pos_y + sin(now.ang_yaw) * width/3 - cos(now.ang_yaw)*4, 2);
		SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255); // Yellow
		drawRawLight(renderer, now.pos_x - cos(now.ang_yaw) * length/3 - sin(now.ang_yaw)*4, now.pos_y - sin(now.ang_yaw) * width/3 + cos(now.ang_yaw)*4, 3);
		drawRawLight(renderer, now.pos_x - cos(now.ang_yaw) * length/3 + sin(now.ang_yaw)*4, now.pos_y - sin(now.ang_yaw) * width/3 - cos(now.ang_yaw)*4, 3);
	}
}

bool Race::draw() {
	if (mSdlSurfaceFunctionIsDirty) {
		if (NULL != mpSdlTextureCircuit) {
			SDL_DestroyTexture(mpSdlTextureCircuit);
			mpSdlTextureCircuit = NULL;
		}
		mpSdlTextureCircuit = SDL_CreateTextureFromSurface(mxSdlRenderer, mpSdlSurfaceCircuit);
		mSdlSurfaceFunctionIsDirty = false;
	}

	SDL_Rect circ_rect;
	circ_rect.w = mpSdlSurfaceCircuit->w;
	circ_rect.h = mpSdlSurfaceCircuit->h;
	circ_rect.x = 0;
	circ_rect.y = 0;

	SDL_RenderClear(mxSdlRenderer);
	SDL_RenderCopy(mxSdlRenderer, mpSdlTextureCircuit, NULL, &circ_rect);

	SDL_Rect car_rect;
	car_rect.x = car.getPosX() - car.getLength()/2;
	car_rect.y = car.getPosY() - car.getWidth()/2;
	car_rect.w = car.getLength();
	car_rect.h = car.getWidth();

	unsigned char car_angle = (unsigned char)(256 * car.getYaw() / 2.0 / M_PI) % 256;
	SDL_Texture  * car_texture = SDL_CreateTextureFromSurface(mxSdlRenderer, mpaSdlSurfaceCars[miCarId][car_angle]);
	SDL_RenderCopy(mxSdlRenderer, car_texture, NULL, &car_rect);
	if (NULL != car_texture) {
		SDL_DestroyTexture (car_texture);
		car_texture = NULL;
	}

	if ( true ) {
		car.drawPositionLights(mxSdlRenderer);
	}

	if ( (mUpDownJoyAxis > JOY_AXIS_BRAKE_THRESHOLD) && car.getInertiaCoef() > 0.1 ) {
		car.drawBrakeLights(mxSdlRenderer);
	}

	if ( car.getInertiaCoef() < -0.1 ) {
		car.drawReversingLights(mxSdlRenderer);
	}

	if ( car.getInertiaCoef() >= -0.1 && car.getInertiaCoef() <= 0.1 && (SDL_GetTicks() % 800) > 400 ) {
		car.drawWarningLights(mxSdlRenderer);
	}

	SDL_RenderPresent(mxSdlRenderer);
	SDL_SetRenderDrawColor(mxSdlRenderer, 0, 0, 0, 0);

	return true;
}

void Car::updateCheckpoints(int chkpnt) {
	// if we are on the next checkpoint, validate it
	if (chkpnt == last_checkpoint + 1) {
		if (lapflag==3) { // If we validate a missed checkpoint
			lapflag=4;
		}
		++last_checkpoint;
	}

	// if we missed a checkpoint
	if ((chkpnt > last_checkpoint + 1) && (last_checkpoint != 0)) {
		lapflag = 3;
	}

	// if we validate all and start over, we complete a turn
	if (chkpnt == 0 && last_checkpoint == 31) { // reset turn variables
		last_checkpoint = 0;
		++lap;
		lapflag = 1;
	}

	// if we are at the start but not each checkpoint validate, it's an incomplete lap
	if (chkpnt == 0 && chkpnt !=0 && last_checkpoint != 31 && last_checkpoint > 0) {
		last_checkpoint = 0;
		lapflag = 2;
	}

	current_checkpoint = chkpnt;
}

void Race::moveCar(unsigned int milliseconds) {
	Uint32 c;

	// reset flags
	car.crashflag=0;

	float center_x = car.getPosX();
	float center_y = car.getPosY();
	Uint8 center_r, center_g, center_b;

	// get the pixel color under the center of car in the function map
	c = sdlGetPixel(mpSdlSurfaceFunction, center_x, center_y);

	// red layer = checkpoints; green layer = road quality; blue = map height
	SDL_GetRGB(c, mpSdlSurfaceFunction->format, &center_r, &center_g, &center_b);

	float angle  = car.getYaw();
	float length = car.getLength();
	float width  = car.getWidth();

	float left_back_x = center_x + cos(angle) * length/3 - sin(angle)*3;
	float left_back_y = center_y + sin(angle) * width/3 + cos(angle)*4;
	c = sdlGetPixel(mpSdlSurfaceFunction, left_back_x, left_back_y);
	Uint8 left_back_r, left_back_g, left_back_b;
	SDL_GetRGB(c, mpSdlSurfaceFunction->format, &left_back_r, &left_back_g, &left_back_b);

	float right_back_x = center_x + cos(angle) * length/3 + sin(angle)*3;
	float right_back_y = center_y + sin(angle) * width/3 - cos(angle)*4;
	c = sdlGetPixel(mpSdlSurfaceFunction, right_back_x, right_back_y);
	Uint8 right_back_r, right_back_g, right_back_b;
	SDL_GetRGB(c, mpSdlSurfaceFunction->format, &right_back_r, &right_back_g, &right_back_b);

	float left_front_x = center_x - cos(angle) * length/3 - sin(angle)*4;
	float left_front_y = center_y - sin(angle) * width/3 + cos(angle)*4;
	c = sdlGetPixel(mpSdlSurfaceFunction, left_front_x, left_front_y);
	Uint8 left_front_r, left_front_g, left_front_b;
	SDL_GetRGB(c, mpSdlSurfaceFunction->format, &left_front_r, &left_front_g, &left_front_b);

	float right_front_x = center_x - cos(angle) * length/3 + sin(angle)*4;
	float right_front_y = center_y - sin(angle) * width/3 - cos(angle)*4;
	c = sdlGetPixel(mpSdlSurfaceFunction, right_front_x, right_front_y);
	Uint8 right_front_r, right_front_g, right_front_b;
	SDL_GetRGB(c, mpSdlSurfaceFunction->format, &right_front_r, &right_front_g, &right_front_b);

	float pitch_m = ( ( left_front_b + right_front_b - left_back_b - right_back_b ) * Z_UNIT_TO_M ) / ( ( 2.0 * length ) * XY_UNIT_TO_M );
	float roll_m  = ( ( left_front_b + left_back_b - right_front_b - right_back_b)  * Z_UNIT_TO_M ) / ( ( 2.0 * width) * XY_UNIT_TO_M );
	float pitch   = atan( pitch_m );
	float roll    = atan( roll_m );

	car.setZ(center_b, pitch, roll);

	car.incYaw( roll_m * car.getInertiaCoef() * 0.05 );
	car.incInertiaCoef( -pitch_m * 0.01 );

	if (mUpDownJoyAxis < -JOY_AXIS_MIN_THRESHOLD) {
		car.incInertiaCoef( (-mUpDownJoyAxis) * 0.01 * 2. );
	}
	if (mUpDownJoyAxis > JOY_AXIS_MIN_THRESHOLD) {
		car.decInertiaCoef( mUpDownJoyAxis * 0.01 );
	}
	if (mLeftRightJoyAxis < -JOY_AXIS_MIN_THRESHOLD) {
		car.turnLeft( (-mLeftRightJoyAxis) * 0.02 );
	}
	if (mLeftRightJoyAxis > JOY_AXIS_MIN_THRESHOLD) {
		car.turnRight( mLeftRightJoyAxis * 0.02 );
	}

	// update the inertia_coef depending on the road quality
	float average_g = ( left_back_g + right_back_g + left_front_g + right_front_g ) / 4.0 ;
	car.decInertiaCoefByFactor( (255 - average_g) / 1000. );

	// if it is a wall we move back to the last position
	if ( 0 == center_g || 0 == left_back_g || 0 == right_back_g || 0 == left_front_g || 0 == right_front_g ) {
		car.restorePosition();
		car.crashflag=1;
	}

	// save the old position and compute the new one
	car.backupPosition();
	car.computeNewPosition(milliseconds);

	// collision with the border of the screen
	float radius = ( car.getWidth() < car.getLength() ? car.getLength() : car.getWidth() ) / 2.0;
	if (
		car.getPosX() < radius ||
		car.getPosX() > mpSdlSurfaceFunction->w - radius ||
		car.getPosY() < radius ||
		car.getPosY() > mpSdlSurfaceFunction->h - radius
	) {
		car.restorePosition();
		car.setInertiaCoef(0);
		car.crashflag = 1;
	}

	car.updateCheckpoints(center_r/8);

	if (
		( car.getInertiaCoef()>0.5 && (mUpDownJoyAxis > JOY_AXIS_BRAKE_THRESHOLD) ) ||
		( car.getInertiaCoef()>2.0 && !(mUpDownJoyAxis < -JOY_AXIS_MIN_THRESHOLD) )
	) { // if the car is fast or braking, it slides
		if (show_tires) { // display tires slide

			float x = car.getPosX();
			float y = car.getPosY();
			float w = car.getLength();
			float h = car.getWidth();
			float angle = car.getYaw();
			float cos_a = cos(angle);
			float sin_a = sin(angle);

			sdlPutPixel( mpSdlSurfaceCircuit,
				x + cos_a * w/3 - sin_a*4,
				y + sin_a * h/3 + cos_a*4,
				SDL_MapRGB(mpSdlSurfaceCircuit->format,0,0,0) // black
			);
			sdlPutPixel(mpSdlSurfaceCircuit,
				x + cos_a * w/3 + sin_a*4,
				y + sin_a * h/3 - cos_a*4,
				SDL_MapRGB(mpSdlSurfaceCircuit->format,0,0,0) // black
			);
			if (mUpDownJoyAxis > JOY_AXIS_BRAKE_THRESHOLD) { // if we are braking the slide is larger
				sdlPutPixel(mpSdlSurfaceCircuit,
					x + cos_a * w/3 - sin_a*3,
					y + sin_a * h/3 + cos_a*3,
					SDL_MapRGB(mpSdlSurfaceCircuit->format,0,0,0) // black
				);
				sdlPutPixel(mpSdlSurfaceCircuit,
					x + cos_a * w/3 + sin_a*3,
					y + sin_a * h/3 - cos_a*3,
					SDL_MapRGB(mpSdlSurfaceCircuit->format,0,0,0) // black
				);
			}
			mSdlSurfaceFunctionIsDirty = true;
		}
	}

	car.updateTimer(milliseconds);
}

unsigned int Race::update(unsigned int milliseconds) {
	while ( milliseconds > 8 ) {
		moveCar(8);
		switch (car.lapflag) {
			case 1: // if we completed a lap
				printInfoLog("Lap Complete");
				car.lapflag=0;
				break;
			case 2: // if we completed an incomplete lap
				printInfoLog("Last Lap Canceled");
				car.lapflag=0;
				break;
			case 3: // if we miss a checkpoint
				printInfoLog("Checkpoint missed!");
				break;
			case 4: // if we validate a missed checkpoint
				printInfoLog("Checkpoint missed OK");
				break;
			default: // nothing
				break;
		}
		milliseconds -= 8;
	}
	return milliseconds;
}

static void printModifiers(Uint16 mod) {
	printf( "Modifers: " );

	if( mod == KMOD_NONE ) { // If there are none then say so and return
		printf( "None\n" );
		return;
	}

	if( mod & KMOD_NUM )    printf( "NUMLOCK " );
	if( mod & KMOD_CAPS )   printf( "CAPSLOCK " );
	if( mod & KMOD_LCTRL )  printf( "LCTRL " );
	if( mod & KMOD_RCTRL )  printf( "RCTRL " );
	if( mod & KMOD_RSHIFT ) printf( "RSHIFT " );
	if( mod & KMOD_LSHIFT ) printf( "LSHIFT " );
	if( mod & KMOD_RALT )   printf( "RALT " );
	if( mod & KMOD_LALT )   printf( "LALT " );
	if( mod & KMOD_CTRL )   printf( "CTRL " );
	if( mod & KMOD_SHIFT )  printf( "SHIFT " );
	if( mod & KMOD_ALT )    printf( "ALT " );
	printf( "\n" );
}

static void printKeyEventInfo( SDL_KeyboardEvent & key_event ) {
	if( key_event.type == SDL_KEYUP ) { // Is it a key release or a key press?
		printf( "Release: " );
	} else {
		printf( "Press: " );
	}

	printf( "Scancode: 0x%02X", key_event.keysym.scancode );
	printf( ", Name: %s", SDL_GetKeyName( key_event.keysym.sym ) );
	printf( "\n" );

	printModifiers(key_event.keysym.mod);
}

bool Race::eventHandlerKeyboard(SDL_Event & event) {
	//printf("Key\n");
	//printKeyEventInfo(*(SDL_KeyboardEvent*)(&event));
	switch (event.type) {
		case SDL_KEYDOWN: {
			switch (event.key.keysym.sym) {
				case SDLK_UP:
					mUpKey = true;
					break;
				case SDLK_DOWN:
					mDownKey = true;
					break;
				case SDLK_LEFT:
					mLeftKey = true;
					break;
				case SDLK_RIGHT:
					mRightKey = true;
					break;
				default:
					return false;
			}
			mUpDownJoyAxis    = ( mDownKey  ? 1.0 : 0.0 ) - ( mUpKey   ? 1.0 : 0.0 );
			mLeftRightJoyAxis = ( mRightKey ? 1.0 : 0.0 ) - ( mLeftKey ? 1.0 : 0.0 );
			return true;
		}
		case SDL_KEYUP: {
			switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					car.togglePositionLights();
					break;
				case SDLK_UP:
					mUpKey = false;
					break;
				case SDLK_DOWN:
					mDownKey = false;
					break;
				case SDLK_LEFT:
					mLeftKey = false;
					break;
				case SDLK_RIGHT:
					mRightKey = false;
					break;
				default:
					return false;
			}
			mUpDownJoyAxis    = ( mDownKey  ? 1.0 : 0.0 ) - ( mUpKey   ? 1.0 : 0.0 );
			mLeftRightJoyAxis = ( mRightKey ? 1.0 : 0.0 ) - ( mLeftKey ? 1.0 : 0.0 );
			return true;
		}
		default:
			return false;
	}
	return true;
}

bool Race::eventHandlerMouse(SDL_Event & event) {
	switch(event.type) {
		case SDL_MOUSEMOTION: {
			//printf("MOTION: x: %u, y: %u, xrel:%d, yrel: %d\n",
			//	event.motion.x,
			//	event.motion.y,
			//	event.motion.xrel,
			//	event.motion.yrel
			//);
			return true;
		}

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			//printf("BUTTON: button: %d, state: %s, x: %u, y: %u\n",
			//	event.button.button,
			//	SDL_MOUSEBUTTONDOWN == event.type ? "Down" : "Up",
			//	event.button.x,
			//	event.button.y
			//);
			return true;
		}

		case SDL_MOUSEWHEEL: {
			//printf("WHEEL: which: %d, x: %u, y: %u\n",
			//	event.wheel.which,
			//	event.wheel.x,
			//	event.wheel.y
			//);
			return true;
		}

		default:
			return false;
	}
}

bool Race::eventHandlerJoystick(SDL_Event & event) {
	switch(event.type) {
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP: {
			printf("JOY%d.BUTTON%d: %s\n", event.button.which, event.button.button, (event.button.state == SDL_PRESSED) ? "DOWN" : "UP");
			return true;
		}

		case SDL_JOYAXISMOTION: {
			printf("JOY%d.AXIS%d: %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
			if (/*0 == event.jaxis.axis ||*/ 3 == event.jaxis.axis) {
				mLeftRightJoyAxis = event.jaxis.value / 32767.0;
				if (mLeftRightJoyAxis < -1.0) {
					mLeftRightJoyAxis = -1.0;
				}
				if (mLeftRightJoyAxis > 1.0) {
					mLeftRightJoyAxis = 1.0;
				}
			}
			if (1 == event.jaxis.axis /*|| 2 == event.jaxis.axis*/) {
				mUpDownJoyAxis = event.jaxis.value / 32767.0;
				if (mUpDownJoyAxis < -1.0) {
					mUpDownJoyAxis = -1.0;
				}
				if (mUpDownJoyAxis > 1.0) {
					mUpDownJoyAxis = 1.0;
				}
			}
			return true;
		}

		case SDL_JOYHATMOTION: {
			printf("JOY%d.PAD%d: %d\n", event.jhat.which, event.jhat.hat, event.jhat.value);
			return true;
		}

		case SDL_JOYBALLMOTION: {
			printf("JOY%d.BALL%d: %d, %d\n", event.jball.which, event.jball.ball, event.jball.xrel, event.jball.yrel);
			return true;
		}

		default:
			return false;
	}
}

bool Race::eventHandlerUser(SDL_Event & event) {
	switch(event.type) {
		case SDL_USEREVENT: {
			//printf("USR%d\n", event.user.code);
			return true;
		}

		default:
			return false;
	}
}

bool Race::getInfo(void * dest, unsigned int type, intptr_t param) {
	switch (type) {
		case INFO_NONE: {
			return true;
		}
		case INFO_POSITION_3F: {
			float * f = (float*)dest;
			f[0] = car.getPosX() * XY_UNIT_TO_M;
			f[1] = car.getPosY() * XY_UNIT_TO_M;
			f[2] = car.getPosZ() * Z_UNIT_TO_M;
			return true;
		}
		case INFO_SPEED_3F: {
			float * f = (float*)dest;
			f[0] = car.getSpeedX() * XY_UNIT_TO_M;
			f[1] = car.getSpeedY() * XY_UNIT_TO_M;
			f[2] = car.getSpeedZ() * Z_UNIT_TO_M;
			return true;
		}
		case INFO_ANGLES_3F: {
			float * f = (float*)dest;
			f[0] = car.getYaw();
			f[1] = car.getPitch();
			f[2] = car.getRoll();
			return true;
		}
		case INFO_CHECKPOINT_2I: {
			int * i = (int*)dest;
			i[0] = car.getCurrentCheckpoint();
			i[1] = car.getLastCheckpoint();
			return true;
		}
		default:
			return false;
	}
	return false;
}

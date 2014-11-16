/*
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
#include "core/Common.h"

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

	{ "formula",  350, 330, 220,   "Formula",                        "Ju" },
	{ NULL,       0,   0,   0,     "",                               "" },
	{ NULL,       0,   0,   0,     "",                               "" },
	{ NULL,       0,   0,   0,     "",                               "" },
};

Race::Race() :
	mxSdlRenderer(NULL),
	mpSdlTextureCircuit(NULL),
	mpSdlSurfaceCircuit(NULL),
	mpSdlSurfaceFunction(NULL),
	miTrackId(0),
	miCarId(0),
	show_tires(true),
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

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static Uint32 sdlGetPixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
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
    /* Here p is the address to the pixel we want to set */
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

/* load the car sprite and rotate it for every angles */
void Race::generateCars() {
  int i,j;
  SDL_Surface *car;
  char temp[20]="sprites/carX.png";
  for (i=0;i<NB_CARS;i++) {
    temp[11]='A'+i;
    car=IMG_Load(temp); // load the car sprite
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
        x2=(x-mpaSdlSurfaceCars[i][j]->w/2.0)*tcos+(y-mpaSdlSurfaceCars[i][j]->h/2.0)*tsin+car->w/2.0;
        y2=(x-mpaSdlSurfaceCars[i][j]->w/2.0)*tsin-(y-mpaSdlSurfaceCars[i][j]->h/2.0)*tcos+car->h/2.0;
        if (x2>0 && x2<car->w && y2>0 && y2<car->h)
          sdlPutPixel(mpaSdlSurfaceCars[i][j],x,y,sdlGetPixel(car,x2,y2));
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
  mpSdlTextureCircuit = SDL_CreateTextureFromSurface(mxSdlRenderer, mpSdlSurfaceCircuit);

  char funcname[128];
  sprintf(funcname, "tracks/%s_function.png", track[miTrackId].filename);
  mpSdlSurfaceFunction = IMG_Load(funcname);

	mUpKey = false;
	mDownKey = false;
	mLeftKey = false;
	mRightKey = false;

  car.speed = 0;
  car.angle = track[miTrackId].start_a * 2. * M_PI / 360.;
  car.ox = car.x = track[miTrackId].start_x;
  car.oy = car.y = track[miTrackId].start_y;
  car.lastcheck = 0;
  car.lapflag = 0;
  car.w = mpaSdlSurfaceCars[0][0]->w;
  car.h = mpaSdlSurfaceCars[0][0]->h;
}

#if 0
void Race::turn() {
  SDL_Rect pos;
  SDL_Event event;
  int i;
  int delay=DELAY;


    delay=DELAY;

    /* clear the old position */
    pos.x=car.ox-car.w/2;
    pos.y=car.oy-car.h/2;
    pos.w=car.w;
    pos.h=car.h;
    SDL_BlitSurface(mpSdlSurfaceCircuit, &pos, mpSdlSurfaceScreen, &pos);
        
    /* display the car at the new position */
    pos.x=car.x-car.w/2;
    pos.y=car.y-car.h/2;
    pos.w=car.w;
    pos.h=car.h;
    SDL_BlitSurface(mpaSdlSurfaceCars[miCarId][(unsigned char)(256*car.angle/2.0/M_PI)%256], NULL, mpSdlSurfaceScreen, &pos);

    
    /* if the car is fast or braking, it slides */
    if ((kd && car.speed>0.5) || (car.speed>2.0 && !ku)) {
      /* display tires slide */
      if (show_tires) {
        sdlPutPixel(mpSdlSurfaceCircuit,car.x+cos(car.angle)*car.w/3-sin(car.angle)*4,car.y+sin(car.angle)*car.h/3+cos(car.angle)*4,T_BLACK);
        sdlPutPixel(mpSdlSurfaceCircuit,car.x+cos(car.angle)*car.w/3+sin(car.angle)*4,car.y+sin(car.angle)*car.h/3-cos(car.angle)*4,T_BLACK);
        /* if we are braking the slide is larger */
        if (kd) {
          sdlPutPixel(mpSdlSurfaceCircuit,car.x+cos(car.angle)*car.w/3-sin(car.angle)*3,car.y+sin(car.angle)*car.h/3+cos(car.angle)*3,T_BLACK);
          sdlPutPixel(mpSdlSurfaceCircuit,car.x+cos(car.angle)*car.w/3+sin(car.angle)*3,car.y+sin(car.angle)*car.h/3-cos(car.angle)*3,T_BLACK);
        }
      }
    }
   
    //SDL_RenderPresent(mxSdlRenderer);
    /* let the system breath */
    SDL_Delay(delay);
}
#endif

void Race::lights(int x, int y, int r) {
  SDL_RenderDrawPoint(mxSdlRenderer, x, y);
  if (r>1) {
    SDL_RenderDrawPoint(mxSdlRenderer, x-1, y   );
    SDL_RenderDrawPoint(mxSdlRenderer, x+1, y   );
    SDL_RenderDrawPoint(mxSdlRenderer, x,   y-1 );
    SDL_RenderDrawPoint(mxSdlRenderer, x,   y+1 );
  }
  if (r>2) {
    SDL_RenderDrawPoint(mxSdlRenderer, x-2, y   );
    SDL_RenderDrawPoint(mxSdlRenderer, x+2, y   );
    SDL_RenderDrawPoint(mxSdlRenderer, x,   y-2 );
    SDL_RenderDrawPoint(mxSdlRenderer, x,   y+2 );
    SDL_RenderDrawPoint(mxSdlRenderer, x-1, y-1 );
    SDL_RenderDrawPoint(mxSdlRenderer, x-1, y+1 );
    SDL_RenderDrawPoint(mxSdlRenderer, x+1, y-1 );
    SDL_RenderDrawPoint(mxSdlRenderer, x+1, y+1 );
  }
}

bool Race::draw() {
	SDL_Rect circ_rect;
	circ_rect.w = mpSdlSurfaceCircuit->w;
	circ_rect.h = mpSdlSurfaceCircuit->h;
	circ_rect.x = 0;
	circ_rect.y = 0;

	SDL_RenderClear(mxSdlRenderer);
	SDL_RenderCopy(mxSdlRenderer, mpSdlTextureCircuit, NULL, &circ_rect);

	SDL_Rect car_rect;
	car_rect.x = car.x - car.w/2;
	car_rect.y = car.y - car.h/2;
	car_rect.w = car.w;
	car_rect.h = car.h;

	unsigned char car_angle = (unsigned char)(256*car.angle/2.0/M_PI)%256;
	SDL_Texture  * car_texture = SDL_CreateTextureFromSurface(mxSdlRenderer, mpaSdlSurfaceCars[miCarId][car_angle]);
	SDL_RenderCopy(mxSdlRenderer, car_texture, NULL, &car_rect);
	if (NULL != car_texture) {
		SDL_DestroyTexture (car_texture);
		car_texture = NULL;
	}

    if ( mDownKey && car.speed > 0.1 ) { // if the car is braking, display red lights
      SDL_SetRenderDrawColor(mxSdlRenderer, 255, 0, 0, 255); // Red
      lights(car.x+cos(car.angle)*car.w/3-sin(car.angle)*4,car.y+sin(car.angle)*car.h/3+cos(car.angle)*4, 3);
      lights(car.x+cos(car.angle)*car.w/3+sin(car.angle)*4,car.y+sin(car.angle)*car.h/3-cos(car.angle)*4, 3);
    }
    
    if ( car.speed < -0.1 ) { // if the car is going backwards, display white lights
      SDL_SetRenderDrawColor(mxSdlRenderer, 255, 255, 255, 255); // White
      lights(car.x+cos(car.angle)*car.w/3-sin(car.angle)*4,car.y+sin(car.angle)*car.h/3+cos(car.angle)*4, 3);
      lights(car.x+cos(car.angle)*car.w/3+sin(car.angle)*4,car.y+sin(car.angle)*car.h/3-cos(car.angle)*4, 3);
    }

    if ( car.speed >= -0.1 && car.speed <= 0.1 && (SDL_GetTicks() % 800) > 400 ) { // if the car is stopped, then warning
      SDL_SetRenderDrawColor(mxSdlRenderer, 255, 200, 0, 255); // Orange
      lights(car.x-cos(car.angle)*car.w/3-sin(car.angle)*5,car.y-sin(car.angle)*car.h/3+cos(car.angle)*5, 2);
      lights(car.x-cos(car.angle)*car.w/3+sin(car.angle)*5,car.y-sin(car.angle)*car.h/3-cos(car.angle)*5, 2);
      lights(car.x+cos(car.angle)*car.w/3-sin(car.angle)*5,car.y+sin(car.angle)*car.h/3+cos(car.angle)*5, 2);
      lights(car.x+cos(car.angle)*car.w/3+sin(car.angle)*5,car.y+sin(car.angle)*car.h/3-cos(car.angle)*5, 2);
    }
    
    if ( false ) { // position lights
      SDL_SetRenderDrawColor(mxSdlRenderer, 255, 0, 0, 255); // Red
      lights(car.x+cos(car.angle)*car.w/3-sin(car.angle)*3,car.y+sin(car.angle)*car.h/3+cos(car.angle)*4, 2);
      lights(car.x+cos(car.angle)*car.w/3+sin(car.angle)*3,car.y+sin(car.angle)*car.h/3-cos(car.angle)*4, 2);
      SDL_SetRenderDrawColor(mxSdlRenderer, 255, 255, 100, 255); // Yellow
      lights(car.x-cos(car.angle)*car.w/3-sin(car.angle)*4,car.y-sin(car.angle)*car.h/3+cos(car.angle)*4, 3);
      lights(car.x-cos(car.angle)*car.w/3+sin(car.angle)*4,car.y-sin(car.angle)*car.h/3-cos(car.angle)*4, 3);
    }

	SDL_RenderPresent(mxSdlRenderer);
	SDL_SetRenderDrawColor(mxSdlRenderer, 0, 0, 0, 0);

	return true;
}

void Race::moveCar() {
  Uint32 c;
  Uint8 r,g,b;
  
  // reset flags
  car.crashflag=0;
  
  // get the pixel color under the center of car in the function map
  c = sdlGetPixel(mpSdlSurfaceFunction, car.x, car.y);

  /* red layer (checkpoints) */
  /* green layer (road quality) */
  SDL_GetRGB(c,mpSdlSurfaceFunction->format, &r, &g, &b);

  if (mUpKey) {
    car.speed += 0.01 * 2. * COEFF;
  }
  if (mDownKey) {
    car.speed -= 0.01 * COEFF;
  }
  if (mLeftKey) {
    if (car.speed < 0) {
      car.angle += 0.01 * COEFF;
    } else {
      car.angle -= 0.01 * COEFF;
    }
  }
  if (mRightKey) {
    if (car.speed<0) {
      car.angle -= 0.01 * COEFF;
    } else {
      car.angle += 0.01 * COEFF;
    }
  }
  
  // limit angle between 0 and 2*pi
  if (car.angle < 0.) {
       car.angle += 2. * M_PI;
  }
  if (car.angle > 2. * M_PI) {
      car.angle -= 2. * M_PI;
  }
  
  // update the speed depending on the road quality
  car.speed -= car.speed * (255-g) / 1000.;
 
  // if it is a wall we move back to the last position
  if (g==0) {
    car.x = car.ox;
    car.y = car.oy;
    car.crashflag=1;
  }
  
  // save the old position and compute the new one
  car.ox = car.x;
  car.oy = car.y;
  car.speed *= 0.995;
  car.x -= cos(car.angle) * car.speed;
  car.y -= sin(car.angle) * car.speed;
  
  // collision with the border of the screen
  if (car.x < car.w/2 || car.x > mpSdlSurfaceFunction->w - car.w/2 || car.y < car.h/2 || car.y > mpSdlSurfaceFunction->h - car.h/2) {
    car.x = car.ox;
    car.y = car.oy;
    car.speed = 0;
    car.crashflag = 1;
  }
  
  // if we are on the next checkpoint, validate it
  if (r/8 == car.lastcheck + 1) {
    /* If we validate a missed checkpoint */
    if (car.lapflag==3) car.lapflag=4;
    car.lastcheck++;
  }
  
  // if we missed a checkpoint
  if ((r/8 > car.lastcheck+1) && (car.lastcheck != 0)) car.lapflag = 3;

  // if we validate all and start over, we complete a turn
  if (r/8 == 0 && car.lastcheck == 31) { // reset turn variables
    car.lastcheck = 0;
    car.lap++;
    car.lapflag = 1;
  }
  
  // if we are at the start but not each checkpoint validate, it's an incomplete lap
  if (r/8==0 && r!=0 && car.lastcheck != 31 && car.lastcheck > 0) {
    car.lastcheck = 0;
    car.lapflag = 2;
  }
  
  return;
}

unsigned int Race::update(unsigned int milliseconds) {
	while ( milliseconds > 8 ) {
		moveCar();

    switch (car.lapflag) {
      case 1: // if we completed a lap
        printLog("Lap Complete");
        car.lapflag=0;
        break;
      case 2: // if we completed an incomplete lap
        printLog("Last Lap Canceled");
        car.lapflag=0;
        break;
      case 3: // if we miss a checkpoint
        printLog("Checkpoint missed!");
        break;
      case 4: // if we validate a missed checkpoint
        printLog("Checkpoint missed OK");
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
			return true;
		}
		case SDL_KEYUP: {
			switch (event.key.keysym.sym) {
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
			return true;
		}
		default:
			return false;
	}
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
			//printf("JOY%d.BUTTON%d: %s\n", event.button.which, event.button.button, (event.button.state == SDL_PRESSED) ? "DOWN" : "UP");
			return true;
		}

		case SDL_JOYAXISMOTION: {
			//printf("JOY%d.AXIS%d: %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
			return true;
		}

		case SDL_JOYHATMOTION: {
			//printf("JOY%d.PAD%d: %d\n", event.jhat.which, event.jhat.hat, event.jhat.value);
			return true;
		}

		case SDL_JOYBALLMOTION: {
			//printf("JOY%d.BALL%d: %d, %d\n", event.jball.which, event.jball.ball, event.jball.xrel, event.jball.yrel);
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

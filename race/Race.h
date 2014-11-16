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
public:
	float x,y,ox,oy,angle,speed;
	int w,h,lastcheck,lap;
	int lapflag,crashflag;
	int color;
	int lights_brake,lights_backwards,lights_warning,lights_position;
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

private:
	static const size_t MAXLINELENGTH = 80;
	static const int DELAY = 7;
	static const int NB_CARS = 16;
	static const int MAX_TRACKS = 16;

	static const int SCREEN_WIDTH  = 1024;
	static const int SCREEN_HEIGHT = 768;

	static const float COEFF = 1.;

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
	void moveCar();
	void lights(int x, int y, int r);
	void darkenTrack(SDL_Surface * surface, float coef = 0.3);
};

#endif // RACE_H_A71ADAE4_6CB3_11E4_93E0_10FEED04CD1C

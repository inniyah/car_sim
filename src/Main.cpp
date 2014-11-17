#include "MainGtk3App.h"
#include "Sdl2App.h"
#include <SDL2/SDL.h>
#include <time.h>

int main(int argc, char *argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "\nUnable to initialize SDL:  %s\n", SDL_GetError() );
		return 1;
	}

	srand(time(NULL));

	int status = startMainGtk3App(argc, argv, new Sdl2App());

	return status;
}

#include "Sdl2App.h"
#include "Threads.h"

#define LOGO_BMP "data/sdl_logo.bmp"

struct Sdl2AppThread : public ThreadBase {
	Sdl2AppThread(Sdl2App * app) : App(app), KeepRunning(true) {
	}

	bool stop() {
		KeepRunning = false;
		return join();
	}

	virtual void run();
	Sdl2App * App;
	volatile bool KeepRunning;
};

void Sdl2AppThread::run() {
	printf("Thread ON\n");

	while (KeepRunning) {
	}

	printf("Thread OFF\n");
}

Sdl2App::Sdl2App() : mxSdlWindow(NULL) {
	mpSdlRenderer = NULL;

	mpThread = new Sdl2AppThread(this);
	if (!mpThread->start()) {
		delete mpThread;
		mpThread = NULL;
	}
}

Sdl2App::~Sdl2App() {
	printf("Waiting for thread to stop...\n");
	if (NULL != mpThread) {
		mpThread->stop();
		delete mpThread;
		mpThread = NULL;
	}

}

void Sdl2App::init(SDL_Window * sdl_window, int w, int h) {
	mxSdlWindow = sdl_window;

	miStartClock = SDL_GetTicks();
	mfFramesPerSecond = 0.0;

	miScreenWidth = w;
	miScreenHeight = h;
	mbInputGrabActive = false;
	mbInputGrabRequested = false;
	mbWindowMinimized = false;
	mbUseRelativeMouse = true;
	mbCanUseRelativeMouse = true;
	mbRelativeMouseActive = false;
	mbRepeatKeys = false;

	printf ("Initializing renderer\n");
	int i;
	int num = SDL_GetNumRenderDrivers();
	for (i=0;i<num;i++) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		printf ("Renderer %d: %s\n",i, info.name);
	}

	Uint32 window_flags = SDL_GetWindowFlags(mxSdlWindow);
	if (!(window_flags & SDL_WINDOW_OPENGL)) {
		printf ("Window doesn't support OpenGL, using software rendering\n");
		mpSdlRenderer = SDL_CreateRenderer(mxSdlWindow, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
	} else {
		mpSdlRenderer = SDL_CreateRenderer(mxSdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	}

	mpSdlImage = SDL_LoadBMP( LOGO_BMP );
	mpSdlTexture = SDL_CreateTextureFromSurface(mpSdlRenderer, mpSdlImage);

	mRace.setUp(mpSdlRenderer);
	mRace.startTrack(1);

	miLastUpdateTime = SDL_GetTicks();
}

void Sdl2App::destroy() {
	if (NULL != mpSdlImage) {
		SDL_FreeSurface (mpSdlImage);
		mpSdlImage = NULL;
	}
	if (NULL != mpSdlTexture) {
		SDL_DestroyTexture (mpSdlTexture);
		mpSdlTexture = NULL;
	}
	if (NULL != mpSdlRenderer) {
		SDL_DestroyRenderer (mpSdlRenderer);
		mpSdlRenderer = NULL;
	}

	mxSdlWindow = NULL;
}

void Sdl2App::draw() {
	if (!mRace.draw()) {
		SDL_Rect dest_rect;
		dest_rect.w = mpSdlImage->w;
		dest_rect.h = mpSdlImage->h;
		dest_rect.x = miScreenWidth / 2 - mpSdlImage->w / 2;
		dest_rect.y = miScreenHeight / 2 - mpSdlImage->h / 2;

		SDL_RenderClear(mpSdlRenderer);
		SDL_RenderCopy(mpSdlRenderer, mpSdlTexture, NULL, &dest_rect);
		SDL_RenderPresent(mpSdlRenderer);
	}

	int delta = SDL_GetTicks() - miStartClock;
	miStartClock = SDL_GetTicks();
	if (delta != 0) {
		mfFramesPerSecond = 1000.0 / delta;
		//printf("%2.2f fps\n", mfFramesPerSecond);
	}
}

void Sdl2App::update() {
	Uint32 current_time = SDL_GetTicks();
	unsigned int pending = mRace.update(current_time - miLastUpdateTime);
	miLastUpdateTime = current_time - pending;
}

void Sdl2App::processEvents() {
	SDL_Event event;
	while ( SDL_PollEvent( &event ) ) {
			eventHandler(event);
	}
}

bool Sdl2App::eventHandler(SDL_Event & event) {
	switch(event.type) {
		case SDL_QUIT: {
			processQuitRequest();
			return true;
		}

		case SDL_TEXTINPUT: {
			processTextInput(event.text.text, strlen(event.text.text));
			return true;
		}

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return mRace.eventHandlerKeyboard(event);

		case SDL_WINDOWEVENT:
			return eventHandlerWindow(event);

		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			return mRace.eventHandlerMouse(event);

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYAXISMOTION:
		case SDL_JOYHATMOTION:
		case SDL_JOYBALLMOTION:
			return mRace.eventHandlerJoystick(event);

		case SDL_USEREVENT:
			return mRace.eventHandlerUser(event);
	}

	return false;
}

// https://wiki.libsdl.org/SDL_WindowEventID

bool Sdl2App::eventHandlerWindow(SDL_Event & event) {
	switch (event.type) {
		case SDL_WINDOWEVENT: {
			switch(event.window.event) {
				case SDL_WINDOWEVENT_FOCUS_GAINED: {
					mbInputGrabRequested = true;
					return true;
				}

				case SDL_WINDOWEVENT_ENTER: {
					requestInputGrab();
					return true;
				}

				case SDL_WINDOWEVENT_LEAVE:
				case SDL_WINDOWEVENT_FOCUS_LOST: {
					releaseInputGrab();
					return true;
				}

				case SDL_WINDOWEVENT_MINIMIZED: {
					mbWindowMinimized = true;
					return true;
				}

				case SDL_WINDOWEVENT_MAXIMIZED:
				case SDL_WINDOWEVENT_RESTORED: {
					mbWindowMinimized = false;
					return true;
				}

				case SDL_WINDOWEVENT_RESIZED: {
					printf("SDL_WINDOWEVENT_RESIZED: %dx%d\n", event.window.data1, event.window.data2);
					return true;
				}

				case SDL_WINDOWEVENT_SIZE_CHANGED: {
					printf("SDL_WINDOWEVENT_SIZE_CHANGED: %dx%d\n", event.window.data1, event.window.data2);
					miScreenWidth = event.window.data1;
					miScreenHeight = event.window.data2;
					return true;
				}

				default:
					return false;
			}
		}

		default:
			return false;
	}
}

void Sdl2App::requestInputGrab() {
	mbInputGrabActive = true;
	SDL_ShowCursor(SDL_FALSE);
	if (mbCanUseRelativeMouse && mbUseRelativeMouse) {
		if(SDL_SetRelativeMouseMode(SDL_TRUE) >= 0) {
			SDL_SetWindowGrab(mxSdlWindow, SDL_TRUE);
			mbRelativeMouseActive = true;
		} else {
			SDL_SetWindowGrab(mxSdlWindow, SDL_FALSE);
			mbCanUseRelativeMouse = false;
			mbRelativeMouseActive = false;
		}
	}
	mbInputGrabRequested = false;
}

void Sdl2App::releaseInputGrab() {
	mbInputGrabActive = false;
	SDL_ShowCursor(SDL_TRUE);
	if (mbRelativeMouseActive) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_SetWindowGrab(mxSdlWindow, SDL_FALSE);
		mbRelativeMouseActive = false;
	}
	mbInputGrabRequested = false;
}

void Sdl2App::processTextInput(const char *str, int len) {
}

void Sdl2App::processKeyboardKey(int code, bool isdown) {
}

void Sdl2App::processQuitRequest() {
	exit(0);
}


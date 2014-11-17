#ifndef SDL2APP_H_F74726CC_5FC4_11E4_8E19_10FEED04CD1C
#define SDL2APP_H_F74726CC_5FC4_11E4_8E19_10FEED04CD1C

#include "ISdl2App.h"
#include "Race.h"

#include <SDL2/SDL.h>
#include <stdint.h>
#include <slm/mat4.h>
#include <slm/quat.h>

class Sdl2AppThread;

class Sdl2App : public ISdl2App {
	public:
		Sdl2App();
		virtual ~Sdl2App();

		virtual void init(SDL_Window * sdl_window, int w, int h);
		virtual void destroy();

		virtual void draw();
		virtual void update();
		virtual void processEvents();

		virtual bool getInfo(void * dest, unsigned int type, intptr_t param);

	protected:
		bool eventHandler(SDL_Event & event);
		bool eventHandlerWindow(SDL_Event & event);

		void requestInputGrab();
		void releaseInputGrab();
		void processTextInput(const char *str, int len);
		void processKeyboardKey(int code, bool isdown);
		void processQuitRequest();

	private:
		SDL_Window   * mxSdlWindow;
		SDL_Renderer * mpSdlRenderer;

		SDL_Surface * mpSdlImage;
		SDL_Texture * mpSdlTexture;

		Race mRace;

		int miScreenWidth;
		int miScreenHeight;
		bool mbInputGrabActive;
		bool mbInputGrabRequested;
		bool mbWindowMinimized;
		bool mbUseRelativeMouse;
		bool mbCanUseRelativeMouse;
		bool mbRelativeMouseActive;
		bool mbRepeatKeys;

		Uint32 miStartClock;
		float mfFramesPerSecond;

		Uint32 miLastUpdateTime;

		Sdl2AppThread * mpThread;

		Sdl2App(const Sdl2App &);
		Sdl2App & operator=(const Sdl2App &);

};

#endif // SDL2APP_H_F74726CC_5FC4_11E4_8E19_10FEED04CD1C

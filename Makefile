PROGRAM=test

all: $(PROGRAM)

SRCS = \
	src/MainGtk3App.cpp \
	src/Sdl2App.cpp \
	src/Threads.cpp \
	src/InfoHandler.cpp \
	src/Main.cpp \
	src/Race.cpp

OBJS = $(SRCS:.cpp=.o)

PKG_CONFIG=gtk+-3.0 sdl2
PKG_CONFIG_CFLAGS=`pkg-config --cflags $(PKG_CONFIG)`
PKG_CONFIG_LIBS=`pkg-config --libs $(PKG_CONFIG)`

CFLAGS= -O2 -g -Wall
INCS=-I. -Islmath/include -Igamepad/include
LDFLAGS= -Wl,-z,defs -Wl,--as-needed -Wl,--no-undefined
LIBS=$(PKG_CONFIG_LIBS) -lSDL2_image -lSDL2_gfx -lpthread -lm -Lslmath -lslmath -Lgamepad -lgamepad

$(PROGRAM): $(OBJS) slmath/libslmath.a
	g++ $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: %.cpp
	g++ -o $@ -c $< $(CFLAGS) $(INCS) $(PKG_CONFIG_CFLAGS)

%.o: %.c
	gcc -o $@ -c $< $(CFLAGS) $(INCS) $(PKG_CONFIG_CFLAGS)

.depend depend dep:
	g++ $(CFLAGS) -MM $(SRCS) $(INCS) $(PKG_CONFIG_CFLAGS) > .depend
	$(MAKE) -C slmath .depend
	$(MAKE) -C gamepad .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

slmath/libslmath.a:
	$(MAKE) -C slmath libslmath.a
	$(MAKE) -C gamepad libgamepad.a

clean:
	rm -f $(OBJS)
	rm -f $(PROGRAM)
	rm -f *.o *.a *~

clean-all: clean
	$(MAKE) -C slmath clean
	$(MAKE) -C gamepad clean

.PHONY: all depend dep clean install

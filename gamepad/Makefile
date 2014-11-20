PROGRAM=testharness
LIBNAME=gamepad
LIBRARY=lib$(LIBNAME)

all: static

static: $(LIBRARY).a
dynamic: $(LIBRARY).so
program: $(PROGRAM)

include version

MAJOR = $(VERSION_MAJOR)
MINOR = $(VERSION_MINOR)

LIB_SRC = src/Gamepad_private.c src/Gamepad_linux.c
PRG_SRC = src/testharness/main.c

SHARED_OBJS = $(LIB_SRC:.c=.shared.o)
STATIC_OBJS = $(LIB_SRC:.c=.static.o)
PROGRAM_OBJS = $(PRG_SRC:.c=.o)

PKG_CONFIG=gl glu
PKG_CONFIG_CFLAGS=`pkg-config --cflags $(PKG_CONFIG) 2>/dev/null`
PKG_CONFIG_LIBS=`pkg-config --libs $(PKG_CONFIG) 2>/dev/null`

CFLAGS=-O2 -g -Wall 
INCLUDE=-Iinclude $(PKG_CONFIG_CFLAGS)
STATIC_CFLAGS= -O2 -g -Wall $(CFLAGS) $(INCLUDE)
SHARED_CFLAGS= $(STATIC_CFLAGS) -fPIC

LDFLAGS= -Wl,-z,defs -Wl,--as-needed -Wl,--no-undefined
LIBS=-lpthread -lglut $(PKG_CONFIG_LIBS)

$(PROGRAM): $(PROGRAM_OBJS) $(LIBRARY).a $(LIBRARY).so
	g++ $(LDFLAGS) $(PROGRAM_OBJS) -o $@ -l$(LIBNAME) -L. $(LIBS)

$(LIBRARY).so.$(MAJOR).$(MINOR): $(SHARED_OBJS)
	g++ $(LDFLAGS) -shared \
		-Wl,-soname,$(LIBRARY).so.$(MAJOR) \
		-o $(LIBRARY).so.$(MAJOR).$(MINOR) \
		$+ -o $@ $(LIBS)

$(LIBRARY).so: $(LIBRARY).so.$(MAJOR).$(MINOR)
	rm -f $@.$(MAJOR)
	ln -s $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	rm -f $@
	ln -s $@.$(MAJOR) $@

$(LIBRARY).a: $(STATIC_OBJS)
	ar cru $@ $+

%.shared.o: %.cpp
	g++ -o $@ -c $+ $(SHARED_CFLAGS)

%.shared.o: %.c
	gcc -o $@ -c $+ $(SHARED_CFLAGS)

%.so : %.o
	g++ $(LDFLAGS) -shared $^ -o $@

%.static.o: %.cpp
	g++ -o $@ -c $+ $(STATIC_CFLAGS)

%.static.o: %.c
	gcc -o $@ -c $+ $(STATIC_CFLAGS)

%.o: %.cpp
	g++ -o $@ -c $+ $(STATIC_CFLAGS)

%.o: %.c
	gcc -o $@ -c $+ $(STATIC_CFLAGS)

%.cpp %.h: %.ll
	flex --outfile=$@ --header-file=$(@:.cpp=.h) $<

%.cpp %.h: %.yy
	bison --output=$@ --defines=$(@:.cpp=.h) $<

clean:
	rm -f $(SHARED_OBJS)
	rm -f $(STATIC_OBJS)
	rm -f $(PROGRAM_OBJS)
	rm -f $(PROGRAM)
	rm -f src/lexer.cpp src/lexer.h
	rm -f src/grammar.cpp src/grammar.h
	rm -f *.so *.so* *.a *~

.depend depend dep: $(LIB_SRC) $(PRG_SRC)
	g++ $(CFLAGS) -MM $(LIB_SRC) $(PRG_SRC) $(INCLUDE) $(PKG_CONFIG_CFLAGS) > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

test: $(PROGRAM)
	LD_LIBRARY_PATH="." ./$(PROGRAM)

.PHONY: all static dynamic program depend dep clean install

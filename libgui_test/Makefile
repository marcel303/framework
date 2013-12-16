include ../Makefile.defs

LIBGG = ../libgg/libgg.a
LIBGUI = ../libgui/libgui.a

LIBGUI_TEST = test
LIBGUI_TEST_SRC = \
	TestMain.cpp \
	../libiphone/EventManager.cpp \
	../libiphone/Event_SDL.cpp \
	../libgg/TextureRGBA.cpp

LIBGUI_TEST_DST = $(LIBGUI_TEST_SRC:.cpp=.o)

all : $(LIBGUI_TEST)

$(LIBGUI) :
	cd ../libgui && $(MAKE)

$(LIBGUI_TEST) : $(LIBGUI) $(LIBGUI_TEST_DST)
	gcc -o $(LIBGUI_TEST) $(LIBGUI_TEST_DST) $(LIBGUI) $(LIBGG) -lSDL -lGL -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ -I"../libiphone" -I"../libgui" -I"../libgg"


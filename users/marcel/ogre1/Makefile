EXE = ogre
OBJS = main.o tinyxml2.o ../../../libgg/Quat.o ../framework/image.o ../framework/model.o ../framework/model_ogre.o
CFLAGS = -Wall -Wextra -O2 -I../../../libgg -DNO_SDL_GLEXT
LFLAGS = -lFreeImage `sdl-config --static-libs` -lstdc++

all : $(EXE)
	./ogre

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

EXE = framework
OBJS = framework.o internal.o audio.o image.o audiostream/AudioMixer.o audiostream/AudioOutput.o audiostream/AudioStreamVorbis.o audiostream/oggvorbis.o
#CFLAGS = -Wall -ffast-math -g -DDEBUG
CFLAGS = -Wall -ffast-math -O3 -msse3
CFLAGS += -I./ -I./AudioStream/
CFLAGS += `freetype-config --cflags` -DNO_SDL_GLEXT
LFLAGS = `freetype-config --libs` `sdl-config --static-libs` -framework OpenAL -lFreeImage -lstdc++ -lGLEW

all : $(EXE)
	./framework devmode

$(EXE) : example.o $(OBJS)
	gcc -o $(EXE) example.o $(OBJS) $(LFLAGS)

samples : examples/gui.o examples/sounds.o examples/sprites.o
	gcc -o examples/gui examples/gui.o $(OBJS) $(LFLAGS)
	gcc -o examples/sounds examples/sounds.o $(OBJS) $(LFLAGS)
	gcc -o examples/sprites examples/sprites.o $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS)

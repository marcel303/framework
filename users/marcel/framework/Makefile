EXE = framework
OBJS = framework.o internal.o audio.o image.o model.o model_fbx.o model_ogre.o audiostream/AudioMixer.o audiostream/AudioOutput.o audiostream/AudioStreamVorbis.o audiostream/oggvorbis.o ../fbx1/fbx.o tinyxml2.o ../../../libgg/Quat.o
#CFLAGS = -Wall -ffast-math -g -DDEBUG
CFLAGS = -Wall -ffast-math -O3 -msse3
CFLAGS += -I./ -I./AudioStream/
CFLAGS += `freetype-config --cflags` -DNO_SDL_GLEXT -I../../../libgg
LFLAGS = `freetype-config --libs` `sdl-config --static-libs` -framework OpenAL -lFreeImage -lstdc++ -lGLEW
SAMPLES = examples/gui.exe examples/model.exe examples/sounds.exe examples/sprites.exe

all : $(EXE)
	./framework devmode

$(EXE) : example.o $(OBJS)
	gcc -o $(EXE) example.o $(OBJS) $(LFLAGS)

samples : $(SAMPLES)

%.exe : %.o $(OBJS)
	gcc -o $@ $< $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS)

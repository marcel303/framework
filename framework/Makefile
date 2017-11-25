EXE = framework
OBJS = framework.o internal.o audio.o image.o model.o model_fbx.o model_ogre.o audiostream/AudioMixer.o audiostream/AudioOutput.o audiostream/AudioStreamVorbis.o audiostream/oggvorbis.o ../fbx1/fbx.o tinyxml2.o ../../../libgg/Quat.o
#CFLAGS = -Wall -ffast-math -g -DDEBUG
CFLAGS = -Wall -ffast-math -O3 -msse3
CFLAGS += -I./ -I./AudioStream/
CFLAGS += `freetype-config --cflags`  -I../../../libgg -I./macos/glew/include -I./macos/FreeImage
CFLAGS += -I./macos/SDL -DNO_SDL_GLEXT
LFLAGS = `freetype-config --libs` -framework OpenAL -L./macos/FreeImage -lfreeimage -L./macos/glew/lib -lGLEW
LFLAGS += macos/SDL/SDL2/SDL2 -framework OpenGL
LFLAGS += -lc++
#SAMPLES = examples/gui.exe examples/model.exe examples/sounds.exe examples/sprites.exe
SAMPLES = examples/model.exe

all : $(EXE) samples
	#./framework devmode
	./examples/model.exe

$(EXE) : example.o $(OBJS)
	gcc -o $(EXE) example.o $(OBJS) $(LFLAGS)

samples : $(SAMPLES)

clean :
	@rm $(OBJS)
	@rm $(EXE)
	@rm $(SAMPLES)

%.exe : %.o $(OBJS)
	gcc -o $@ $< $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS)

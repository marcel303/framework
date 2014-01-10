EXE = framework
OBJS = example.o framework.o internal.o audio.o image.o audiostream/AudioMixer.o audiostream/AudioOutput.o audiostream/AudioStreamVorbis.o audiostream/oggvorbis.o
#CFLAGS = -Wall -ffast-math -g -DDEBUG
CFLAGS = -Wall -ffast-math -O3 -msse3
CFLAGS += -I./ -I./AudioStream/
CFLAGS += `freetype-config --cflags`

all : $(EXE)
	./framework

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) `freetype-config --libs` `sdl-config --static-libs` -framework OpenAL -lFreeImage -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS)

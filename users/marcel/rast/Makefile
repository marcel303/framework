include ../../../Makefile.defs

ifeq ($(OS),linux)
CFLAGS = -march=native -mtune=native -O3 -msse4 -finline-functions -funroll-loops -ffast-math -mfpmath=sse -Wall -pipe -ffast-math -DNDEBUG
else ifeq ($(OS),macos)
CFLAGS = -march=core2 -mtune=native -O3 -msse4 -finline-functions -funroll-loops -ffast-math -mfpmath=sse -DNDEBUG -Wall
else
CFLAGS = -march=native -mtune=native -ffast-math -O2 -msse3 -funroll-loops -Wall -pipe -fexceptions -ftree-vectorize -funsafe-math-optimizations -DNDEBUG -DWINDOWS
endif

all : rast

clean :
	-rm rast
	-rm main-mini.o md3.o

rast : main-mini.o md3.o
	#clang -o rast main-mini.o md3.o -lSDL `allegro-config --libs` -m32 -lstdc++ -s
	gcc -o rast main.o md3.o `sdl-config --static-libs` -lstdc++ -s
#`allegro-config --libs`

%.o : %.cpp
	#clang -o $@ -c $< $(CFLAGS) -Wno-unknown-pragmas
	gcc -o $@ -c $< $(CFLAGS) -Wno-unknown-pragmas


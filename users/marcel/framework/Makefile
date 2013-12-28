EXE = framework
OBJS = example.o framework.o internal.o

all : $(EXE)
	./framework

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) `freetype-config --libs` `sdl-config --static-libs` -framework OpenAL -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ -Wall -ffast-math -O3 -msse3 `freetype-config --cflags`

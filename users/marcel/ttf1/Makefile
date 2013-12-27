EXE = ttf
OBJS = main.o

all : $(EXE)
	./ttf

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) `freetype-config --libs` `sdl-config --static-libs` -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ -Wall -ffast-math -O3 -msse3 `freetype-config --cflags`

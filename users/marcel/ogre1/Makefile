EXE = ogre
OBJS = main.o tinyxml2.o
CFLAGS = -Wall -Wextra -Werror -ansi -pedantic -O2
LFLAGS = `sdl-config --static-libs` -lstdc++

all : $(EXE)
	./ogre

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

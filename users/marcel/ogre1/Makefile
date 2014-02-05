EXE = ogre
OBJS = main.o tinyxml2.o Quaternion.o
CFLAGS = -Wall -Wextra -O2
LFLAGS = `sdl-config --static-libs` -lstdc++

all : $(EXE)
	./ogre

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

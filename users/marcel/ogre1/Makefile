EXE = ogre
OBJS = main.o tinyxml2.o ../../../libgg/Quat.o
CFLAGS = -Wall -Wextra -O2 -I../../../libgg
LFLAGS = `sdl-config --static-libs` -lstdc++

all : $(EXE)
	./ogre

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

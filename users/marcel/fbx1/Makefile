EXE = fbx
OBJS = fbx.o main.o ../../../libgg/Quat.o ../framework/model.o
CFLAGS = -Wall -Wextra -Wno-unused -Werror -ansi -pedantic -O2 -I../../../libgg -DNO_SDL_GLEXT
LFLAGS = `sdl-config --static-libs` -lstdc++

all : $(EXE)
	./fbx test3.fbx -d

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

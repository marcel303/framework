EXE = fbx
OBJS = fbx.o main.o
CFLAGS = -Wall -Wextra -Wno-unused -Werror -ansi -pedantic -O2 -I../../../libgg
LFLAGS = `sdl-config --static-libs` -lstdc++

all : $(EXE)
	./fbx test3.fbx -d

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

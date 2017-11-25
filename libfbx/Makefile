EXE = fbx
OBJS = fbx.o main.o
CFLAGS = -Wall -Wextra -ansi -pedantic -Wno-unused -Werror -O2
LFLAGS = -lstdc++

all : $(EXE)
	./fbx test3.fbx -vs

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

EXE = fbx
OBJS = fbx.o main.o
CFLAGS = -Wall -Wextra -Werror -ansi -pedantic -O2
LFLAGS = -lstdc++

all : $(EXE)
	./fbx test2.fbx

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

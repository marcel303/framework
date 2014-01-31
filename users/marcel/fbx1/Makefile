EXE = bfx
OBJS = main.o
CFLAGS = -Wall -Wextra -Werror -ansi -pedantic
LFLAGS = -lstdc++

all : $(EXE)
	./bfx

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

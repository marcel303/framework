EXE = bfx
OBJS = main.o
#CFLAGS = -Wall -ffast-math -g -DDEBUG
LFLAGS = -lstdc++

all : $(EXE)
	./bfx

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) $(LFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

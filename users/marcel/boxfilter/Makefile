EXE = boxfilter
OBJS = main.o

all : $(EXE)
	./$(EXE)

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) `sdl-config --static-libs` -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ -Wall -O2

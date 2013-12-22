EXE = dof
OBJS = main.o

all : $(EXE)
	./dof

$(EXE) : $(OBJS)
	gcc -o $(EXE) $(OBJS) `sdl-config --static-libs` -lstdc++ -fopenmp

%.o : %.cpp
	gcc -c $< -o $@ -Wall -ffast-math -O3 -msse3 -fopenmp

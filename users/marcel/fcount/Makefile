all :
	gcc -c main.cpp -o fcount.o -O3 -finline-functions -funroll-loops -msse4 -mfpmath=sse -fopenmp
	gcc -o fcount fcount.o -fopenmp
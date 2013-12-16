all :
	gcc -c main.cpp -o main.o -march=native -mtune=native -O2 -msse2 -finline-functions -funroll-loops -ffast-math -mfpmath=sse -I../../../libgg/ -fopenmp
	gcc -o voxel.exe main.o `allegro-config --libs` -march=native -lstdc++ -s -fopenmp
	#gcc -c main.cpp -o main.o -march=native -mtune=native -g -msse2 -ffast-math -mfpmath=sse -I../../../libgg/ -fopenmp
	#gcc -o voxel.exe main.o `allegro-config --libs` -march=native -lstdc++

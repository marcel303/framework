LIBGG = ../../../libgg/libgg32.a
LIBGG_PATH = ../../../libgg/
LIBKLODDER_PATH = ../../../klodder/Classes/
LIBALLEG = `allegro-config --libs`

CFLAGS = -I$(LIBGG_PATH) -I$(LIBKLODDER_PATH) -m32 -O7 -ffast-math

all : $(LIBGG)
	gcc -c $(LIBKLODDER_PATH)Bezier.cpp -o Bezier.o $(CFLAGS)
	gcc -c $(LIBKLODDER_PATH)BezierTraveller.cpp -o BezierTraveller.o $(CFLAGS)
	gcc -c main.cpp -o main.o $(CFLAGS)
	gcc -o beztest main.o Bezier.o BezierTraveller.o $(LIBGG) $(LIBALLEG) -lstdc++ -m32
	./beztest

$(LIBGG) :
	cd ../../../libgg && $(MAKE)
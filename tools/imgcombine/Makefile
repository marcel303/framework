include ../../Makefile.defs

IMGCOMBINE = imgcombine$(EXECUTABLE_EXT)

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg

IMGCOMBINE_SRC = \
	imgcombine.cpp

IMGCOMBINE_DST = $(IMGCOMBINE_SRC:.cpp=.o)

all : $(IMGCOMBINE)

clean :
	-rm *.o
	-rm *.a
	
test : all
	cd explosion3 && ../imgcombine explosion3.png 4 4 64 64 boom-5-0001.png boom-5-0004.png boom-5-0009.png boom-5-0016.png boom-5-0023.png boom-5-0030.png boom-5-0037.png boom-5-0044.png boom-5-0051.png boom-5-0058.png boom-5-0065.png boom-5-0072.png boom-5-0079.png boom-5-0086.png boom-5-0093.png boom-5-0100.png
	cd explosion1 && ../imgcombine explosion1.png 4 4 64 64 boom-1-0001.png boom-1-0007.png boom-1-0013.png boom-1-0019.png boom-1-0025.png boom-1-0031.png boom-1-0037.png boom-1-0044.png boom-1-0051.png boom-1-0058.png boom-1-0065.png boom-1-0072.png boom-1-0079.png boom-1-0086.png boom-1-0093.png boom-1-0100.png
	
$(LIBGG) : dummy
	cd ../../prototypes/libgg/ && $(MAKE)

$(LIBRES) : dummy
	cd ../rescompiler/ && $(MAKE)


$(IMGCOMBINE) : $(LIBGG) $(LIBRES) $(LIBATL) $(IMGCOMBINE_DST)
	gcc -o $(IMGCOMBINE) $(IMGCOMBINE_DST) $(LIBRES) $(LIBATL) $(LIBGG) $(LIBALLEG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(LIBRES_INCLUDE) $(CFLAGS) $(LIBGG_INCLUDE)

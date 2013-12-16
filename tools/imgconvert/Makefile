include ../../Makefile.defs

IMGCONVERT = imgconvert$(EXECUTABLE_EXT)

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

IMGCONVERT_SRC = \
	imgconvert.cpp

IMGCONVERT_DST = $(IMGCONVERT_SRC:.cpp=.o)

all : $(IMGCONVERT)

clean :
	-rm *.o

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)
	
$(IMGCONVERT) : $(LIBGG) $(IMGCONVERT_DST)
	gcc -o $(IMGCONVERT) $(IMGCONVERT_DST) $(LIBGG) $(LIBALLEG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

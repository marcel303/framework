include ../../Makefile.defs

IMG2BRUSH = img2brush$(EXECUTABLE_EXT)

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg
LIBKLODDER = ../libklodder.a
LIBKLODDER_INCLUDE = -I../Classes -I../../usg/Classes

IMG2BRUSH_SRC = \
	main.cpp

IMG2BRUSH_DST = $(IMG2BRUSH_SRC:.cpp=.o)

all : $(IMG2BRUSH)

clean :
	-rm *.o

$(LIBGG) : dummy
	cd ../../prototypes/libgg/ && $(MAKE)
$(LIBKLODDER) : dummy
	cd ../ && $(MAKE)
	
$(IMG2BRUSH) : $(LIBGG) $(LIBKLODDER) $(IMG2BRUSH_DST)
	gcc -o $(IMG2BRUSH) $(IMG2BRUSH_DST) $(LIBKLODDER) $(LIBGG) $(LIBALLEG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBKLODDER_INCLUDE) $(LIBGG_INCLUDE)

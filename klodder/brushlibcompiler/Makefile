include ../../Makefile.defs

BRUSHLIBCOMPILER = brushlibcompiler$(EXECUTABLE_EXT)

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg
LIBKLODDER = ../libklodder.a
LIBKLODDER_INCLUDE = -I../Classes -I../../usg/Classes

BRUSHLIBCOMPILER_SRC = \
	brushlibcompiler.cpp

BRUSHLIBCOMPILER_DST = $(BRUSHLIBCOMPILER_SRC:.cpp=.o)

all : $(BRUSHLIBCOMPILER)

clean :
	-rm *.o

$(LIBGG) : dummy
	cd ../../prototypes/libgg/ && $(MAKE)
$(LIBKLODDER) : dummy
	cd ../ && $(MAKE)
	
$(BRUSHLIBCOMPILER) : $(LIBGG) $(LIBKLODDER) $(BRUSHLIBCOMPILER_DST)
	gcc -o $(BRUSHLIBCOMPILER) $(BRUSHLIBCOMPILER_DST) $(LIBKLODDER) $(LIBGG) $(LIBALLEG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBKLODDER_INCLUDE) $(LIBGG_INCLUDE)

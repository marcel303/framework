include ../../Makefile.defs

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg
LIBKLODDER = ../libklodder.a
LIBKLODDER_INCLUDE = -I../Classes -I../../usg/Classes
LIBCX = ../CxImage/libcx.a
LIBCX_INCLUDE = -I../CxImage

BRUSHCOMPILER = brushcompiler$(EXECUTABLE_EXT)
BRUSHCOMPILER_SRC = brushcompiler.cpp
BRUSHCOMPILER_DST = $(BRUSHCOMPILER_SRC:.cpp=.o)

all : $(BRUSHCOMPILER)

clean :
	-rm *.o

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)
$(LIBKLODDER) : dummy
	cd ../ && $(MAKE) libklodder.a
$(LIBCX) : dummy
	cd ../CxImage/ && $(MAKE)

$(BRUSHCOMPILER) : $(LIBGG) $(LIBKLODDER) $(LIBCX) $(BRUSHCOMPILER_DST)
	gcc -o $(BRUSHCOMPILER) $(BRUSHCOMPILER_DST) $(LIBKLODDER) $(LIBCX) $(LIBGG) $(LIBALLEG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++

brushes :
	./brushcompiler -c brushes.txt -o ../brushes.lib

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBCX_INCLUDE) $(LIBKLODDER_INCLUDE) $(LIBGG_INCLUDE)


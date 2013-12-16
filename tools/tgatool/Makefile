include ../../Makefile.defs

TGATOOL = tgatool$(EXECUTABLE_EXT)

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

TGATOOL_SRC = \
	tgatool.cpp

TGATOOL_DST = $(TGATOOL_SRC:.cpp=.o)

all : $(TGATOOL)

clean :
	-rm *.o

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)
	
$(TGATOOL) : $(LIBGG) $(TGATOOL_DST)
	gcc -o $(TGATOOL) $(TGATOOL_DST) $(LIBGG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

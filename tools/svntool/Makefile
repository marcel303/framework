include ../../Makefile.defs

SVNTOOL = svntool$(EXECUTABLE_EXT)

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

SVNTOOL_SRC = \
	svnutil.cpp

SVNTOOL_DST = $(SVNTOOL_SRC:.cpp=.o)

all : $(SVNTOOL)

clean :
	-rm *.o

test : all
	./svntool -wv ./ test.cpp test.h

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)
	
$(SVNTOOL) : $(LIBGG) $(SVNTOOL_DST)
	gcc -o $(SVNTOOL) $(SVNTOOL_DST) $(LIBGG) -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

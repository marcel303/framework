include ../../Makefile.defs

ATLCOMPILER = atlcompiler$(EXECUTABLE_EXT)

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

LIBRES = ../rescompiler/libres.a
LIBRES_INCLUDE = -I../rescompiler

LIBATL_SRC = \
	Atlas.cpp \
	AtlasNode.cpp \
	AtlasBuilder.cpp \
	AtlasBuilderV2.cpp
LIBATL_DST = $(LIBATL_SRC:.cpp=.o)
LIBATL = libatl.a

ATLCOMPILER_SRC = \
	Preview.cpp \
	Image_Allegro.cpp \
	atlcompiler.cpp

ATLCOMPILER_DST = $(ATLCOMPILER_SRC:.cpp=.o)

all : $(ATLCOMPILER)

#install : all
#	sudo cp ./atlcompiler /bin/

clean :
	-rm *.o
	-rm *.a
	
test : all
	./atlcompiler -c test0.png test1.png test2.png test3.png -o test.atl -b 2 -s 512
	
$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)

$(LIBRES) : dummy
	cd ../rescompiler/ && $(MAKE)


$(LIBATL) : $(LIBATL_DST)
	ar -rcs $(LIBATL) $(LIBATL_DST)

$(ATLCOMPILER) : $(LIBGG) $(LIBRES) $(LIBATL) $(ATLCOMPILER_DST)
	gcc -o $(ATLCOMPILER) $(ATLCOMPILER_DST) $(LIBRES) $(LIBATL) $(LIBGG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++ $(CFLAGS)
	
%.o : %.cpp
	gcc -c $< -o $@ $(LIBRES_INCLUDE) $(CFLAGS) $(LIBGG_INCLUDE)

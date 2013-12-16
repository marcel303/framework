include ../../Makefile.defs

SFXCOMPILER = sfxcompiler$(EXECUTABLE_EXT)

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg

LIBSFX_SRC = \
	SfxHeader.cpp \
	SfxFile.cpp
LIBSFX_DST = $(LIBSFX_SRC:.cpp=.o)
LIBSFX = libsfx.a

SFXCOMPILER_SRC = \
	AudioOps.cpp \
	sfxcompiler.cpp

SFXCOMPILER_DST = $(SFXCOMPILER_SRC:.cpp=.o)

all : $(SFXCOMPILER)

#install : all
#	sudo cp ./atlcompiler /bin/

clean :
	-rm *.o
	-rm *.a
	
$(LIBGG) : dummy
	cd ../../prototypes/libgg/ && $(MAKE)

$(LIBSFX) : $(LIBSFX_DST)
	ar -rcs $(LIBSFX) $(LIBSFX_DST)

$(SFXCOMPILER) : $(LIBGG) $(LIBSFX) $(SFXCOMPILER_DST)
	gcc -o $(SFXCOMPILER) $(SFXCOMPILER_DST) $(LIBRES) $(LIBSFX) $(LIBGG) -lboost_filesystem-mt -lboost_system-mt -lstdc++ $(CFLAGS)
	
%.o : %.cpp
	gcc -c $< -o $@ $(LIBRES_INCLUDE) $(CFLAGS) $(LIBGG_INCLUDE)

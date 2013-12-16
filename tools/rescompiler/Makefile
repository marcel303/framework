include ../../Makefile.defs

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

LIBRES = libres.a
LIBRES_SRC = ResIndex.cpp ResIndexCompiler.cpp
LIBRES_DST = $(LIBRES_SRC:.cpp=.o)

RESCOMPILER_SRC = rescompiler.cpp
RESCOMPILER_DST = $(RESCOMPILER_SRC:.cpp=.o)
RESCOMPILER = rescompiler$(EXECUTABLE_EXT)

all : $(RESCOMPILER)

all_clean : clean all

test : all
	./rescompiler -c resources.txt -o-index resources.idx -o-header resources.h -o-source resources.cpp -name Resources
	@-rm MakeResources.lst
	@echo "RESOURCES = \\" > MakeResources.lst
	@echo `./$(RESCOMPILER) -c resources.txt -l` >> MakeResources.lst
	@$(MAKE) -f MakeResources
	
clean :
	-rm *.o
	-rm *.a

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)

$(RESCOMPILER) : $(LIBRES) $(RESCOMPILER_DST) $(LIBGG)
	gcc -o $(RESCOMPILER) $(RESCOMPILER_DST) $(LIBRES) $(LIBGG) -lstdc++ $(CFLAGS)

$(LIBRES) : $(LIBRES_DST)
	ar -rcs $(LIBRES) $(LIBRES_DST)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

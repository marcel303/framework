include ../../Makefile.defs

LIBGG = ../../prototypes/libgg/libgg.a
LIBGG_INCLUDE = -I../../prototypes/libgg

FONTGEN_SRC = font.cpp fontgen.cpp
FONTGEN_DST = $(FONTGEN_SRC:.cpp=.o)
FONTGEN = fontgen$(EXECUTABLE_EXT)

all : $(FONTGEN)

all_clean : clean all

test : all
	./fontgen
	cat *.vg > test.tmp
	../vecrend/vecview test.tmp

clean :
	-rm *.o
	-rm *.a

$(FONTGEN) : $(LIBRES) $(FONTGEN_DST)
	gcc -o $(FONTGEN) $(FONTGEN_DST) $(LIBGG) -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ $(CCFLAGS) $(LIBGG_INCLUDE)
	
# -Werror


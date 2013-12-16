include ../../Makefile.defs

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

PSPFIXDDS_SRC = psp_fixdds.cpp
PSPFIXDDS_DST = $(PSPFIXDDS_SRC:.cpp=.o)
PSPFIXDDS = psp_fixdds$(EXECUTABLE_EXT)

all : $(PSPFIXDDS)

all_clean : clean all

clean :
	-rm *.o
	-rm *.a

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)

$(PSPFIXDDS) : $(PSPFIXDDS_DST) $(LIBGG)
	gcc -o $(PSPFIXDDS) $(PSPFIXDDS_DST) $(LIBGG) -lboost_filesystem-mt -lboost_system-mt -lstdc++ $(CFLAGS)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

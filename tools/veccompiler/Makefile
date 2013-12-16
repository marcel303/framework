include ../../Makefile.defs

VECCOMPILER_VC = veccompiler-vc$(EXECUTABLE_EXT)

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

LIBVEC = ../../prototypes/vecrend/libvec.a
LIBVEC_INCLUDE = -I../../prototypes/rescompiler

VECCOMPILER_VC_SRC = \
	CompiledComposition.cpp \
	CompositionCompiler.cpp \
	CompositionIO.cpp \
	VecComposition.cpp \
	veccompile-vc.cpp

VECCOMPILER_VC_DST = $(VECCOMPILER_VC_SRC:.cpp=.o)

all : $(VECCOMPILER_VC)

clean :
	-rm *.o


$(VECCOMPILER_VC) : $(LIBGG) $(LIBVEC) $(VECCOMPILER_VC_DST)
	gcc -o $(VECCOMPILER_VC) $(VECCOMPILER_VC_DST) $(LIBGG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++

$(LIBGG) : dummy
	cd ../../libgg && $(MAKE)

$(LIBVEC) : dummy
	cd ../../prototypes/vecrend && $(MAKE)

%.o : %.cpp
	gcc -c $< -o $@ $(LIBVEC_INCLUDE) $(CFLAGS) $(LIBGG_INCLUDE)

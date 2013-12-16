include ../../Makefile.defs

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

LIBVEC = veclib.a
LIBVEC_SRC = Buffer.cpp CompiledShape.cpp Scanner.cpp Shape.cpp ShapeCompiler.cpp ShapeIO.cpp Triangulate.cpp VecRend.cpp VecRend_Bezier.cpp VecRend_FreeImage.cpp
LIBVEC_DST = $(LIBVEC_SRC:.cpp=.o)

VECCOMPILE_SRC = veccompile.cpp
VECCOMPILE_DST = $(VECCOMPILE_SRC:.cpp=.o)
VECCOMPILE = veccompile$(EXECUTABLE_EXT)

VECVIEW_SRC = vecview.cpp VecRend_Allegro.cpp
VECVIEW_DST = $(VECVIEW_SRC:.cpp=.o)
VECVIEW = vecview$(EXECUTABLE_EXT)

all : $(VECCOMPILE)
# $(VECVIEW)

all_clean : clean $(VECCOMPILE)

clean :
	-rm *.o
	-rm *.a

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)

$(LIBVEC) : $(LIBVEC_DST)
	ar -rcs $(LIBVEC) $(LIBVEC_DST)

$(VECCOMPILE) : $(VECCOMPILE_DST) $(LIBGG) $(LIBVEC)
	gcc -o $(VECCOMPILE) $(VECCOMPILE_DST) $(LIBVEC) $(LIBGG) -lfreeimage -lstdc++

$(VECVIEW) : $(VECVIEW_DST) $(LIBGG) $(LIBVEC)
	gcc -o $(VECVIEW) $(VECVIEW_DST) $(LIBVEC) $(LIBGG) -lfreeimage $(LIBALLEG) -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

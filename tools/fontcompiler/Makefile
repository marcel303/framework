include ../../Makefile.defs

FONTCOMPILER = fontcompiler$(EXECUTABLE_EXT)

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

LIBRES = ../rescompiler/libres.a
LIBRES_INCLUDE = -I../rescompiler

LIBATL = ../atlcompiler/libatl.a
LIBATL_INCLUDE = -I../atlcompiler

FONTCOMPILER_SRC = \
	CompiledFont.cpp \
	Font.cpp \
	FontCompiler.cpp \
	fontcompile.cpp

FONTCOMPILER_DST = $(FONTCOMPILER_SRC:.cpp=.o)

all : $(FONTCOMPILER)

test : $(FONTCOMPILER)
	./fontcompiler -c ../../usg/content/Art/font_calibri.png -o blaat -r 32 127 -s 300 270

clean :
	-rm *.o

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)

$(LIBRES) : dummy
	cd ../rescompiler/ && $(MAKE)
	
$(LIBATL) : dummy
	cd ../atlcompiler/ && $(MAKE)
	
$(FONTCOMPILER) : $(LIBGG) $(LIBRES) $(LIBATL) $(FONTCOMPILER_DST)
	gcc -o $(FONTCOMPILER) $(FONTCOMPILER_DST) $(LIBATL) $(LIBRES) $(LIBGG) -lFreeImage -lboost_filesystem-mt -lboost_system-mt -lstdc++
	
%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBATL_INCLUDE) $(LIBRES_INCLUDE) $(LIBGG_INCLUDE)

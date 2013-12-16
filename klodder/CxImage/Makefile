include ../../Makefile.defs

LIBCX_SRC = ximage.cpp ximainfo.cpp ximaint.cpp ximalpha.cpp ximapal.cpp ximath.cpp ximatran.cpp
LIBCX_DST = $(LIBCX_SRC:.cpp=.o)
LIBCX = libcx.a

all : $(LIBCX)

all_clean : clean all

clean :
	-rm *.o
	-rm *.a

$(LIBCX) : $(LIBCX_DST)
	ar -rcs $(LIBCX) $(LIBCX_DST)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

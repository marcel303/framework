include ../../Makefile.defs

LIBGG = ../../libgg/libgg.a
LIBGG_INCLUDE = -I../../libgg

LIBPKG = libpkg.a
LIBPKG_SRC = CompiledPackage.cpp
LIBPKG_DST = $(LIBPKG_SRC:.cpp=.o)

PKGCOMPILER_SRC = PackageCompiler.cpp pkgcompiler.cpp
PKGCOMPILER_DST = $(PKGCOMPILER_SRC:.cpp=.o)
PKGCOMPILER = pkgcompiler$(EXECUTABLE_EXT)

all : $(PKGCOMPILER)

all_clean : clean all

clean :
	-rm *.o
	-rm *.a

$(LIBGG) : dummy
	cd ../../libgg/ && $(MAKE)

$(PKGCOMPILER) : $(LIBPKG) $(PKGCOMPILER_DST) $(LIBGG)
	gcc -o $(PKGCOMPILER) $(PKGCOMPILER_DST) $(LIBPKG) $(LIBGG) -lboost_filesystem-mt -lboost_system-mt -lstdc++ $(CFLAGS)

$(LIBPKG) : $(LIBPKG_DST)
	ar -rcs $(LIBPKG) $(LIBPKG_DST)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBGG_INCLUDE)

.NOTPARALLEL :

all : libgg svntool rescompiler pkgcompiler tgatool atlcompiler fontcompiler veccompile veccompile-vc

clean :
	cd libgg/ && $(MAKE) clean
	cd prototypes/vecrend/ && $(MAKE) clean
	cd tools/atlcompiler/ && $(MAKE) clean
	cd tools/fontcompiler/ && $(MAKE) clean
	cd tools/pkgcompiler/ && $(MAKE) clean
	cd tools/rescompiler/ && $(MAKE) clean
	cd tools/svntool/ && $(MAKE) clean
	cd tools/tgatool/ && $(MAKE) clean
	cd tools/veccompiler/ && $(MAKE) clean
	cd tools/psp_fixdds/ && $(MAKE) clean

libgg :
	cd libgg/ && $(MAKE)

fontcompiler :
	cd tools/fontcompiler/ && $(MAKE)

veccompile :
	cd prototypes/vecrend/ && $(MAKE)

veccompile-vc :
	cd tools/veccompiler/ && $(MAKE)

atlcompiler :
	cd tools/atlcompiler/ && $(MAKE)

rescompiler :
	cd tools/rescompiler/ && $(MAKE)

pkgcompiler :
	cd tools/pkgcompiler/ && $(MAKE)

tgatool :
	cd tools/tgatool/ && $(MAKE)

psp_fixdds :
	cd tools/psp_fixdds/ && $(MAKE)

svntool :
	cd tools/svntool/ && $(MAKE)

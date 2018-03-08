####################################################
#                                                  #
#     JGMOD makefile by Guan Foo Wah               #
#                                                  #
#    modified version for Linux, by George Foot    #
#    and Henrik Stokseth                           #
#    Ported to BEOS by Angelo Mottola              #
#                                                  #
####################################################

CPLIBDEST = /boot/develop/lib/x86
CPINCDEST = /boot/develop/headers/cpp

OBJ_DIR = ../obj/be

LIB_DIR = ../lib/be
STATIC_LIB = $(LIB_DIR)/libjgmod.a


include makefile.lst

CFLAGS = -O3 -W -Wno-unused -Wall -mcpu=pentium -ffast-math -funroll-loops


all : $(OBJ_LIST) $(STATIC_LIB) jgmod jgm
	@echo Done. 
	@echo To compress the executables, type \`make compress\' now
	@echo To install the executables, type \`make install\' as root
	@echo Please read readme.txt

install:
	cp $(STATIC_LIB) $(CPLIBDEST)
	cp jgmod.h $(CPINCDEST)
	@echo Please read readme.txt

include ../obj/be/makefile.dep


$(OBJ_DIR)/%.o: %.c
	gcc $(CFLAGS) -o $@ -c $<


$(STATIC_LIB) : $(OBJ_LIST)
	ar rs $(STATIC_LIB) $(OBJ_LIST)

jgmod : jgmod.c $(STATIC_LIB)
	gcc jgmod.c -o jgmod -s -L../lib/be -ljgmod `allegro-config --libs`

jgm : jgm.c $(STATIC_NAME)
	gcc jgm.c -o jgm -s -L../lib/be -ljgmod `allegro-config --libs`


clean :
	rm $(OBJ_DIR)/*.o 
	rm $(STATIC_LIB)
	rm jgmod
	rm jgm


veryclean :
	rm $(OBJ_DIR)/*.o 
	rm $(STATIC_LIB)
	rm jgmod
	rm jgm
	rm $(CPLIBDEST)/libjgmod.so
	rm $(CPINCDEST)/jgmod.h


compress: jgmod jgm
    ifneq ($(DJP),)
	@$(DJP) jgmod jgm
	@echo Done. 
    else
	@echo No executable compressor found! This target requires either the
	@echo DJP or UPX utilities to be installed.
    endif
	@echo To install the executables, type \`make install\' as root
	@echo Please read readme.txt


CFLAGS = -I/usr/local/include -DNO_SDL_GLEXT -g

LFLAGS += `sdl-config --static-libs`
LFLAGS += -framework OpenGL -lc++

SRC = \
	static/AUDIODCT.c \
	static/AUDIOHHD.c \
	static/CONTEXT.c \
	static/EGADICT.c \
	static/EGAHEAD.c \
	static/GAMETEXT.c \
	static/MAPDICT.c \
	static/MAPHEAD.c \
	static/STORY.c \
	opl/opl.c \
	gelib.c \
	id_ca.c \
	id_in.c \
	id_rf_routines.c \
	id_rf.c \
	id_sd.c \
	id_us_routines.c \
	id_us.c \
	id_vw_routines.c \
	id_vw.c \
	jam_io.c \
	kd_act1.c \
	kd_act2.c \
	kd_demo.c \
	kd_keen.c \
	kd_main.c \
	kd_play.c \
	lzhuf.c \
	shifttable.c \
	soft.c \
	stubs.c \
	syscode.c

OBJ = $(SRC:.c=.o)

BIN2C = tools/bin2c

KDREAMS = KDREAMS

all : $(KDREAMS)
	echo done

$(BIN2C) : tools/bin2c.o
	gcc -o $(BIN2C) tools/bin2c.o -lstdc++

.PHONY : dummy

STATIC : $(BIN2C) dummy
	$(BIN2C) static/AUDIODCT.KDR static/AUDIODCT audiodict
	$(BIN2C) static/AUDIOHHD.KDR static/AUDIOHHD audiohead
	$(BIN2C) static/CONTEXT.KDR static/CONTEXT context
	$(BIN2C) static/EGADICT.KDR static/EGADICT EGAdict
	$(BIN2C) static/EGAHEAD.KDR static/EGAHEAD EGAhead
	$(BIN2C) static/GAMETEXT.KDR static/GAMETEXT gametext
	$(BIN2C) static/MAPDICT.KDR static/MAPDICT mapdict
	$(BIN2C) static/MAPHEAD.KDR static/MAPHEAD maphead
	$(BIN2C) static/STORY.KDR static/STORY story

$(KDREAMS) : STATIC $(OBJ)
	gcc -o KDREAMS $(OBJ) $(LFLAGS)
	cd game && lldb ../KDREAMS

%.o : %.c
	gcc $(CFLAGS) -c $< -o $@

%.o : %.cpp
	gcc $(CFLAGS) -c $< -o $@
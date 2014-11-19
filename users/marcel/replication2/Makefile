include ../../../Makefile.defs

INCLUDE = -I"."
INCLUDE += -I"./Event/"
INCLUDE += -I"./Game/"
INCLUDE += -I"./Graphics/"
INCLUDE += -I"./Res/"
INCLUDE += -I"./Sound/"
INCLUDE += -I"./Util/"

INCLUDE += -I"../../../libgg/"
INCLUDE += -I"../../../libnet/"
INCLUDE += -I/usr/local/include

#CFLAGS += -g
CFLAGS += -O2 -std=c++11
CFLAGS += -DNO_SDL_GLEXT
CFLAGS += -Wall -Werror

REPLICATION_SRC = $(wildcard *.cpp)
REPLICATION_SRC += $(wildcard ./Event/*.cpp)
REPLICATION_SRC += $(wildcard ./Game/*.cpp)
REPLICATION_SRC += $(wildcard ./Graphics/*.cpp)
REPLICATION_SRC += $(wildcard ./Res/*.cpp)
REPLICATION_SRC += $(wildcard ./Sound/*.cpp)
REPLICATION_SRC += $(wildcard ./Util/*.cpp)

REPLICATION_SRC := $(filter-out ./Game/Main.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Graphics/DisplayWindows.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Graphics/GraphicsDeviceD3D9.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Graphics/GraphicsDeviceD3D11.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Graphics/GraphicsDeviceGL.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Res/ResLoaderSnd.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Res/ResLoaderTex.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out ./Sound/SoundDevice.cpp, $(REPLICATION_SRC))
REPLICATION_SRC := $(filter-out SystemDefault.cpp, $(REPLICATION_SRC))

REPLICATION_DST = $(REPLICATION_SRC:.cpp=.o)
REPLICATION = replication2.exe

all : deps $(REPLICATION)

all_clean : clean all

clean :
	-rm $(REPLICATION_DST)
	-rm $(REPLICATION)

deps : .PHONY
	#cd ../../../libgg && $(MAKE)
	#cd ../../../libnet && $(MAKE)

$(REPLICATION) : $(REPLICATION_DST)
	gcc -o $(REPLICATION) $(REPLICATION_DST) ../../../libnet/libnet.a ../../../libgg/libgg.a `sdl-config --static-libs` -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(INCLUDE)
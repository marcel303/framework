include ../Makefile.defs

INCLUDE = -I"../libgg/"
INCLUDE += -I/usr/local/include
INCLUDE += -I"./test/"

CFLAGS += -g

LIBNET_SRC = \
	BitStream.cpp \
	Channel.cpp \
	ChannelManager.cpp \
	NetAddress.cpp \
	NetSocket.cpp \
	NetStats.cpp \
	NetTimer.cpp \
	Packet.cpp \
	PacketDispatcher.cpp \
	PacketListener.cpp \
	RpcManager.cpp

LIBNET_SRC += \
	../libgg/BinaryDiff.cpp \
	../libgg/Hash.cpp \
	../libgg/Log.cpp \
	../libgg/PolledTimer.cpp \
	../libgg/Timer.cpp

LIBNET_DST = $(LIBNET_SRC:.cpp=.o)
LIBNET = libnet.a

LIBGG = ../libgg/libgg.a

TEST = test.exe

all : $(LIBNET) $(TEST)

all_clean : clean all

clean :
	-rm *.o
	-rm *.a

$(LIBNET) : $(LIBNET_DST)
	ar -rcs $(LIBNET) $(LIBNET_DST)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(INCLUDE)

$(TEST) : $(LIBNET) dummy
	gcc -c test.cpp -o test.o $(CFLAGS) $(INCLUDE)
#	gcc -o $(TEST) test.o $(LIBNET) $(LIBGG) -lwsock32 -lstdc++
#	gcc -o $(TEST) test.o $(LIBNET) $(LIBGG) `sdl-config --static-libs` -lstdc++
	gcc -o $(TEST) test.o $(LIBNET) `sdl-config --static-libs` -lstdc++

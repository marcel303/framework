include ../Makefile.defs

#CFLAGS = $(CFLAGS:-Werror=)
CFLAGS32 = $(CFLAGS) -m32
INCLUDE = -I"../libgg/"

LIBNET_SRC = \
	Address.cpp \
	Channel.cpp \
	ChannelManager.cpp \
	NetSocket.cpp \
	NetStats.cpp \
	NetTimer.cpp \
	Packet.cpp \
	PacketDispatcher.cpp \
	PacketListener.cpp

LIBNET_DST = $(LIBNET_SRC:.cpp=.o)
LIBNET_DST32 = $(LIBNET_SRC:.cpp=.o32)
LIBNET = libnet.a
LIBNET32 = libnet32.a

LIBGG = ../libgg/libgg.a

TEST = test.exe

all : $(LIBNET) $(LIBNET32) $(TEST)

all_clean : clean all

clean :
	-rm *.o
	-rm *.a

$(LIBNET) : $(LIBNET_DST)
	ar -rcs $(LIBNET) $(LIBNET_DST)

$(LIBNET32) : $(LIBNET_DST32)
	ar -rcs $(LIBNET32) $(LIBNET_DST32)

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(INCLUDE)

%.o32 : %.cpp
	gcc -c $< -o $@ $(CFLAGS32) $(INCLUDE)

$(TEST) : $(LIBNET) dummy
	gcc -c test.cpp -o test.o $(CFLAGS) $(INCLUDE)
#	gcc -o $(TEST) test.o $(LIBNET) $(LIBGG) -lwsock32 -lstdc++
	gcc -o $(TEST) test.o $(LIBNET) $(LIBGG) -lstdc++ -lSDL

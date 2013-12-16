include ../Makefile.defs

#CFLAGS = $(CFLAGS:-Werror=)
CFLAGS32 = $(CFLAGS) -m32

GGLIB_SRC = \
	AngleController.cpp \
	AnimTimer.cpp \
	Base64.cpp \
	Benchmark.cpp \
	BezierPath.cpp \
	BinaryDiff.cpp \
	ByteString.cpp \
	Calc.cpp \
	ColHashMap.cpp \
	DdsLoader.cpp \
	Debugging.cpp \
	DeltaTimer.cpp \
	Directory.cpp \
	Exception.cpp \
	FastList.cpp \
	FileStream.cpp \
	FileStreamExtends.cpp \
	Hash.cpp \
	Heap.cpp \
	IImageLoader.cpp \
	Image.cpp \
	ImageLoader_FreeImage.cpp \
	ImageLoader_Photoshop.cpp \
	ImageLoader_Tga.cpp \
	Log.cpp \
	Mat3x2.cpp \
	MatrixStack.cpp \
	MemAllocators.cpp \
	MemAllocators2.cpp \
	MemOps.cpp \
	MemoryStream.cpp \
	Parse.cpp \
	Path.cpp \
	PolledTimer.cpp \
	PsdChannel.cpp \
	PsdColorModeData.cpp \
	PsdCompression.cpp \
	PsdForward.cpp \
	PsdHeaderInfo.cpp \
	PsdImageData.cpp \
	PsdInfo.cpp \
	PsdLayer.cpp \
	PsdLayerAndMaskInfo.cpp \
	PsdRect.cpp \
	PsdResource.cpp \
	PsdResource_ChannelNames.cpp \
	PsdResource_DisplayInfo.cpp \
	PsdResourceList.cpp \
	PsdResource_ResolutionInfo.cpp \
	PsdTypes.cpp \
	Random.cpp \
	Stream.cpp \
	StreamReader.cpp \
	StreamWriter.cpp \
	StringBuilder.cpp \
	StringEx.cpp \
	SubStream.cpp \
	TextureDDS.cpp \
	TexturePVR.cpp \
	TextureRGBA.cpp \
	Timer.cpp \
	TimeTracker.cpp \
	Traveller.cpp \
	TravellerCapturer.cpp \
	TriggerTimer.cpp \
	Types.cpp \
	Util_Color.cpp

GGLIB_DST = $(GGLIB_SRC:.cpp=.o)
#GGLIB_DST32 = $(GGLIB_SRC:.cpp=.o32)
GGLIB = libgg.a
#GGLIB32 = libgg32.a

all : $(GGLIB)
# $(GGLIB32)

all_clean : clean all

clean :
	-rm *.o
	-rm *.a

$(GGLIB) : $(GGLIB_DST)
	ar -rcs $(GGLIB) $(GGLIB_DST)

#$(GGLIB32) : $(GGLIB_DST32)
#	ar -rcs $(GGLIB32) $(GGLIB_DST32)

test_tga : test_tga.o $(GGLIB)
	gcc -o test_tga test_tga.o $(GGLIB) -lFreeImage -lstdc++

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS)

#%.o32 : %.cpp
#	gcc -c $< -o $@ $(CFLAGS32)

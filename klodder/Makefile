include ../Makefile.defs

LIBGG = ../libgg/libgg.a
LIBGG_INCLUDE = -I../libgg
LIBCX = ./CxImage/libcx.a
LIBCX_INCLUDE = -I./CxImage
LIBAGG = ./libagg/libagg.a
LIBAGG_INCLUDE = -I./libagg

LIBKLODDER = libklodder.a
LIBKLODDER_SRC = \
	Classes/Brush_Pattern.cpp \
	Classes/BrushLibrary.cpp \
	Classes/Filter.cpp
LIBKLODDER_DST = $(LIBKLODDER_SRC:.cpp=.o)

#TOOL_IMG2FLT = img2flt/img2flt
#TOOL_BRUSHCOMPILER = brushcompiler/brushcompiler
TOOL_IMGCONVERT = ../tools/imgconvert/imgconvert

IMAGES_SRC0 =
#IMAGES_SRC0 = tool_erazor.psd tool_erazor_sel.psd
#IMAGES_SRC = $(addprefix content/Resources/, $(IMAGES_SRC0)
IMAGES_DST = $(addprefix Resources/, $(IMAGES_SRC0:.psd=.png))
#IMAGES_DST = $(IMAGES_SRC0:.psd=.png)

#	Classes/Main.cpp \
#	Classes/MouseMgr.cpp \
#	Classes/QuickTimeEncoder.cpp \
#	Classes/Traveller.cpp \
#	Classes/TravellerCapturer.cpp \

KLODDER = klodder.exe
KLODDER_SRC = \
	Classes/Application.cpp \
	Classes/Bezier.cpp \
	Classes/BezierTraveller.cpp \
	Classes/Bitmap.cpp \
	Classes/BlitTransform.cpp \
	Classes/Brush_Pattern.cpp \
	Classes/BrushLibrary.cpp \
	Classes/BrushLibrarySet.cpp \
	Classes/BrushSettingsLibrary.cpp \
	Classes/CommandPacket.cpp \
	Classes/CommandStream.cpp \
	Classes/DataStream.cpp \
	Classes/ExceptionLogger.cpp \
	Classes/FileArchive.cpp \
	Classes/Filter.cpp \
	Classes/ImageConversion.cpp \
	Classes/ImageDescription.cpp \
	Classes/ImageId.cpp \
	Classes/ImageResampling.cpp \
	Classes/ITool.cpp \
	Classes/KlodderSystem.cpp \
	Classes/LayerMgr.cpp \
	Classes/MacImage.cpp \
	Classes/Main.cpp \
	Classes/MouseMgr.cpp \
	Classes/Settings.cpp \
	Classes/Settings_FileSystem.cpp \
	Classes/StreamProvider.cpp \
	Classes/StreamProvider_Replay.cpp \
	Classes/Swatch.cpp \
	Classes/Tool_Blur.cpp \
	Classes/Tool_Brush.cpp \
	Classes/Tool_Smudge.cpp \
	Classes/TouchZoom.cpp \
	Classes/UndoStack.cpp \
	Classes/Util_Mem.cpp \
	Classes/XmlNode.cpp \
	Classes/XmlReader.cpp \
	Classes/XmlWriter.cpp \
	CxImage/ximage.cpp \
	CxImage/ximainfo.cpp \
	CxImage/ximaint.cpp \
	CxImage/ximalpha.cpp \
	CxImage/ximath.cpp \
	CxImage/ximatran.cpp \
	libtinyxml/tinystr.cpp \
	libtinyxml/tinyxml.cpp \
	libtinyxml/tinyxmlerror.cpp \
	libtinyxml/tinyxmlparser.cpp \
	../libiphone/TouchInfo.cpp

KLODDER_DST = $(KLODDER_SRC:.cpp=.o)
ifeq ($(OS), win)
KLODDER_LIBS = -lmingw32 -lSDLmain -lSDL -lFreeImage -lstdc++
else ifeq ($(OS), linux)
KLODDER_LIBS = -lSDLmain -lSDL -lFreeImage -lstdc++
else
KLODDER_LIBS = -lSDLmain -lSDL -lFreeImage -framework AppKit -lstdc++
endif

all : $(LIBKLODDER) $(KLODDER)

all_clean : clean all

content : tools
# $(IMAGES_DST)
#	$(TOOL_IMG2FLT) -c Resources/brush01.psd Resources/brush02.psd
#	#brushlibcompiler/brushlibcompiler -c `brushcompiler/brushcompiler -c brushes.txt -l` -o Resources/standard.brushes
#	cd brushcompiler && $(MAKE) brushes
	cd content && $(MAKE)

tools : dummy
	cd brushcompiler && $(MAKE)
#	cd brushlibcompiler && $(MAKE)
#	cd img2flt && $(MAKE)

clean :
	-cd Classes && rm *.o
	-cd Classes && rm *.a
	-cd CxImage && rm *.o
	-cd CxImage && rm *.a
	-cd libagg && rm *.o
	-cd libagg && rm *.a
	-cd libcx && rm *.o
	-cd libcx && rm *.a
	-cd libtinyxml && rm *.o
	-cd libtinyxml && rm *.a
	-rm *.o
	-rm *.a

$(LIBGG) : dummy
	cd ../libgg && $(MAKE)

$(LIBCX) : dummy
	cd ./CxImage/ && $(MAKE)

$(LIBAGG) : dummy
	cd ./libagg/ && $(MAKE)

$(LIBKLODDER) : dummy $(LIBCX) $(LIBAGG) $(LIBGG) $(LIBKLODDER_DST)
	ar -rcs $(LIBKLODDER) $(LIBKLODDER_DST) $(LIBCX) $(LIBAGG) $(LIBGG)

$(KLODDER) : $(KLODDER_DST)
	gcc -o $(KLODDER) $(KLODDER_DST) $(LIBCX) $(LIBAGG) $(LIBGG) $(KLODDER_LIBS) -s

Classes/Filter.o : Classes/Filter.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBCX_INCLUDE) $(LIBAGG_INCLUDE) $(LIBGG_INCLUDE) -Wno-strict-aliasing -I../usg/Classes -I../libgg -I../libiphone -Ilibtinyxml -DKLODDER_LITE=0 -DMACOS -DSETTINGS_FILESYSTEM -DUSE_CXIMAGE -DUSE_AGG -DFILTER_STANDALONE -DDEPLOYMENT

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(LIBCX_INCLUDE) $(LIBAGG_INCLUDE) $(LIBGG_INCLUDE) -Wno-strict-aliasing -I../usg/Classes -I../libgg -I../libiphone -Ilibtinyxml -DKLODDER_LITE=0 -DMACOS -DSETTINGS_FILESYSTEM -DUSE_CXIMAGE -DUSE_AGG -DDEPLOYMENT

Resources/%.png : content/Resources/%.psd
	$(TOOL_IMGCONVERT) -c $< -o $@

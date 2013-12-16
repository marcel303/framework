TOOL_TEXTURETOOL = /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/texturetool
CHANNEL_WEIGHTING = --channel-weighting-perceptual

all : ovl1.pvr4 ovl2.pvr4 ind.pvr4

%.pvr4 : %.png
	$(TOOL_TEXTURETOOL) -f PVR -m -e PVRTC --bits-per-pixel-4 $(CHANNEL_WEIGHTING) -o $@ $<
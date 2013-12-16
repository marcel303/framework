TEXTURETOOL = /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/texturetool
CHANNEL_WEIGHTING = --channel-weighting-perceptual
#CHANNEL_WEIGHTING = --channel-weighting-linear

all : pvr

pvr :
	$(TEXTURETOOL) -f PVR -m -e PVRTC --bits-per-pixel-2 $(CHANNEL_WEIGHTING) -o texture_pvr_rgb2.pvr texture_pvr_rgb2.png
	$(TEXTURETOOL) -f PVR -m -e PVRTC --bits-per-pixel-4 $(CHANNEL_WEIGHTING) -o texture_pvr_rgb4.pvr texture_pvr_rgb4.png
	$(TEXTURETOOL) -f PVR -m -e PVRTC --bits-per-pixel-2 $(CHANNEL_WEIGHTING) -o texture_pvr_rgba2.pvr texture_pvr_rgba2.png
	$(TEXTURETOOL) -f PVR -m -e PVRTC --bits-per-pixel-4 $(CHANNEL_WEIGHTING) -o texture_pvr_rgba4.pvr texture_pvr_rgba4.png

#include "vfxNodeVideo.h"
#include "../avpaint/video.h"

VfxNodeVideo::VfxNodeVideo()
	: VfxNodeBase()
	, image(nullptr)
	, mediaPlayer(nullptr)
{
	image = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_Image, image);
	
	mediaPlayer = new MediaPlayer();
	mediaPlayer->openAsync("video6.mpg", false);
}

VfxNodeVideo::~VfxNodeVideo()
{
	delete mediaPlayer;
	mediaPlayer = nullptr;
	
	delete image;
	image = nullptr;
}

void VfxNodeVideo::tick(const float dt)
{
	mediaPlayer->tick(mediaPlayer->context);
	
	if (mediaPlayer->context->hasBegun)
		mediaPlayer->presentTime += dt;
	
	image->texture = mediaPlayer->getTexture();
}

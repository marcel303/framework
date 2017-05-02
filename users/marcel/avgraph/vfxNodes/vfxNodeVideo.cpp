#include "vfxNodeVideo.h"
#include "../avpaint/video.h"

VfxNodeVideo::VfxNodeVideo()
	: VfxNodeBase()
	, image(nullptr)
	, mediaPlayer(nullptr)
{
	image = new VfxImage_Texture();
	
	mediaPlayer = new MediaPlayer();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addInput(kInput_Transform, kVfxPlugType_Transform);
	addOutput(kOutput_Image, kVfxPlugType_Image, image);
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
	if (mediaPlayer->isActive(mediaPlayer->context))
	{
		mediaPlayer->tick(mediaPlayer->context);
		
		if (mediaPlayer->context->hasBegun)
			mediaPlayer->presentTime += dt;
		
		image->texture = mediaPlayer->getTexture();
	}
}

void VfxNodeVideo::init(const GraphNode & node)
{
	const std::string source = getInputString(kInput_Source, "");
	
	if (!source.empty())
	{
		mediaPlayer->openAsync(source.c_str(), false);
	}
}

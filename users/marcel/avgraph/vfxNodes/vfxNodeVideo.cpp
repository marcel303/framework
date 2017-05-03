#include "vfxNodeVideo.h"
#include "../avpaint/video.h"

VfxNodeVideo::VfxNodeVideo()
	: VfxNodeBase()
	, image(nullptr)
	, mediaPlayer(nullptr)
	, textureBlack(0)
{
	image = new VfxImage_Texture();
	
	mediaPlayer = new MediaPlayer();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addInput(kInput_Transform, kVfxPlugType_Transform);
	addOutput(kOutput_Image, kVfxPlugType_Image, image);
	
	uint32_t pixels[4] = { 0, 0, 0, 0 };
	textureBlack = createTextureFromRGBA8(pixels, 1, 1, false, true);
}

VfxNodeVideo::~VfxNodeVideo()
{
	glDeleteTextures(1, &textureBlack);
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
	
	delete image;
	image = nullptr;
}

void VfxNodeVideo::tick(const float dt)
{
	if (mediaPlayer->isActive(mediaPlayer->context))
	{
		const char * filename = getInputString(kInput_Source, "");
		
		if (mediaPlayer->context->openParams.filename != filename)
		{
			mediaPlayer->close();
			
			mediaPlayer->presentTime = 0.f;
			
			mediaPlayer->openAsync(filename, false);
		}
		
		mediaPlayer->tick(mediaPlayer->context);
		
		if (mediaPlayer->context->hasBegun)
			mediaPlayer->presentTime += dt;
		
		image->texture = mediaPlayer->getTexture();
	}
	
	if (image->texture == 0)
	{
		image->texture = textureBlack;
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

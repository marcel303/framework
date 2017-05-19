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
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_Speed, kVfxPlugType_Float);
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
	const bool loop = getInputBool(kInput_Loop, true);
	const float speed = getInputFloat(kInput_Speed, 1.f);
	
	if (mediaPlayer->isActive(mediaPlayer->context))
	{
		const char * source = getInputString(kInput_Source, "");
		
		if (mediaPlayer->context->openParams.filename != source || (mediaPlayer->context->hasPresentedLastFrame && loop))
		{
			mediaPlayer->close(false);
			
			mediaPlayer->presentTime = 0.f;
			
			mediaPlayer->openAsync(source, false);
		}
		
		mediaPlayer->tick(mediaPlayer->context);
		
		if (mediaPlayer->context->hasBegun)
			mediaPlayer->presentTime += dt * speed;
		
		image->texture = mediaPlayer->getTexture();
	}
	else
	{
		const char * source = getInputString(kInput_Source, "");
		
		if (source[0])
		{
			mediaPlayer->openAsync(source, false);
		}
	}
	
	if (image->texture == 0)
	{
		image->texture = textureBlack;
	}
}

void VfxNodeVideo::init(const GraphNode & node)
{
	const char * source = getInputString(kInput_Source, "");
	
	if (source[0])
	{
		mediaPlayer->openAsync(source, false);
	}
}

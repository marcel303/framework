#include "vfxNodeVideo.h"
#include "../avpaint/video.h"
#include "mediaplayer_new/MPVideoBuffer.h"

VfxNodeVideo::VfxNodeVideo()
	: VfxNodeBase()
	, imageOutput()
	, imageCpuOutput()
	, mediaPlayer(nullptr)
	, textureBlack(0)
{
	mediaPlayer = new MediaPlayer();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addInput(kInput_Transform, kVfxPlugType_Transform);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_Speed, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	addOutput(kOutput_ImageCpu, kVfxPlugType_ImageCpu, &imageCpuOutput);
	
	uint32_t pixels[4] = { 0, 0, 0, 0 };
	textureBlack = createTextureFromRGBA8(pixels, 1, 1, false, true);
}

VfxNodeVideo::~VfxNodeVideo()
{
	glDeleteTextures(1, &textureBlack);
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
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
		
		imageOutput.texture = mediaPlayer->getTexture();
	}
	else
	{
		const char * source = getInputString(kInput_Source, "");
		
		if (source[0])
		{
			mediaPlayer->openAsync(source, false);
		}
	}
	
	if (mediaPlayer->videoFrame != nullptr)
	{
		imageCpuOutput.setDataRGBA8((uint8_t*)mediaPlayer->videoFrame->m_frameBuffer, mediaPlayer->videoFrame->m_width, mediaPlayer->videoFrame->m_height, 0);
	}
	else
	{
		imageCpuOutput.reset();
	}
	
	if (imageOutput.texture == 0)
	{
		imageOutput.texture = textureBlack;
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

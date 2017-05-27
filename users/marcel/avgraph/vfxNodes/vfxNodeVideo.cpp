#include "framework.h"
#include "vfxNodeVideo.h"
#include "../avpaint/video.h"
#include "mediaplayer_new/MPVideoBuffer.h"

VfxNodeVideo::VfxNodeVideo()
	: VfxNodeBase()
	, imageOutput()
	, imageCpuOutputRGBA()
	, imageCpuOutputY()
	, imageCpuOutputU()
	, imageCpuOutputV()
	, mediaPlayer(nullptr)
	, textureBlack(0)
{
	mediaPlayer = new MediaPlayer();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_Speed, kVfxPlugType_Float);
	addInput(kInput_OutputMode, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	addOutput(kOutput_ImageCpuRGBA, kVfxPlugType_ImageCpu, &imageCpuOutputRGBA);
	addOutput(kOutput_ImageCpuY, kVfxPlugType_ImageCpu, &imageCpuOutputY);
	addOutput(kOutput_ImageCpuU, kVfxPlugType_ImageCpu, &imageCpuOutputU);
	addOutput(kOutput_ImageCpuV, kVfxPlugType_ImageCpu, &imageCpuOutputV);
	
	uint32_t pixels[4] = { 0, 0, 0, 0 };
	textureBlack = createTextureFromRGBA8(pixels, 1, 1, false, true);
}

VfxNodeVideo::~VfxNodeVideo()
{
	glDeleteTextures(1, &textureBlack);
	textureBlack = 0;
	checkErrorGL();
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
}

void VfxNodeVideo::tick(const float dt)
{
	const bool loop = getInputBool(kInput_Loop, true);
	const float speed = getInputFloat(kInput_Speed, 1.f);
	const MP::OutputMode outputMode = (MP::OutputMode)getInputInt(kInput_OutputMode, MP::kOutputMode_RGBA);
	
	imageCpuOutputRGBA.reset();
	imageCpuOutputY.reset();
	imageCpuOutputU.reset();
	imageCpuOutputV.reset();
	
	if (mediaPlayer->isActive(mediaPlayer->context))
	{
		const char * source = getInputString(kInput_Source, "");
		
		const bool paramsChanged =
			mediaPlayer->context->openParams.filename != source ||
			(mediaPlayer->context->hasPresentedLastFrame && loop) ||
			mediaPlayer->context->openParams.outputMode != outputMode;
		
		if (paramsChanged)
		{
			// todo : media player should re-allocate texture when output mode has changed
			
			mediaPlayer->close(false);
			
			mediaPlayer->presentTime = 0.f;
			
			mediaPlayer->openAsync(source, outputMode);
		}
		
		mediaPlayer->tick(mediaPlayer->context);
		
		if (mediaPlayer->context->hasBegun)
			mediaPlayer->presentTime += dt * speed;
		
		//
		
		imageOutput.texture = mediaPlayer->getTexture();
		
		//
		
		if (mediaPlayer->videoFrame != nullptr)
		{
			Assert(mediaPlayer->videoFrame->m_isValidForRead);
			
			if (mediaPlayer->context->openParams.outputMode == MP::kOutputMode_PlanarYUV)
			{
				int ySx;
				int ySy;
				int yPitch;
				const uint8_t * yBytes = mediaPlayer->videoFrame->getY(ySx, ySy, yPitch);
				
				int uSx;
				int uSy;
				int uPitch;
				const uint8_t * uBytes = mediaPlayer->videoFrame->getU(uSx, uSy, uPitch);
				
				int vSx;
				int vSy;
				int vPitch;
				const uint8_t * vBytes = mediaPlayer->videoFrame->getV(vSx, vSy, vPitch);
				
				imageCpuOutputY.setDataR8(yBytes, ySx, ySy, 4, yPitch);
				imageCpuOutputU.setDataR8(uBytes, uSx, uSy, 4, uPitch);
				imageCpuOutputV.setDataR8(vBytes, vSx, vSy, 4, vPitch);
			}
			else if (mediaPlayer->context->openParams.outputMode == MP::kOutputMode_RGBA)
			{
				imageCpuOutputRGBA.setDataRGBA8(
					(uint8_t*)mediaPlayer->videoFrame->m_frameBuffer,
					mediaPlayer->videoFrame->m_width,
					mediaPlayer->videoFrame->m_height, 4, 0);
			}
		}
	}
	else
	{
		const char * source = getInputString(kInput_Source, "");
		
		if (source[0])
		{
			mediaPlayer->openAsync(source, MP::kOutputMode_RGBA);
		}
	}

	if (imageOutput.texture == 0)
	{
		imageOutput.texture = textureBlack;
	}
}

void VfxNodeVideo::draw() const
{
	// todo : remove once decode buffer optimize is free of bugs
	
	if (mediaPlayer->videoFrame)
	{
		Assert(mediaPlayer->videoFrame->m_isValidForRead);
	}
}

void VfxNodeVideo::init(const GraphNode & node)
{
	const char * source = getInputString(kInput_Source, "");
	const MP::OutputMode outputMode = (MP::OutputMode)getInputInt(kInput_OutputMode, MP::kOutputMode_RGBA);
	
	if (source[0])
	{
		mediaPlayer->openAsync(source, outputMode);
	}
}

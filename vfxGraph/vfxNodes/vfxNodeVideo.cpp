/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "vfxNodeVideo.h"
#include "../libvideo/video.h"
#include "mediaplayer/MPVideoBuffer.h"

VFX_ENUM_TYPE(videoOutputMode)
{
	elem("rgba");
	elem("yuv");
}

VFX_NODE_TYPE(VfxNodeVideo)
{
	typeName = "video";
	
	in("source", "string");
	in("loop", "bool", "1");
	in("speed", "float", "1");
	inEnum("mode", "videoOutputMode");
	out("image", "image");
	out("mem.rgb", "image_cpu");
	out("mem.y", "image_cpu");
	out("mem.u", "image_cpu");
	out("mem.v", "image_cpu");
}

VfxNodeVideo::VfxNodeVideo()
	: VfxNodeBase()
	, imageOutput()
	, imageCpuOutputRGBA()
	, imageCpuOutputY()
	, imageCpuOutputU()
	, imageCpuOutputV()
	, mediaPlayer(nullptr)
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
}

VfxNodeVideo::~VfxNodeVideo()
{
	delete mediaPlayer;
	mediaPlayer = nullptr;
}

void VfxNodeVideo::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeVideo);
	
	if (isPassthrough)
	{
		mediaPlayer->close(true);
		mediaPlayer->presentTime = 0.f;
		
		imageOutput.texture = 0;
		
		rgbaData.free();
		
		imageCpuOutputRGBA.reset();
		imageCpuOutputY.reset();
		imageCpuOutputU.reset();
		imageCpuOutputV.reset();
		
		return;
	}
	
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
			mediaPlayer->context->openParams.outputMode != outputMode;
		
		const bool hasLooped = mediaPlayer->context->hasPresentedLastFrame && loop;
		
		if (paramsChanged)
		{
			mediaPlayer->close(true);
			mediaPlayer->presentTime = 0.f;
			
			mediaPlayer->openAsync(source, outputMode);
		}
		else if (hasLooped)
		{
			mediaPlayer->close(false);
			mediaPlayer->presentTime = 0.f;
			
			mediaPlayer->openAsync(source, outputMode);
		}
		
		Assert(mediaPlayer->context->openParams.outputMode == outputMode);
		
		const bool wantsTexture = outputs[kOutput_Image].isReferenced();
		
		mediaPlayer->tick(mediaPlayer->context, wantsTexture);
		
		if (mediaPlayer->context->hasBegun && isPassthrough == false)
		{
			mediaPlayer->presentTime += dt * speed;
		}
		
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
				
				imageCpuOutputY.setDataR8(yBytes, ySx, ySy, 16, yPitch);
				imageCpuOutputU.setDataR8(uBytes, uSx, uSy, 16, uPitch);
				imageCpuOutputV.setDataR8(vBytes, vSx, vSy, 16, vPitch);
			}
			else if (mediaPlayer->context->openParams.outputMode == MP::kOutputMode_RGBA)
			{
				// check if the output is connected. otherwise the conversion to planar can be avoided
				
				// todo : investigate adding a planar RGB output mode media player
				
				const VfxPlug * output = tryGetOutput(kOutput_ImageCpuRGBA);
				
				if (output->isReferenced())
				{
					int sx;
					int sy;
					int pitch;
					const uint8_t * bytes = mediaPlayer->videoFrame->getRGBA(sx, sy, pitch);
					
					rgbaData.allocOnSizeChange(sx, sy, 4);
					
					VfxImageCpu::deinterleave4(
						bytes, sx, sy, 16, pitch,
						rgbaData.image.channel[0],
						rgbaData.image.channel[1],
						rgbaData.image.channel[2],
						rgbaData.image.channel[3]);
					
					imageCpuOutputRGBA = rgbaData.image;
				}
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
	
	if (imageCpuOutputRGBA.numChannels == 0)
	{
		rgbaData.free();
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

void VfxNodeVideo::getDescription(VfxNodeDescription & d)
{
	if (mediaPlayer->isActive(mediaPlayer->context))
	{
		int sx;
		int sy;
		double duration;
		
		if (mediaPlayer->getVideoProperties(sx, sy, duration))
		{
			d.add("file: %s. size: %d x %d", mediaPlayer->context->openParams.filename.c_str(), sx, sy);
			
			const int dh = duration / 3600.0;
			duration -= dh * 3600;
			const int dm = duration / 60.0;
			duration -= dm * 60;
			
			double presentTime = mediaPlayer->presentTime;
			const int ph = presentTime / 3600.0;
			presentTime -= dh * 3600;
			const int pm = presentTime / 60.0;
			presentTime -= pm * 60;
			
			d.add("time: %02d:%02d:%05.2fs / %02d:%02d:%05.2fs", ph, pm, presentTime, dh, dm, duration);
		}
		else
		{
			d.add("file: %s", mediaPlayer->context->openParams.filename.c_str());
		}
	}
	else
	{
		d.add("file: (not active)");
	}
	d.newline();
	
	d.addOpenglTexture("texture", mediaPlayer->getTexture());
	d.newline();
	
	d.add("RGBA image", imageCpuOutputRGBA);
	d.newline();
	
	d.add("Y image", imageCpuOutputY);
	d.newline();
	
	d.add("U image", imageCpuOutputU);
	d.newline();
	
	d.add("V image", imageCpuOutputV);
}

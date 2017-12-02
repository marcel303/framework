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

#import "framework.h"
#import "vfxGraph.h"
#import "vfxNodeDrawSyphon.h"

VFX_NODE_TYPE(VfxNodeDrawSyphon)
{
	typeName = "draw.syphon";
	
	in("image", "image");
	out("any", "draw", "draw");
}

VfxNodeDrawSyphon::VfxNodeDrawSyphon()
	: VfxNodeBase()
	, server(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, this);
	
	auto context = CGLGetCurrentContext();
	
	server = [[SyphonServer alloc] initWithName:@"My Output" context:context options:nil];
	checkErrorGL();
}

VfxNodeDrawSyphon::~VfxNodeDrawSyphon()
{
	[server stop];
	checkErrorGL();
	
	[server release];
	server = nil;
	checkErrorGL();
}

void VfxNodeDrawSyphon::draw() const
{
	if (isPassthrough)
		return;

	vfxCpuTimingBlock(VfxNodeDrawSyphon);

	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	
	if (image != nullptr)
	{
		vfxGpuTimingBlock(VfxNodeDrawSyphon);
		
		const int sx = image->getSx();
		const int sy = image->getSy();
		
	#if 1
		pushSurface(g_currentVfxSurface);
		{
			[server bindToDrawFrameOfSize:NSMakeSize(sx, sy)];
			checkErrorGL();
			{
				glViewport(0, 0, sx, sy);
				gxMatrixMode(GL_MODELVIEW);
				gxLoadIdentity();
				gxMatrixMode(GL_PROJECTION);
				gxLoadIdentity();
				
				gxSetTexture(image->getTexture());
				{
					pushBlend(BLEND_OPAQUE);
					{
						drawRect(-1, +1, +1, -1);
					}
					popBlend();
				}
				gxSetTexture(0);
			}
			[server unbindAndPublish];
			checkErrorGL();
		}
		popSurface();
	#else
		// note : this would be the preferred method, but sadly it doesn't work with OpenGL 4.1+ we're using
		[server publishFrameTexture:image->getTexture() textureTarget:GL_TEXTURE_RECTANGLE_EXT imageRegion:NSMakeRect(0, 0, sx, sy) textureDimensions:NSMakeSize(sx, sy) flipped:NO];
		checkErrorGL();
	#endif
	}
}

/*
	Copyright (C) 2020 Marcel Smit
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

#if ENABLE_SYPHON

#import "framework.h"
#import "vfxGraph.h"
#import "vfxNodeDrawSyphon.h"

VFX_NODE_TYPE(VfxNodeDrawSyphon)
{
	typeName = "draw.syphon";
	
	in("image", "image");
	in("name", "string");
	out("any", "draw", "draw");
}

VfxNodeDrawSyphon::VfxNodeDrawSyphon()
	: VfxNodeBase()
	, server(nullptr)
	, currentName()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Name, kVfxPlugType_String);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
}

VfxNodeDrawSyphon::~VfxNodeDrawSyphon()
{
	[server stop];
	checkErrorGL();
	
	[server release];
	server = nil;
	checkErrorGL();
}

void VfxNodeDrawSyphon::tick(const float dt)
{
	const char * name = getInputString(kInput_Name, "");
	
	if (name != currentName)
	{
		if (server != nil)
		{
			[server stop];
			checkErrorGL();
			
			[server release];
			server = nil;
			checkErrorGL();
		}
		
		currentName = name;
		
		if (!currentName.empty())
		{
			auto context = CGLGetCurrentContext();
		
			server = [[SyphonServer alloc] initWithName:[NSString stringWithCString:name encoding:NSASCIIStringEncoding] context:context options:nil];
			checkErrorGL();
		}
	}
}

void VfxNodeDrawSyphon::draw() const
{
	if (isPassthrough)
		return;
	if (server == nil)
		return;

	vfxCpuTimingBlock(VfxNodeDrawSyphon);

	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	
	if (image != nullptr)
	{
		vfxGpuTimingBlock(VfxNodeDrawSyphon);
		
		const int sx = image->getSx();
		const int sy = image->getSy();
		
	#if 1 // note : take the inefficient code path here as it's the only one which seems to work on OpenGL 4.1+
		pushSurface(g_currentVfxSurface);
		{
			[server bindToDrawFrameOfSize:NSMakeSize(sx, sy)];
			checkErrorGL();
			{
				glViewport(0, 0, sx, sy);
				gxMatrixMode(GX_MODELVIEW);
				gxLoadIdentity();
				gxMatrixMode(GX_PROJECTION);
				gxLoadIdentity();
				
				gxSetTexture(image->getTexture());
				{
					pushBlend(BLEND_OPAQUE);
					{
						setColor(colorWhite);
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

#endif

void linkVfxNodes_Syphon()
{
}

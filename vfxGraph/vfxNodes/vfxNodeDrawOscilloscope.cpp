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

/*
The code here uses shaders that come from,
https://github.com/m1el/woscope

There is a very nice live demo of this code in action available here,
http://m1el.github.io/woscope/
*/

#include "framework.h"
#include "vfxGraph.h"
#include "vfxNodeDrawOscilloscope.h"

VFX_ENUM_TYPE(drawOscilloscopeSizeMode)
{
	elem("contain");
	elem("fill");
	elem("dontScale");
	elem("stretch");
	elem("fitX");
	elem("fitY");
}

VFX_NODE_TYPE(VfxNodeDrawOscilloscope)
{
	typeName = "draw.oscilloscope";
	
	inEnum("sizeMode", "drawOscilloscopeSizeMode");
	in("sampleRate", "float", "44100");
	in("stroke", "float", "4");
	in("color", "color", "ffff");
	in("intensity", "float", "1");
	in("x", "float");
	in("y", "float");
	out("draw", "draw", "draw");
}

VfxNodeDrawOscilloscope::VfxNodeDrawOscilloscope()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_SizeMode, kVfxPlugType_Int);
	addInput(kInput_SampleRate, kVfxPlugType_Float);
	addInput(kInput_StrokeSize, kVfxPlugType_Float);
	addInput(kInput_Color, kVfxPlugType_Color);
	addInput(kInput_Intensity, kVfxPlugType_Float);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	
	flags |= kFlag_CustomTraverseTick;
}

void VfxNodeDrawOscilloscope::draw() const
{
	if (isPassthrough)
		return;

	vfxCpuTimingBlock(VfxNodeDrawOscilloscope);

	const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
	
	const SizeMode sizeMode = (SizeMode)getInputInt(kInput_SizeMode, 0);
	const float strokeSize = getInputFloat(kInput_StrokeSize, 4.f);
	const VfxColor * color = getInputColor(kInput_Color, &defaultColor);
	const float intensity = getInputFloat(kInput_Intensity, 1.f);
	
	//if (image != nullptr)
	{
		vfxGpuTimingBlock(VfxNodeDrawImage);
		
		const TRANSFORM transform = getTransform();
		
		if (transform == TRANSFORM_SCREEN)
		{
			float scaleX = 1.f;
			float scaleY = 1.f;
			
			const int imageSx = 2;
			const int imageSy = 2;
			
			const float viewSx = g_currentVfxSurface->getWidth();
			const float viewSy = g_currentVfxSurface->getHeight();
			
			const float fillScaleX = viewSx / float(imageSx);
			const float fillScaleY = viewSy / float(imageSy);
			
			if (sizeMode == kSizeMode_Fill)
			{
				const float scale = fmaxf(fillScaleX, fillScaleY);
				
				scaleX = scale;
				scaleY = scale;
			}
			else if (sizeMode == kSizeMode_Contain)
			{
				const float scale = fminf(fillScaleX, fillScaleY);
				
				scaleX = scale;
				scaleY = scale;
			}
			else if (sizeMode == kSizeMode_DontScale)
			{
				scaleX = 1.f;
				scaleY = 1.f;
			}
			else if (sizeMode == kSizeMode_Stretch)
			{
				scaleX = fillScaleX;
				scaleY = fillScaleY;
			}
			else if (sizeMode == kSizeMode_FitX)
			{
				const float scale = fillScaleX;
				
				scaleX = scale;
				scaleY = scale;
			}
			else if (sizeMode == kSizeMode_FitY)
			{
				const float scale = fillScaleY;
				
				scaleX = scale;
				scaleY = scale;
			}
			else
			{
				Assert(false);
			}
		
			gxPushMatrix();
			{
				gxScalef(scaleX, scaleY, 1.f);
				
				pushBlend(BLEND_ADD);
				{
					Shader shader("oscline");
					setShader(shader);
					{
						shader.setImmediate("uInvert", 1.f);
						shader.setImmediate("uSize", strokeSize / scaleX);
						shader.setImmediate("uIntensity", intensity);
						shader.setImmediate("uColor", color->r, color->g, color->b, color->a);
						shader.setImmediate("uNumPoints", values.size());
						
						if (!values.empty())
						{
							gxBegin(GL_QUADS);
							{
								for (size_t i = 0; i < values.size() - 1; ++i)
								{
									auto & xy1 = values[i + 0];
									auto & xy2 = values[i + 1];
									
									const float x1 = xy1.x;
									const float y1 = xy1.y;
									const float x2 = xy2.x;
									const float y2 = xy2.y;
									
									for (int j = 0; j < 4; ++j)
										gxVertex4f(x1, y1, x2, y2);
								}
							}
							gxEnd();
						}
					}
					clearShader();
				}
				popBlend();
			}
			gxPopMatrix();
		}
	}
}

void VfxNodeDrawOscilloscope::customTraverseTick(const int traversalId, const float dt)
{
	const float sampleRate = getInputFloat(kInput_SampleRate, 44100.f);
	const float sampleDt = 1.f / sampleRate;
	
	const int numSubsteps = std::max(1, (int)std::ceilf(dt / sampleDt));
	
	const float substepDt = dt / numSubsteps;
	
	values.clear();
	values.resize(numSubsteps);
	
	// todo : figure out a better way to implement timestep subdivisions
	
	int substepTraversalId = traversalId;
	
	for (int i = 0; i < numSubsteps; ++i)
	{
		for (auto predep : predeps)
		{
			if (predep->lastTickTraversalId != substepTraversalId)
				predep->traverseTick(substepTraversalId, substepDt);
		}
		
		auto & xy = values[i];
		xy.x = getInputFloat(kInput_X, 0.f);
		xy.y = getInputFloat(kInput_Y, 0.f);
		
		substepTraversalId++;
	}
}

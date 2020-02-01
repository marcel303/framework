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

#include "framework.h"
#include "internal.h"

// todo : do we still need builtinShaders ? loading and caching now (with the chibi resource paths registration) work as expected, just as any other shader or resource

static const Vec4 lumiVec(.30f, .59f, .11f, 0.f); // luminance = dot(lumiVec, srgb_color)

void makeGaussianKernel(int kernelSize, ShaderBuffer & kernel, float sigma)
{
	auto dist = [](const float x) { return .5f * erfcf(-x); };
	
	float * values = (float*)alloca(sizeof(float) * kernelSize);
	
	if (kernelSize > 0)
	{
		for (int i = 0; i < kernelSize; ++i)
		{
			const float x1 = (i - .5f) / float(kernelSize - 1.f);
			const float x2 = (i + .5f) / float(kernelSize - 1.f);
			
			const float y1 = dist(x1 * sigma);
			const float y2 = dist(x2 * sigma);
			
			const float dy = y2 - y1;
			
			//printf("%02.2f - %02.2f : %02.4f\n", x1, x2, dy);
			
			values[i] = dy;
		}
		
		float total = values[0];
		
		for (int i = 1; i < kernelSize; ++i)
			total += values[i] * 2.f;
		
		if (total > 0.f)
		{
			for (int i = 0; i < kernelSize; ++i)
			{
				values[i] /= total;
			}
		}
	}
	
	kernel.setData(values, sizeof(float) * kernelSize);
}

void setShader_GaussianBlurH(const GxTextureId source, const int kernelSize, const float radius)
{
	Shader & shader = globals.builtinShaders->gaussianBlurH.get();
	setShader(shader);
	
	auto & kernel = globals.builtinShaders->gaussianKernelBuffer;
	makeGaussianKernel(kernelSize, kernel);
	
	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("kernelSize", kernelSize);
	shader.setImmediate("radius", radius);
	shader.setBuffer("filterKernel", kernel);
}

void setShader_GaussianBlurV(const GxTextureId source, const int kernelSize, const float radius)
{
	Shader & shader = globals.builtinShaders->gaussianBlurV.get();
	setShader(shader);
	
	auto & kernel = globals.builtinShaders->gaussianKernelBuffer;
	makeGaussianKernel(kernelSize, kernel);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("kernelSize", kernelSize);
	shader.setImmediate("radius", radius);
	shader.setBuffer("filterKernel", kernel);
}

static void setShader_ThresholdLumiEx(
	const GxTextureId source,
	const float threshold,
	const Vec4 weights,
	bool doFailReplacement,
	bool doPassReplacement,
	const Color & failColor,
	const Color & passColor,
	const float opacity)
{
	Shader & shader = globals.builtinShaders->threshold.get();
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("settings", threshold, doFailReplacement, doPassReplacement, opacity);
	shader.setImmediate("weights", weights[0], weights[1], weights[2], weights[3]);
	shader.setImmediate("failValue", failColor.r, failColor.g, failColor.b, failColor.a);
	shader.setImmediate("passValue", passColor.r, passColor.g, passColor.b, passColor.a);
}

void setShader_ThresholdLumi(const GxTextureId source, const float lumi, const Color & failColor, const Color & passColor, const float opacity)
{
	setShader_ThresholdLumiEx(
		source,
		lumi,
		lumiVec,
		true,
		true,
		failColor,
		passColor,
		opacity);
}

void setShader_ThresholdLumiFail(const GxTextureId source, const float lumi, const Color & failColor, const float opacity)
{
	setShader_ThresholdLumiEx(
		source,
		lumi,
		lumiVec,
		true,
		false,
		failColor,
		colorWhite,
		opacity);
}

void setShader_ThresholdLumiPass(const GxTextureId source, const float lumi, const Color & passColor, const float opacity)
{
	setShader_ThresholdLumiEx(
		source,
		lumi,
		lumiVec,
		false,
		true,
		colorWhite,
		passColor,
		opacity);
}

static void setShader_ThresholdValueEx(
	const GxTextureId source,
	const Vec4 threshold,
	bool doFailReplacement,
	bool doPassReplacement,
	const Color & failColor,
	const Color & passColor,
	const Vec4 opacity)
{
	Shader & shader = globals.builtinShaders->thresholdValue.get();
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("settings", 0.f, doFailReplacement, doPassReplacement, 0.f);
	shader.setImmediate("tresholds", threshold[0], threshold[1], threshold[2], threshold[3]);
	shader.setImmediate("failValue", failColor.r, failColor.g, failColor.b, failColor.a);
	shader.setImmediate("passValue", passColor.r, passColor.g, passColor.b, passColor.a);
	shader.setImmediate("opacities", opacity[0], opacity[1], opacity[2], opacity[3]);
}

void setShader_ThresholdValue(const GxTextureId source, const Color & value, const Color & failColor, const Color & passColor, const float opacity)
{
	setShader_ThresholdValueEx(
		source,
		Vec4(value.r, value.g, value.b, value.a),
		true,
		true,
		failColor,
		passColor,
		Vec4(opacity, opacity, opacity, 0.f));
}

void setShader_GrayscaleLumi(const GxTextureId source, const float opacity)
{
	Shader shader("engine/builtin-grayscale-lumi");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("opacity", opacity);
}

void setShader_GrayscaleWeights(const GxTextureId source, const Vec3 & weights, const float opacity)
{
	Shader shader("engine/builtin-grayscale-weights");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("weights", weights[0], weights[1], weights[2]);
	shader.setImmediate("opacity", opacity);
}

void setShader_Colorize(const GxTextureId source, const float hue, const float opacity)
{
	Shader shader("engine/builtin-hue-assign");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("hue", hue);
	shader.setImmediate("opacity", opacity);
}

void setShader_HueShift(const GxTextureId source, const float hue, const float opacity)
{
	Shader shader("engine/builtin-hue-shift");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("hueShift", hue);
	shader.setImmediate("opacity", opacity);
}

void setShader_ColorMultiply(const GxTextureId source, const Color & color, const float opacity)
{
	Shader & shader = globals.builtinShaders->colorMultiply.get();
	setShader(shader);
	
	shader.setTexture("source", 0, source);
	shader.setImmediate("color", color.r, color.g, color.b, color.a);
	shader.setImmediate("opacity", opacity);

}

void setShader_ColorTemperature(const GxTextureId source, const float temperature, const float opacity)
{
	Shader & shader = globals.builtinShaders->colorTemperature.get();
	setShader(shader);
	
	shader.setTexture("source", 0, source);
	shader.setImmediate("temperature", temperature);
	shader.setImmediate("opacity", opacity);
}

void setShader_TextureSwizzle(const GxTextureId source, const int r, const int g, const int b, const int a)
{
	Shader & shader = globals.builtinShaders->textureSwizzle.get();
	setShader(shader);
	
	shader.setTexture("source", 0, source);
	shader.setImmediate("swizzleMask", r, g, b, a);
}
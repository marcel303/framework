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

#include "framework.h"

#if ENABLE_OPENGL

#include "shaders.h"

#include "data/engine/BasicSkinned.vs"
#include "data/engine/BasicSkinned.ps"
#include "data/engine/Generic.vs"
#include "data/engine/Generic.ps"
#include "data/engine/ShaderVS.txt"
#include "data/engine/ShaderPS.txt"
#include "data/engine/ShaderCS.txt"
#include "data/engine/ShaderUtil.txt"

#include "data/engine/builtin-colormultiply.vs"
#include "data/engine/builtin-colormultiply.ps"
#include "data/engine/builtin-colortemperature.vs"
#include "data/engine/builtin-colortemperature.ps"
#include "data/engine/builtin-guassian-h.ps"
#include "data/engine/builtin-guassian-h.vs"
#include "data/engine/builtin-guassian-v.ps"
#include "data/engine/builtin-guassian-v.vs"
#include "data/engine/builtin-treshold.ps"
#include "data/engine/builtin-treshold.vs"
#include "data/engine/builtin-treshold-componentwise.ps"
#include "data/engine/builtin-treshold-componentwise.vs"

#include "data/engine/builtin-hq-common.txt"
#include "data/engine/builtin-hq-common-vs.txt"
#include "data/engine/builtin-hq-filled-circle.ps"
#include "data/engine/builtin-hq-filled-circle.vs"
#include "data/engine/builtin-hq-filled-rect.ps"
#include "data/engine/builtin-hq-filled-rect.vs"
#include "data/engine/builtin-hq-filled-rounded-rect.ps"
#include "data/engine/builtin-hq-filled-rounded-rect.vs"
#include "data/engine/builtin-hq-filled-triangle.ps"
#include "data/engine/builtin-hq-filled-triangle.vs"
#include "data/engine/builtin-hq-line.ps"
#include "data/engine/builtin-hq-line.vs"
#include "data/engine/builtin-hq-shaded-triangle.ps"
#include "data/engine/builtin-hq-shaded-triangle.vs"
#include "data/engine/builtin-hq-stroked-circle.ps"
#include "data/engine/builtin-hq-stroked-circle.vs"
#include "data/engine/builtin-hq-stroked-rect.ps"
#include "data/engine/builtin-hq-stroked-rect.vs"
#include "data/engine/builtin-hq-stroked-rounded-rect.ps"
#include "data/engine/builtin-hq-stroked-rounded-rect.vs"
#include "data/engine/builtin-hq-stroked-triangle.ps"
#include "data/engine/builtin-hq-stroked-triangle.vs"

#include "data/engine/builtin-msdf-text.ps"
#include "data/engine/builtin-msdf-text.vs"

void registerBuiltinShaders()
{
	shaderSource("engine/ShaderCommon.txt",
		R"SHADER(
		#if LEGACY_GL
			#define shader_attrib attribute
			#define shader_in varying
			#define shader_out varying
			
			#define texture texture2D
		#else
			#define shader_attrib in
			#define shader_in in
			#define shader_out out
			
			precision highp float;
		#endif
		
		#define tex2D texture
		
		#define VS_USE_LEGACY_MATRICES 0
		
		#define VS_POSITION      0
		#define VS_NORMAL        1
		#define VS_COLOR         2
		#define VS_TEXCOORD      3
		#define VS_BLEND_INDICES 4
		#define VS_BLEND_WEIGHTS 5
		)SHADER"
	);

	shaderSource("engine/BasicSkinned.vs", s_basicSkinnedVs);
	shaderSource("engine/BasicSkinned.ps", s_basicSkinnedPs);
	
	shaderSource("engine/Generic.vs", s_genericVs);
	shaderSource("engine/Generic.ps", s_genericPs);

	shaderSource("engine/ShaderVS.txt", s_shaderVs);
	shaderSource("engine/ShaderPS.txt", s_shaderPs);
	shaderSource("engine/ShaderCS.txt", s_shaderCs);

	shaderSource("engine/ShaderUtil.txt", s_shaderUtil);
	
	shaderSource("engine/builtin-colormultiply.ps", s_colorMultiplyPs);
	shaderSource("engine/builtin-colormultiply.vs", s_colorMultiplyVs);
	shaderSource("engine/builtin-colortemperature.ps", s_colorTemperaturePs);
	shaderSource("engine/builtin-colortemperature.vs", s_colorTemperatureVs);
	shaderSource("engine/builtin-gaussian-h.ps", s_guassianHPs);
	shaderSource("engine/builtin-gaussian-h.vs", s_guassianHVs);
	shaderSource("engine/builtin-gaussian-v.ps", s_guassianVPs);
	shaderSource("engine/builtin-gaussian-v.vs", s_guassianVVs);
	shaderSource("engine/builtin-treshold.ps", s_tresholdPs);
	shaderSource("engine/builtin-treshold.vs", s_tresholdVs);
	shaderSource("engine/builtin-treshold-componentwise.ps", s_tresholdComponentwisePs);
	shaderSource("engine/builtin-treshold-componentwise.vs", s_tresholdComponentwiseVs);
	
	shaderSource("engine/builtin-hq-common.txt", s_hqCommonPs);
	shaderSource("engine/builtin-hq-common-vs.txt", s_hqCommonVs);
	shaderSource("engine/builtin-hq-filled-circle.ps", s_hqFilledCirclePs);
	shaderSource("engine/builtin-hq-filled-circle.vs", s_hqFilledCircleVs);
	shaderSource("engine/builtin-hq-filled-rect.ps", s_hqFilledRectPs);
	shaderSource("engine/builtin-hq-filled-rect.vs", s_hqFilledRectVs);
	shaderSource("engine/builtin-hq-filled-rounded-rect.ps", s_hqFilledRoundedRectPs);
	shaderSource("engine/builtin-hq-filled-rounded-rect.vs", s_hqFilledRoundedRectVs);
	shaderSource("engine/builtin-hq-filled-triangle.ps", s_hqFilledTrianglePs);
	shaderSource("engine/builtin-hq-filled-triangle.vs", s_hqFilledTriangleVs);
	shaderSource("engine/builtin-hq-line.ps", s_hqLinePs);
	shaderSource("engine/builtin-hq-line.vs", s_hqLineVs);
	shaderSource("engine/builtin-hq-shaded-triangle.ps", s_hqShadedTrianglePs);
	shaderSource("engine/builtin-hq-shaded-triangle.vs", s_hqShadedTriangleVs);
	shaderSource("engine/builtin-hq-stroked-circle.ps", s_hqStrokedCirclePs);
	shaderSource("engine/builtin-hq-stroked-circle.vs", s_hqStrokedCircleVs);
	shaderSource("engine/builtin-hq-stroked-rect.ps", s_hqStrokedRectPs);
	shaderSource("engine/builtin-hq-stroked-rect.vs", s_hqStrokedRectVs);
	shaderSource("engine/builtin-hq-stroked-rounded-rect.ps", s_hqStrokedRoundedRectPs);
	shaderSource("engine/builtin-hq-stroked-rounded-rect.vs", s_hqStrokedRoundedRectVs);
	shaderSource("engine/builtin-hq-stroked-triangle.ps", s_hqStrokedTrianglePs);
	shaderSource("engine/builtin-hq-stroked-triangle.vs", s_hqStrokedTriangleVs);
	
	shaderSource("engine/builtin-msdf-text.ps", s_msdfTextPs);
	shaderSource("engine/builtin-msdf-text.vs", s_msdfTextVs);
}

#endif

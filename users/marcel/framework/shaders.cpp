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

#include "data/engine/builtin-colortemperature.vs"
#include "data/engine/builtin-colortemperature.ps"
#include "data/engine/builtin-guassian-h.ps"
#include "data/engine/builtin-guassian-h.vs"
#include "data/engine/builtin-guassian-v.ps"
#include "data/engine/builtin-guassian-v.vs"
#include "data/engine/builtin-hq-common.txt"
#include "data/engine/builtin-hq-filled-circle.ps"
#include "data/engine/builtin-hq-filled-circle.vs"
#include "data/engine/builtin-hq-filled-rect.ps"
#include "data/engine/builtin-hq-filled-rect.vs"
#include "data/engine/builtin-hq-filled-triangle.ps"
#include "data/engine/builtin-hq-filled-triangle.vs"
#include "data/engine/builtin-hq-line.ps"
#include "data/engine/builtin-hq-line.vs"
#include "data/engine/builtin-hq-stroked-circle.ps"
#include "data/engine/builtin-hq-stroked-circle.vs"
#include "data/engine/builtin-hq-stroked-rect.ps"
#include "data/engine/builtin-hq-stroked-rect.vs"
#include "data/engine/builtin-hq-stroked-triangle.ps"
#include "data/engine/builtin-hq-stroked-triangle.vs"

void registerBuiltinShaders()
{
	shaderSource("engine/ShaderCommon.txt",
		R"SHADER(
		#if LEGACY_GL
			#define shader_attrib attribute
			#define shader_in varying
			#define shader_out varying
		#else
			#define shader_attrib in
			#define shader_in in
			#define shader_out out
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
	
	shaderSource("engine/builtin-colortemperature.ps", s_colorTemperaturePs);
	shaderSource("engine/builtin-colortemperature.vs", s_colorTemperatureVs);
	shaderSource("engine/builtin-gaussian-h.ps", s_guassianHPs);
	shaderSource("engine/builtin-gaussian-h.vs", s_guassianHVs);
	shaderSource("engine/builtin-gaussian-v.ps", s_guassianVPs);
	shaderSource("engine/builtin-gaussian-v.vs", s_guassianVVs);
	shaderSource("engine/builtin-hq-common.txt", s_hqCommon);
	shaderSource("engine/builtin-hq-filled-circle.ps", s_hqFilledCirclePs);
	shaderSource("engine/builtin-hq-filled-circle.vs", s_hqFilledCircleVs);
	shaderSource("engine/builtin-hq-filled-rect.ps", s_hqFilledRectPs);
	shaderSource("engine/builtin-hq-filled-rect.vs", s_hqFilledRectVs);
	shaderSource("engine/builtin-hq-filled-triangle.ps", s_hqFilledTrianglePs);
	shaderSource("engine/builtin-hq-filled-triangle.vs", s_hqFilledTriangleVs);
	shaderSource("engine/builtin-hq-line.ps", s_hqLinePs);
	shaderSource("engine/builtin-hq-line.vs", s_hqLineVs);
	shaderSource("engine/builtin-hq-stroked-circle.ps", s_hqStrokedCirclePs);
	shaderSource("engine/builtin-hq-stroked-circle.vs", s_hqStrokedCircleVs);
	shaderSource("engine/builtin-hq-stroked-rect.ps", s_hqStrokedRectPs);
	shaderSource("engine/builtin-hq-stroked-rect.vs", s_hqStrokedRectVs);
	shaderSource("engine/builtin-hq-stroked-triangle.ps", s_hqStrokedTrianglePs);
	shaderSource("engine/builtin-hq-stroked-triangle.vs", s_hqStrokedTriangleVs);
}

#endif

#include "framework.h"
#include "shaders.h"

#include "data/engine/BasicSkinned.vs"
#include "data/engine/BasicSkinned.ps"
#include "data/engine/Generic.vs"
#include "data/engine/Generic.ps"
#include "data/engine/ShaderVS.txt"
#include "data/engine/ShaderPS.txt"
#include "data/engine/ShaderCS.txt"
#include "data/engine/ShaderUtil.txt"

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

	shaderSource("builtin-hq-filled-circle.ps", s_hqFilledCirclePs);
	shaderSource("builtin-hq-filled-circle.vs", s_hqFilledCircleVs);
	shaderSource("builtin-hq-filled-rect.ps", s_hqFilledRectPs);
	shaderSource("builtin-hq-filled-rect.vs", s_hqFilledRectVs);
	shaderSource("builtin-hq-filled-triangle.ps", s_hqFilledTrianglePs);
	shaderSource("builtin-hq-filled-triangle.vs", s_hqFilledTriangleVs);
	shaderSource("builtin-hq-line.ps", s_hqLinePs);
	shaderSource("builtin-hq-line.vs", s_hqLineVs);
	shaderSource("builtin-hq-stroked-circle.ps", s_hqStrokedCirclePs);
	shaderSource("builtin-hq-stroked-circle.vs", s_hqStrokedCircleVs);
	shaderSource("builtin-hq-stroked-rect.ps", s_hqStrokedRectPs);
	shaderSource("builtin-hq-stroked-rect.vs", s_hqStrokedRectVs);
	shaderSource("builtin-hq-stroked-triangle.ps", s_hqStrokedTrianglePs);
	shaderSource("builtin-hq-stroked-triangle.vs", s_hqStrokedTriangleVs);
}

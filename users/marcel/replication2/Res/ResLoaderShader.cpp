#include "Archive.h"
#include "ResLoaderShader.h"
#include "ResMgr.h"
#include "ResShader.h"

static bool conv_bool(const std::string& v);
static TEST_FUNC conv_test_func(const std::string& v);
static CULL_MODE conv_cull_mode(const std::string& v);
static int conv_int(const std::string& v);
static float conv_float(const std::string& v);
static STENCIL_OP conv_stencil_op(const std::string& v);
static BLEND_OP conv_blend_op(const std::string& v);

ResLoaderShader::ResLoaderShader()
{
}

Res* ResLoaderShader::Load(const std::string& name)
{
	Ar ar;

	if (!ar.Read(name))
		Assert(0);

	ResShader* shader = new ResShader();

	std::string vs = ar("vs", "");
	std::string ps = ar("ps", "");

	std::string depth_test_func = ar("depth_test_func", "");
	std::string light_enabled = ar("light_enabled", "");
	std::string fog_enable = ar("fog_enabled", "");
	std::string cull_mode = ar("cull_mode", "");
	std::string blend_src = ar("blend_src", "");
	std::string blend_dst = ar("blend_dst", "");
	std::string alpha_test_func = ar("alpha_test_func", "");
	std::string alpha_test_ref = ar("alpha_test_ref", "");
	std::string stencil_test_func = ar("stencil_test_func", "");
	std::string stencil_test_ref = ar("stencil_test_ref", "");
	std::string stencil_test_mask = ar("stencil_test_mask", "");
	std::string stencil_test_pass = ar("stencil_test_pass", "");
	std::string stencil_test_fail = ar("stencil_test_fail", "");
	std::string stencil_test_zfail = ar("stencil_test_zfail", "");
	std::string write_color = ar("write_color", "");
	std::string write_depth = ar("write_depth", "");

#define BEGIN(v) \
	if (v != "") \
	{

#define END() \
	}

	BEGIN(vs)
		shader->m_vs = RESMGR.GetVS(vs);
	END()
	BEGIN(ps)
		shader->m_ps = RESMGR.GetPS(ps);
	END()

	BEGIN(depth_test_func)
		shader->InitDepthTest(true, conv_test_func(depth_test_func));
	END()
	BEGIN(light_enabled)
		shader->InitLight(true);
	END()
	BEGIN(fog_enable)
		shader->InitFog(true);
	END()
	BEGIN(cull_mode)
		shader->InitCull(conv_cull_mode(cull_mode));
	END()
	BEGIN(blend_src)
		BEGIN(blend_dst)
			shader->InitBlend(true, conv_blend_op(blend_src), conv_blend_op(blend_dst));
		END()
	END()
	BEGIN(alpha_test_func)
		BEGIN(alpha_test_ref)
			shader->InitAlphaTest(true, conv_test_func(alpha_test_func), conv_float(alpha_test_ref));
		END()
	END()
	BEGIN(stencil_test_func)
		BEGIN(stencil_test_ref)
			BEGIN(stencil_test_mask)
				shader->InitStencilTest(
					true,
					conv_test_func(stencil_test_func),
					conv_int(stencil_test_ref),
					conv_int(stencil_test_mask),
					conv_stencil_op(stencil_test_pass),
					conv_stencil_op(stencil_test_fail),
					conv_stencil_op(stencil_test_zfail));
			END()
		END()
	END()
	BEGIN(write_color)
		shader->InitWriteColor(conv_bool(write_color));
	END()
	BEGIN(write_depth)
		shader->InitWriteDepth(conv_bool(write_depth));
	END()

	return shader;
}

static bool conv_bool(const std::string& v)
{
	if (v == "1" || v == "true")
		return true;

	return false;
}

static TEST_FUNC conv_test_func(const std::string& v)
{
	if (v == "always")
		return CMP_ALWAYS;
	if (v == "eq")
		return CMP_EQ;
	if (v == "l")
		return CMP_L;
	if (v == "le")
		return CMP_LE;
	if (v == "g")
		return CMP_G;
	if (v == "ge")
		return CMP_GE;

	return CMP_ALWAYS;
}

static CULL_MODE conv_cull_mode(const std::string& v)
{
	if (v == "none")
		return CULL_NONE;
	if (v == "cw")
		return CULL_CW;
	if (v == "ccw")
		return CULL_CCW;

	return CULL_NONE;
}

static int conv_int(const std::string& v)
{
	Ar ar;
	ar["v"] = v;
	return ar(v, 0);
}

static float conv_float(const std::string& v)
{
	Ar ar;
	ar["v"] = v;
	return ar(v, 0.0f);
}

static STENCIL_OP conv_stencil_op(const std::string& v)
{
	if (v == "inc")
		return INC;
	if (v == "dec")
		return DEC;
	if (v == "zero")
		return ZERO;

	return ZERO;
}

static BLEND_OP conv_blend_op(const std::string& v)
{
	if (v == "one")
		return BLEND_ONE;
	if (v == "zero")
		return BLEND_ZERO;
	if (v == "src")
		return BLEND_SRC;
	if (v == "dst")
		return BLEND_DST;
	if (v == "src_color")
		return BLEND_SRC_COLOR;
	if (v == "dst_color")
		return BLEND_DST_COLOR;
	if (v == "inv_src")
		return BLEND_INV_SRC;
	if (v == "inv_dst")
		return BLEND_INV_DST;
	if (v == "inv_src_color")
		return BLEND_INV_SRC_COLOR;
	if (v == "inv_dst_color")
		return BLEND_INV_DST_COLOR;

	return BLEND_ONE;
}

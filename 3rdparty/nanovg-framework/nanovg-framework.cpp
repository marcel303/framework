#include "framework.h"
#include "gx_texture.h"
#include "nanovg.h"
#include "nanovg-framework.h"
#include <map>

static const char * s_fillVs = R"SHADER(

include engine/ShaderVS.txt

shader_out vec2 v_texcoord;
shader_out vec2 v_position;

void main()
{
	vec4 position = unpackPosition();
	vec2 texcoord = unpackTexcoord(0);
	
	v_texcoord = texcoord;
	v_position = position.xy;
	
	gl_Position = objectToProjection(position);
}

)SHADER";

// todo : create an optimized shader for stenciling

static const char * s_fillPs = R"SHADER(

include engine/ShaderPS.txt
include engine/ShaderUtil.txt

#define EDGE_AA 1

uniform mat4 scissorMat;
uniform mat4 paintMat;
uniform vec4 innerCol;
uniform vec4 outerCol;
uniform vec2 scissorExt;
uniform vec2 scissorScale;
uniform vec2 extent;
uniform float radius;
uniform float feather;
uniform float dither;
uniform float strokeMult;
uniform float strokeThr;
uniform float texType;
uniform float type;

uniform sampler2D tex;

shader_in vec2 v_position;
shader_in vec2 v_texcoord;

float sdroundrect(vec2 pt, vec2 ext, float rad)
{
	vec2 ext2 = ext - vec2(rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

// Scissoring
float scissorMask(vec2 p)
{
	vec2 sc = (abs((scissorMat * vec4(p, 0.0, 1.0)).xy) - scissorExt);
	sc = vec2(0.5, 0.5) - sc * scissorScale;
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

#if EDGE_AA
// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
float strokeMask()
{
	return min(1.0, (1.0 - abs(v_texcoord.x * 2.0 - 1.0)) * strokeMult) * min(1.0, v_texcoord.y);
}
#endif

void main(void)
{
	vec4 result;
	
	float scissor = scissorMask(v_position);

#if EDGE_AA
	float strokeAlpha = strokeMask();
	
	if (strokeAlpha < strokeThr)
		discard;
#else
	float strokeAlpha = 1.0;
#endif

	if (type == 0.0) // Gradient
	{
		// Calculate gradient color using box gradient
		vec2 pt = (paintMat * vec4(v_position, 0.0, 1.0)).xy;
		float d = clamp((sdroundrect(pt, extent, radius) + feather * 0.5) / feather, 0.0, 1.0);
		vec4 color = mix(innerCol, outerCol, d);
		
		if (dither != 0.0)
		{
			// Dithering
			color.rgb += colorDither8ScreenSpace(v_position);
		}
		
		// Combine alpha
		color *= strokeAlpha * scissor;
		
		result = color;
	}
	else if (type == 1.0) // Image
	{
		// Calculate color fron texture
		vec2 pt = (paintMat * vec4(v_position, 0.0, 1.0)).xy / extent;
		
		vec4 color = texture(tex, pt);
		if (texType == 1) color = vec4(color.xyz * color.w, color.w);
		if (texType == 2) color = vec4(color.x);
		
		// Apply color tint and alpha.
		color *= innerCol;
		
		// Combine alpha
		color *= strokeAlpha * scissor;
		result = color;
	}
	else if (type == 2.0) // Stencil fill
	{
		result = vec4(1, 1, 1, 1);
	}
	else if (type == 3.0) // Textured tris
	{
		vec4 color = texture(tex, v_texcoord);
		
		if (texType == 1.0) color = vec4(color.xyz * color.w, color.w);
		if (texType == 2.0) color = vec4(color.x);
		
		color *= scissor;
		
		result = color * innerCol;
	}
	
	shader_fragColor = result;
}

)SHADER";

struct nvgFrameworkCtx
{
	struct Image
	{
		GxTexture * texture = nullptr;
		int flags = 0;
	};
	
	int flags = 0;
	
	int viewportSx = 0;
	int viewportSy = 0;
	
	std::map<int, Image> images;
	
	int nextImageId = 1;
	
	~nvgFrameworkCtx()
	{
		Assert(images.empty());
		Assert(nextImageId == 1);
	}
	
	void create()
	{
		Assert(images.empty());
		Assert(nextImageId == 1);
		
		shaderSource("nanovg-framework/fill.vs", s_fillVs);
		shaderSource("nanovg-framework/fill.ps", s_fillPs);
	}
	
	void destroy()
	{
		// free textures
		
		for (auto & i : images)
		{
			auto & image = i.second;
			
			image.texture->free();
			
			delete image.texture;
			image.texture = nullptr;
		}
		
		images.clear();
		
		nextImageId = 1;
	}
	
	const Image * getImage(const int imageId) const
	{
		if (imageId == 0)
			return nullptr;
		
		auto i = images.find(imageId);
		Assert(i != images.end());
		if (i != images.end())
			return &i->second;
		else
			return nullptr;
	}
};

static int renderCreate(void* uptr)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	frameworkCtx->create();
	return 1;
}

static int renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	const int imageId = frameworkCtx->nextImageId++;
	auto & image = frameworkCtx->images[imageId];
	Assert(image.texture == nullptr);
	
	image.texture = new GxTexture();
	image.texture->allocate(w, h, type == NVG_TEXTURE_ALPHA ? GX_R8_UNORM : GX_RGBA8_UNORM, true, true);
	image.flags = imageFlags;
	
	if (data != nullptr)
	{
		image.texture->upload(data, 0, 0);
		
		if (image.flags & NVG_IMAGE_GENERATE_MIPMAPS)
			image.texture->generateMipmaps();
	}
	
	return imageId;
}

static int renderDeleteTexture(void* uptr, int imageId)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	auto i = frameworkCtx->images.find(imageId);
	Assert(i != frameworkCtx->images.end());
	if (i != frameworkCtx->images.end())
	{
		auto & image = i->second;
		
		image.texture->free();
	
		delete image.texture;
		image.texture = nullptr;
		
		frameworkCtx->images.erase(i);
		
		return 1;
	}
	else
		return 0;
}

static int renderUpdateTexture(void* uptr, int imageId, int x, int y, int w, int h, const unsigned char* data)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	auto * image = frameworkCtx->getImage(imageId);
	Assert(image != nullptr);
	if (image != nullptr)
	{
		auto * texture = image->texture;
		
		const int pixelStride = texture->format == GX_R8_UNORM ? 1 : 4;
		data += (y * texture->sx + x) * pixelStride;
		
		texture->uploadArea(data, 1, texture->sx, w, h, x, y);
		
		if (image->flags & NVG_IMAGE_GENERATE_MIPMAPS)
			image->texture->generateMipmaps();
		
		return 1;
	}
	else
		return 0;
}

static int renderGetTextureSize(void* uptr, int imageId, int* w, int* h)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	auto * image = frameworkCtx->getImage(imageId);
	Assert(image != nullptr);
	if (image != nullptr)
	{
		*w = image->texture->sx;
		*h = image->texture->sy;
		return 1;
	}
	else
		return 0;
}

static void renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	frameworkCtx->viewportSx = width;
	frameworkCtx->viewportSy = height;
}

static void renderCancel(void* uptr)
{
}

static void renderFlush(void* uptr)
{
}

static Mat4x4 xformToInverseMat(const float * xform)
{
	// construct a 4x4 matrix from the 3x2 xform
	
	Mat4x4 mat;
	
	const float * __restrict s = xform;
	      float * __restrict v = mat.m_v;
	
	v[ 0] = s[0]; v[ 1] = s[1]; v[ 2] = 0.f; v[ 3] = 0.f;
	v[ 4] = s[2]; v[ 5] = s[3]; v[ 6] = 0.f; v[ 7] = 0.f;
	v[ 8] = 0.f;  v[ 9] = 0.f;  v[10] = 1.f; v[11] = 0.f;
	v[12] = s[4]; v[13] = s[5]; v[14] = 0.f; v[15] = 1.f;

	// invert and return it
	
	return mat.CalcInv();
}

static NVGcolor premultiplyColorWithAlpha(NVGcolor c)
{
	c.r *= c.a;
	c.g *= c.a;
	c.b *= c.a;
	return c;
}

static void computeShaderUniforms(
	const NVGpaint & paint,
	const nvgFrameworkCtx::Image * image,
	const NVGscissor & scissor,
	const float width,
	const float fringe,
	Mat4x4 & out_paintMat,
	Mat4x4 & out_scissorMat,
	float * out_scissorExt,
	float * out_scissorScale,
	float & out_strokeMult)
{
	if (paint.image != 0 && image != nullptr && (image->flags & NVG_IMAGE_FLIPY) != 0)
	{
		float m1[6], m2[6];
		nvgTransformTranslate(m1, 0.f, paint.extent[1] * .5f);
		nvgTransformMultiply(m1, paint.xform);
		nvgTransformScale(m2, 1.f, -1.f);
		nvgTransformMultiply(m2, m1);
		nvgTransformTranslate(m1, 0.f, -paint.extent[1] * .5f);
		nvgTransformMultiply(m1, m2);
		
		out_paintMat = xformToInverseMat(m1);
	}
	else
	{
		out_paintMat = xformToInverseMat(paint.xform);
	}

	if (scissor.extent[0] < -.5f || scissor.extent[1] < -.5f)
	{
		memset(&out_scissorMat, 0, sizeof(out_scissorMat));
		out_scissorExt[0] = 1.f;
		out_scissorExt[1] = 1.f;
		out_scissorScale[0] = 1.f;
		out_scissorScale[1] = 1.f;
	}
	else
	{
		out_scissorMat = xformToInverseMat(scissor.xform);
		out_scissorExt[0] = scissor.extent[0];
		out_scissorExt[1] = scissor.extent[1];
		out_scissorScale[0] = sqrtf(scissor.xform[0] * scissor.xform[0] + scissor.xform[2] * scissor.xform[2]) / fringe;
		out_scissorScale[1] = sqrtf(scissor.xform[1] * scissor.xform[1] + scissor.xform[3] * scissor.xform[3]) / fringe;
	}

	out_strokeMult = (width * .5f + fringe * .5f) / fringe;
}

static bool setShaderUniforms(
	Shader & shader,
	const NVGpaint & paint,
	const NVGscissor & scissor,
	const float width,
	const float fringe,
	const float strokeThreshold,
	nvgFrameworkCtx * frameworkCtx)
{
	auto * image = frameworkCtx->getImage(paint.image);
	
	const NVGcolor innerColor = premultiplyColorWithAlpha(paint.innerColor);
	const NVGcolor outerColor = premultiplyColorWithAlpha(paint.outerColor);
	
	Mat4x4 paintMat;
	
	float scissorExt[2];
	float scissorScale[2];
	Mat4x4 scissorMat;
	
	float strokeMult;
	
	computeShaderUniforms(
		paint,
		image,
		scissor,
		width,
		fringe,
		paintMat,
		scissorMat,
		scissorExt,
		scissorScale,
		strokeMult);
	
	shader.setImmediateMatrix4x4("scissorMat", scissorMat.m_v);
	shader.setImmediateMatrix4x4("paintMat", paintMat.m_v);
	shader.setImmediate("innerCol", innerColor.r, innerColor.g, innerColor.b, innerColor.a);
	shader.setImmediate("outerCol", outerColor.r, outerColor.g, outerColor.b, outerColor.a);
	shader.setImmediate("scissorExt", scissorExt[0], scissorExt[1]);
	shader.setImmediate("scissorScale", scissorScale[0], scissorScale[1]);
	shader.setImmediate("extent", paint.extent[0], paint.extent[1]);
	shader.setImmediate("radius", paint.radius);
	shader.setImmediate("feather", paint.feather);
	shader.setImmediate("dither", (frameworkCtx->flags & NVG_DITHER_GRADIENTS) ? 1 : 0);
	shader.setImmediate("strokeMult", strokeMult);
	shader.setImmediate("strokeThr", strokeThreshold);
	
	if (paint.image == 0)
	{
		shader.setImmediate("type", 0); // gradient
		shader.setTexture("tex", 0, 0);
		shader.setImmediate("texType", -1);
	}
	else
	{
		Assert(image != nullptr);
		if (image == nullptr)
			return false;
		
		const bool clamp = (image->flags & (NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY)) == 0;
		const bool filter = (image->flags & NVG_IMAGE_NEAREST) == 0;
		
		shader.setImmediate("type", 1); // image
		shader.setTexture("tex", 0, image->texture->id, filter, clamp);
		if (image->texture->format == GX_RGBA8_UNORM)
			shader.setImmediate("texType", (image->flags & NVG_IMAGE_PREMULTIPLIED) ? 0 : 1);
		else if (image->texture->format == GX_R8_UNORM)
			shader.setImmediate("texType", 2);
		else
			Assert(false);
	}
	
	return true;
}

static BLEND_MODE compositeOperationToBlendMode(const NVGcompositeOperationState & op)
{
#define C(_srcRgb, _srcAlpha, _dstRgb, _dstAlpha, blendMode) if (op.srcRGB == _srcRgb && op.srcAlpha == _srcAlpha && op.dstRGB == _dstRgb && op.dstAlpha == _dstAlpha) { /*logDebug("mode: " # blendMode);*/ return blendMode; }
	C(NVG_ONE,       NVG_ONE, NVG_ZERO,                NVG_ZERO,                BLEND_OPAQUE             );
	C(NVG_SRC_ALPHA, NVG_ONE, NVG_ONE_MINUS_SRC_ALPHA, NVG_ONE_MINUS_SRC_ALPHA, BLEND_ALPHA              );
	C(NVG_ONE,       NVG_ONE, NVG_ONE_MINUS_SRC_ALPHA, NVG_ONE_MINUS_SRC_ALPHA, BLEND_PREMULTIPLIED_ALPHA);
	C(NVG_SRC_ALPHA, NVG_ONE, NVG_ONE,                 NVG_ONE,                 BLEND_ADD                );
	C(NVG_ONE,       NVG_ONE, NVG_ONE,                 NVG_ONE,                 BLEND_ADD_OPAQUE         );
#undef C
	logDebug("unknown blend mode");
	return BLEND_ALPHA;
}

static void drawPrim(const GX_PRIMITIVE_TYPE primType, const NVGvertex * verts, const int numVerts)
{
	if (numVerts < 3)
		return;
	
	gxBegin(primType);
	{
		for (int i = 0; i < numVerts; ++i)
		{
			auto & v = verts[i];
			
			gxTexCoord2f(v.u, v.v);
			gxVertex2f(v.x, v.y);
		}
	}
	gxEnd();
}

static void renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths)
{
	Assert(npaths > 0);
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	Shader shader("nanovg-framework/fill");
	setShader(shader);
	{
		if (!setShaderUniforms(shader, *paint, *scissor, fringe, fringe, -1.f, frameworkCtx))
			return;
		
		// draw the paths into the stencil buffer
		
		setStencilTest()
			.comparison(GX_STENCIL_FUNC_ALWAYS, 0, 0xff)
			.op(GX_STENCIL_FACE_FRONT, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_INC_WRAP)
			.op(GX_STENCIL_FACE_BACK,  GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_DEC_WRAP)
			.writeMask(0xff);
		
		pushColorWriteMask(0, 0, 0, 0);
		{
			for (int i = 0; i < npaths; ++i)
			{
				auto & path = paths[i];
				
				drawPrim(GX_TRIANGLE_FAN, path.fill, path.nfill);
			}
		}
		popColorWriteMask();
		
		// fill a gradient over the stenciled pixels
		
		pushBlend(compositeOperationToBlendMode(compositeOperation));
		
		setStencilTest()
			.comparison(GX_STENCIL_FUNC_NOTEQUAL, 0x00, 0xff)
			.op(GX_STENCIL_OP_ZERO, GX_STENCIL_OP_ZERO, GX_STENCIL_OP_ZERO)
			.writeMask(0xff);
		
		const float x1 = bounds[0];
		const float y1 = bounds[1];
		const float x2 = bounds[2];
		const float y2 = bounds[3];
	
		gxBegin(GX_QUADS);
		{
			gxTexCoord2f(.5f, 1.f);
			gxVertex2f(x1, y1);
			gxVertex2f(x2, y1);
			gxVertex2f(x2, y2);
			gxVertex2f(x1, y2);
		}
		gxEnd();
		
		// and draw anti-aliased edges (with inverted stencil mask to avoid overlap with the filled area)
		
		if (frameworkCtx->flags & NVG_ANTIALIAS)
		{
			setStencilTest()
				.comparison(GX_STENCIL_FUNC_EQUAL, 0x00, 0xff)
				.op(GX_STENCIL_OP_ZERO, GX_STENCIL_OP_ZERO, GX_STENCIL_OP_ZERO)
				.writeMask(0xff);
			
			for (int i = 0; i < npaths; ++i)
			{
				auto & path = paths[i];
				
				drawPrim(GX_TRIANGLE_STRIP, path.stroke, path.nstroke);
			}
		}
		
		clearStencilTest();
		
		popBlend();
	}
	clearShader();
}

static void renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths)
{
	Assert(npaths > 0);
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	Shader shader("nanovg-framework/fill");
	setShader(shader);
	{
		if (!setShaderUniforms(shader, *paint, *scissor, strokeWidth, fringe, -1.f, frameworkCtx))
			return;
		
		pushBlend(compositeOperationToBlendMode(compositeOperation));
		
		for (int i = 0; i < npaths; ++i)
		{
			auto & path = paths[i];
			
			drawPrim(GX_TRIANGLE_STRIP, path.stroke, path.nstroke);
		}
		
		popBlend();
	}
	clearShader();
}

static void renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts)
{
	if (nverts == 0)
		return;
	
	Assert((nverts % 3) == 0);
	Assert((nverts / 3) >= 1);
	
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	Shader shader("nanovg-framework/fill");
	setShader(shader);
	{
		if (!setShaderUniforms(shader, *paint, *scissor, 1.f, 1.f, -1.f, frameworkCtx))
			return;
		shader.setImmediate("type", 3); // textured triangles
		
		pushBlend(compositeOperationToBlendMode(compositeOperation));
		
		drawPrim(GX_TRIANGLES, verts, nverts);
		
		popBlend();
	}
	clearShader();
}

static void renderDelete(void* uptr)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	frameworkCtx->destroy();
	
	delete frameworkCtx;
	frameworkCtx = nullptr;
}

NVGcontext * nvgCreateFramework(int flags)
{
	auto * frameworkCtx = new nvgFrameworkCtx();
	frameworkCtx->flags = flags;
	
	NVGparams params;
	memset(&params, 0, sizeof(params));
	params.renderCreate = renderCreate;
	params.renderCreateTexture = renderCreateTexture;
	params.renderDeleteTexture = renderDeleteTexture;
	params.renderUpdateTexture = renderUpdateTexture;
	params.renderGetTextureSize = renderGetTextureSize;
	params.renderViewport = renderViewport;
	params.renderCancel = renderCancel;
	params.renderFlush = renderFlush;
	params.renderFill = renderFill;
	params.renderStroke = renderStroke;
	params.renderTriangles = renderTriangles;
	params.renderDelete = renderDelete;
	
	params.userPtr = frameworkCtx;
	params.edgeAntiAlias = (flags & NVG_ANTIALIAS) ? 1 : 0;

	return nvgCreateInternal(&params);
}

void nvgDeleteFramework(NVGcontext * ctx)
{
	nvgDeleteInternal(ctx);
}

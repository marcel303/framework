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

static const char * s_fillPs = R"SHADER(

include engine/ShaderPS.txt

#define EDGE_AA 0

uniform mat4 scissorMat;
uniform mat4 paintMat;
uniform vec4 innerCol;
uniform vec4 outerCol;
uniform vec2 scissorExt;
uniform vec2 scissorScale;
uniform vec2 extent;
uniform float radius;
uniform float feather;
uniform float strokeMult;
uniform float strokeThr;
uniform float texType;
uniform float type;

uniform sampler2D tex;

shader_in vec2 v_position;
shader_in vec2 v_texcoord;

#define fpos v_position
#define ftcoord v_texcoord

float sdroundrect(vec2 pt, vec2 ext, float rad)
{
	vec2 ext2 = ext - vec2(rad,rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;
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
	return min(1.0, (1.0 - abs(ftcoord.x * 2.0 - 1.0)) * strokeMult) * min(1.0, ftcoord.y);
}
#endif

void main(void)
{
	vec4 result;
	
	float scissor = scissorMask(fpos);

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
		vec2 pt = (paintMat * vec4(fpos, 0.0, 1.0)).xy;
		float d = clamp((sdroundrect(pt, extent, radius) + feather * 0.5) / feather, 0.0, 1.0);
		vec4 color = mix(innerCol, outerCol, d);
		
		// Combine alpha
		color *= strokeAlpha * scissor;
		result = color;
	}
	else if (type == 1.0) // Image
	{
		// Calculate color fron texture
		vec2 pt = (paintMat * vec4(fpos, 0.0, 1.0)).xy / extent;
		
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
		vec4 color = texture(tex, ftcoord);
		
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
	
	GxTexture * imageToTexture(const int imageId) const
	{
		auto * image = getImage(imageId);
		if (image == nullptr)
			return nullptr;
		else
			return image->texture;
	}
};

// --- declarations ---

static int renderCreate(void* uptr);
static int renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
static int renderDeleteTexture(void* uptr, int image);
static int renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
static int renderGetTextureSize(void* uptr, int image, int* w, int* h);
static void renderViewport(void* uptr, float width, float height, float devicePixelRatio);
static void renderCancel(void* uptr);
static void renderFlush(void* uptr);
static void renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths);
static void renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths);
static void renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts);
static void renderDelete(void* uptr);

// --- implementation ---

static int renderCreate(void* uptr)
{
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	frameworkCtx->create();
	return 1;
}

static int renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
/*
todo :
	NVG_IMAGE_FLIPY				= 1<<3,		// Flips (inverses) image in Y direction when rendered.
*/

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
		
		if (texture->format == GX_R8_UNORM)
		{
			uint8_t * bytes = (uint8_t*)data;
			bytes = bytes + y * texture->sx * 1 + x * 1;
			data = bytes;
		}
		else if (texture->format == GX_RGBA8_UNORM)
		{
			uint8_t * bytes = (uint8_t*)data;
			bytes = bytes + y * texture->sx * 4 + x * 4;
			data = bytes;
		}
		
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
	auto * texture = frameworkCtx->imageToTexture(imageId);
	Assert(texture != nullptr);
	if (texture != nullptr)
	{
		*w = texture->sx;
		*h = texture->sy;
		return 1;
	}
	else
		return 0;
}

static void renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
}

static void renderCancel(void* uptr)
{
}

static void renderFlush(void* uptr)
{
/*
	setColor(colorWhite);
	for (int i = 1; i < 10; ++i)
	{
		const int s = 200;
		gxSetTexture((GxTextureId)i);
		drawRect(i * s, 0, (i + 1) * s, s);
		gxSetTexture(0);
	}
*/
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

static void premultiplyColorWithAlpha(const NVGcolor & c, NVGcolor & r)
{
	r.r = c.r * c.a;
	r.g = c.g * c.a;
	r.b = c.b * c.a;
	r.a = c.a;
}

static void computeShaderUniforms(
	const NVGpaint & paint,
	const NVGscissor & scissor,
	const float width,
	const float fringe,
	Mat4x4 & out_paintMat,
	NVGcolor & out_innerColor,
	NVGcolor & out_outerColor,
	Mat4x4 & out_scissorMat,
	float * out_scissorExt,
	float * out_scissorScale,
	float & out_strokeMult)
{
	out_paintMat = xformToInverseMat(paint.xform);
	premultiplyColorWithAlpha(paint.innerColor, out_innerColor);
	premultiplyColorWithAlpha(paint.outerColor, out_outerColor);

	if (scissor.extent[0] < -0.5f || scissor.extent[1] < -0.5f)
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

static void setShaderUniforms(
	Shader & shader,
	const NVGpaint & paint,
	const NVGscissor & scissor,
	const float width,
	const float fringe,
	nvgFrameworkCtx * frameworkCtx)
{
	Mat4x4 paintMat;
	NVGcolor innerColor;
	NVGcolor outerColor;
	
	float scissorExt[2];
	float scissorScale[2];
	Mat4x4 scissorMat;
	
	float strokeMult;
	
	computeShaderUniforms(
		paint,
		scissor,
		width,
		fringe,
		paintMat,
		innerColor,
		outerColor,
		scissorMat,
		scissorExt,
		scissorScale,
		strokeMult);
	
	const float strokeThr = -1.f;
	
	shader.setImmediateMatrix4x4("scissorMat", scissorMat.m_v);
	shader.setImmediateMatrix4x4("paintMat", paintMat.m_v);
	shader.setImmediate("innerCol", innerColor.r, innerColor.g, innerColor.b, innerColor.a);
	shader.setImmediate("outerCol", outerColor.r, outerColor.g, outerColor.b, outerColor.a);
	shader.setImmediate("scissorExt", scissorExt[0], scissorExt[1]);
	shader.setImmediate("scissorScale", scissorScale[0], scissorScale[1]);
	shader.setImmediate("extent", paint.extent[0], paint.extent[1]);
	shader.setImmediate("radius", paint.radius);
	shader.setImmediate("feather", paint.feather);
	shader.setImmediate("strokeMult", strokeMult);
	shader.setImmediate("strokeThr", strokeThr);
	
	auto * image = frameworkCtx->getImage(paint.image);
	
	if (image == nullptr)
	{
		shader.setImmediate("type", 0); // gradient
		
		shader.setTexture("tex", 0, 0);
		shader.setImmediate("texType", -1);
	}
	else
	{
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
}

static void renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths)
{
	if (npaths <= 0)
		return;
	
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	// todo : if all of the paths are convex, just draw normally
	// todo : for non-convex paths: use stencil and fill
	
	Shader shader("nanovg-framework/fill");
	setShader(shader);
	{
		setShaderUniforms(shader, *paint, *scissor, fringe, fringe, frameworkCtx);
		
		pushBlend(BLEND_PREMULTIPLIED_ALPHA);
		
		for (int i = 0; i < npaths; ++i)
		{
			auto & path = paths[i];
			
			if (path.nfill < 3)
				continue;
		
			gxBegin(GX_TRIANGLE_FAN);
			{
				for (int i = 0; i < path.nfill; ++i)
				{
					auto & v = path.fill[i];
					
					gxTexCoord2f(v.u, v.v);
					gxVertex2f(v.x, v.y);
				}
			}
			gxEnd();
		}
		
		popBlend();
	}
	clearShader();
}

static void renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths)
{
	if (npaths <= 0)
		return;
	
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	Shader shader("nanovg-framework/fill");
	setShader(shader);
	{
		setShaderUniforms(shader, *paint, *scissor, strokeWidth, fringe, frameworkCtx);
		
		pushBlend(BLEND_PREMULTIPLIED_ALPHA);
		
		for (int i = 0; i < npaths; ++i)
		{
			auto & path = paths[i];
			
			if (path.nstroke < 3)
				continue;
			
			gxBegin(GX_TRIANGLE_STRIP);
			{
				for (int i = 0; i < path.nstroke; ++i)
				{
					auto & v = path.stroke[i];
					gxTexCoord2f(v.u, v.v);
					gxVertex2f(v.x, v.y);
				}
			}
			gxEnd();
		}
		
		popBlend();
	}
	clearShader();
}

static void renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts)
{
	if (nverts <= 0)
		return;
	
	Assert((nverts % 3) == 0);
	Assert((nverts / 3) >= 1);
	
	auto * frameworkCtx = (nvgFrameworkCtx*)uptr;
	
	Shader shader("nanovg-framework/fill");
	setShader(shader);
	{
		setShaderUniforms(shader, *paint, *scissor, 1.f, 1.f, frameworkCtx);
		shader.setImmediate("type", 3); // textured triangles
		
		pushBlend(BLEND_PREMULTIPLIED_ALPHA);
	
		gxBegin(GX_TRIANGLES);
		{
			for (int i = 0; i < nverts; ++i)
			{
				auto & v = verts[i];
				
				gxTexCoord2f(v.u, v.v);
				gxVertex2f(v.x, v.y);
			}
		}
		gxEnd();
		
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
	NVGparams params;
	NVGcontext * ctx = nullptr;

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
	
	params.userPtr = new nvgFrameworkCtx();
	params.edgeAntiAlias = false;//flags & NVG_ANTIALIAS ? 1 : 0; // todo

	ctx = nvgCreateInternal(&params);

	return ctx;
}

void nvgDeleteFramework(NVGcontext * ctx)
{
	nvgDeleteInternal(ctx);
}

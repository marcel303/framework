#pragma once

#include "Vec3.h"
#include "Vec4.h"

struct TypeDB;

/*

The renderer supports various render modes.

The 'flat' render mode being the fastest, as it renders the opaque and translucents passes to the back-buffer directly.

The other render modes are more feature rich, including HDR and post-processing effects.
As a consequence, these render modes will draw to intermediate color (and depth) targets, before blitting the result to the back-buffer.

The 'deferred shaded' render mode uses a fixed material model and a deferred lighting pass.
The 'forward shaded' render mode gives the user more flexibility for implementing a material model, at the cost of a slightly higher cost per pixel.

The deferred shaded render mode uses deferred lighting as a solution for lighting opaque geometry. The steps performed by the deferred shaded render mode are,
(-) Draw G-buffers for the opaque geometry using multi-pass rendering. The set of G-buffers includes:
	- Color,
	- Normal,
	- Specular color,
	- Specular exponent,
	- Emissive,
	- Depth
(-) Draw lights into a high-resolution light buffer. The deferred light drawing takes into account the color, normal, specular and emissive properties of a material.
(-) Composite lighting to produce the 'composite' buffer.
(-) Draw the translucent geometry on top of the composited result.
(-) Reconstruct a velocity buffer from depth, using the view and projection matrices from the previous and current frames. The velocity buffer is used to perform a motion-blur effect during the post-processing step.
(-) Perform post-processing.
(-) Perform tone mapping.
(-) Perform optional FXAA as an anti-aliasing solution.

The forward shaded render mode assumes a user-defined forward shading model exists. The forward shaded render mode supports an optional depth pre-pass. The generation of the G-buffers differs depending on whether or not the depth pre-pass is enabled.
When the depth pre-pass is enabled, G-buffers are drawn as follows:
(-) Draw the depth pre-pass.
(-) Draw the color and normal buffers using multi-pass rendering, with the depth test set to EQUAL.
When the depth pre-pass is disabled, all buffers are generated in one pass, using multi-pass rendering.
Once the G-buffers are drawn, the translucents geometry is drawn on top of the result.
After drawing the geometry is done, post-processing is performed. The steps are:
(-) Reconstruct a velocity buffer from depth, using the view and projection matrices from the previous and current frames. The velocity buffer is used to perform a motion-blur effect during the post-processing step.
(-) Perform post-processing.
(-) Perform tone mapping.
(-) Perform optional FXAA as an anti-aliasing solution.

*/
enum RenderMode
{
	kRenderMode_Flat,
	kRenderMode_DeferredShaded,
	kRenderMode_ForwardShaded
};

struct RenderOptions
{
	RenderMode renderMode = kRenderMode_Flat;
	
	bool enableOpaquePass = true;
	bool drawNormals = false;
	
	bool enableTranslucentPass = true;
	
	Vec3 defaultLightDirection = Vec3(1, -4, 1).CalcNormalized();
	Vec3 defaultLightColorTop = Vec3(1.f, 1.f, 1.f);
	Vec3 defaultLightColorBottom = Vec3(.1f, .1f, .1f);
	bool useDefaultLight = true; // when true and no light drawing function is set, the renderer will use a default directional light
	
	struct ScreenSpaceAmbientOcclusion
	{
		bool enabled = false;
		float strength = 1.f;
		
		static void reflect(TypeDB & typeDB);
	} screenSpaceAmbientOcclusion;
	
	struct Fog
	{
		bool enabled = false;
		float thickness = 0.f;

		static void reflect(TypeDB & typeDB);
	} fog;
	
	bool enableScreenSpaceReflections = false;
	
	struct MotionBlur
	{
		bool enabled = true;
		float strength = 1.f;

		static void reflect(TypeDB & typeDB);
	} motionBlur;
	
	struct SimpleScreenSpaceRefraction
	{
		bool enabled = false;
		float strength = 10.f;

		static void reflect(TypeDB & typeDB);
	} simpleScreenSpaceRefraction;
	
	struct DepthOfField
	{
		bool enabled = false;
		float strength = 1.f;
		float focusDistance = 1.f;

		static void reflect(TypeDB & typeDB);
	} depthOfField;
	
	struct Bloom
	{
		bool enabled = false;
		float strength = .15f;
		float blurSize = 20.f; // todo : make blur blur size resolution independent
		
		// when true, the top-level image from the down sample chain is dropped, speeding up processing significantly
		bool dropTopLevelImage = true;

		static void reflect(TypeDB & typeDB);
	} bloom;
	
	struct LightScatter
	{
		bool enabled = false;
		Vec2 origin = Vec2(.5f, .2f);
		int numSamples = 100;
		float decay = .8f;
		float strength = 1.f;
		float strengthMultiplier = 1.f;
	} lightScatter;
	
	struct DepthSilhouette
	{
		bool enabled = false;
		float strength = 1.f;
		Vec4 color = Vec4(1, 1, 1, 1);

		static void reflect(TypeDB & typeDB);
	} depthSilhouette;
	
	struct DeferredLighting
	{
		bool enableStencilVolumes = true;

		static void reflect(TypeDB & typeDB);
	} deferredLighting;

	bool linearColorSpace = false;
	
	struct ToneMapping
	{
		float exposure = 16.f;
		float gamma = 2.2f;

		static void reflect(TypeDB & typeDB);
	} toneMapping;
	
	struct ColorGrading
	{
		bool enabled = false;

		static void reflect(TypeDB & typeDB);
	} colorGrading;
	
	struct Fxaa
	{
		bool enabled = true;

		static void reflect(TypeDB & typeDB);
	} fxaa;
	
	struct Anaglyphic
	{
		bool enabled = false;
		float eyeDistance = .06f; // 6cm is close to the median pupillary distance for males and females. see https://en.wikipedia.org/wiki/Pupillary_distance

		static void reflect(TypeDB & typeDB);
	} anaglyphic;
	
	bool debugRenderTargets = false;
	
	Vec3 backgroundColor;
	
	static void reflect(TypeDB & typeDB);
};

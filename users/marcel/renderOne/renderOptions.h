#pragma once

#include "Vec3.h"
#include "Vec4.h"

struct TypeDB;

enum RenderMode
{
	kRenderMode_Flat,
	kRenderMode_DeferredShaded
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

		static void reflect(TypeDB & typeDB);
	} bloom;
	
	struct LightScatter
	{
		bool enabled = false;
		Vec2 origin = Vec2(.5f, .2f);
		int numSamples = 100;
		float decay = .1f; // todo : decay should be based on distance, not sample # ?
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

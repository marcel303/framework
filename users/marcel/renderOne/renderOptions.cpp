#include "reflection.h"
#include "renderOptions.h"

namespace rOne
{
	void RenderOptions::DeferredLighting::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::DeferredLighting>("RenderOptions::DeferredLighting")
			.add("defaultLightColorBottom", &RenderOptions::DeferredLighting::useDefaultLight)
			.add("defaultLightDirection", &RenderOptions::DeferredLighting::defaultLightDirection)
			.add("defaultLightColorTop", &RenderOptions::DeferredLighting::defaultLightColorTop)
			.add("defaultLightColorBottom", &RenderOptions::DeferredLighting::defaultLightColorBottom)
			.add("enableStencilVolumes", &RenderOptions::DeferredLighting::enableStencilVolumes);
	}
	
	void RenderOptions::ScreenSpaceAmbientOcclusion::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::ScreenSpaceAmbientOcclusion>("RenderOptions::ScreenSpaceAmbientOcclusion")
			.add("enabled", &RenderOptions::ScreenSpaceAmbientOcclusion::enabled)
			.add("strength", &RenderOptions::ScreenSpaceAmbientOcclusion::strength);
	}

	void RenderOptions::Fog::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::Fog>("RenderOptions::Fog")
			.add("enabled", &RenderOptions::Fog::enabled)
			.add("thickness", &RenderOptions::Fog::thickness);
	}

	void RenderOptions::MotionBlur::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::MotionBlur>("RenderOptions::MotionBlur")
			.add("enabled", &RenderOptions::MotionBlur::enabled)
			.add("strength", &RenderOptions::MotionBlur::strength);
	}

	void RenderOptions::SimpleScreenSpaceRefraction::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::SimpleScreenSpaceRefraction>("RenderOptions::SimpleScreenSpaceRefraction")
			.add("enabled", &RenderOptions::SimpleScreenSpaceRefraction::enabled)
			.add("strength", &RenderOptions::SimpleScreenSpaceRefraction::strength);
	}

	void RenderOptions::DepthOfField::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::DepthOfField>("RenderOptions::DepthOfField")
			.add("enabled", &RenderOptions::DepthOfField::enabled)
			.add("strength", &RenderOptions::DepthOfField::strength)
			.add("focusDistance", &RenderOptions::DepthOfField::focusDistance);
	}
	
	void RenderOptions::ChromaticAberration::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::ChromaticAberration>("RenderOptions::ChromaticAberration")
			.add("enabled", &RenderOptions::ChromaticAberration::enabled)
			.add("strength", &RenderOptions::ChromaticAberration::strength);
	}

	void RenderOptions::Bloom::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::Bloom>("RenderOptions::Bloom")
			.add("enabled", &RenderOptions::Bloom::enabled)
			.add("blurSize", &RenderOptions::Bloom::blurSize)
			.add("strength", &RenderOptions::Bloom::strength)
			.add("brightPassValue", &RenderOptions::Bloom::brightPassValue)
			.add("dropTopLevelImage", &RenderOptions::Bloom::dropTopLevelImage);
	}
	
	void RenderOptions::LightScatter::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::LightScatter>("RenderOptions::LightScatter")
			.add("enabled", &RenderOptions::LightScatter::enabled)
			.add("origin", &RenderOptions::LightScatter::origin)
			.add("numSamples", &RenderOptions::LightScatter::numSamples)
			.add("decay", &RenderOptions::LightScatter::decay)
			.add("strength", &RenderOptions::LightScatter::strength);
	}

	void RenderOptions::DepthSilhouette::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::DepthSilhouette>("RenderOptions::DepthSilhouette")
			.add("enabled", &RenderOptions::DepthSilhouette::enabled)
			.add("strength", &RenderOptions::DepthSilhouette::strength)
			.add("color", &RenderOptions::DepthSilhouette::color);
	}

	void RenderOptions::ToneMapping::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::ToneMapping>("RenderOptions::ToneMapping")
			.add("toneMap", &RenderOptions::ToneMapping::toneMap)
			.add("exposure", &RenderOptions::ToneMapping::exposure)
			.add("gamma", &RenderOptions::ToneMapping::gamma);
	}

	void RenderOptions::ColorGrading::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::ColorGrading>("RenderOptions::ColorGrading")
			.add("enabled", &RenderOptions::ColorGrading::enabled);
	}

	void RenderOptions::Fxaa::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::Fxaa>("RenderOptions::Fxaa")
			.add("enabled", &RenderOptions::Fxaa::enabled);
	}

	void RenderOptions::Anaglyphic::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<RenderOptions::Anaglyphic>("RenderOptions::Anaglyphic")
			.add("enabled", &RenderOptions::Anaglyphic::enabled)
			.add("eyeDistance", &RenderOptions::Anaglyphic::eyeDistance);
	}

	void RenderOptions::reflect(TypeDB & typeDB)
	{
		typeDB.addEnum<RenderMode>("RenderMode")
			.add("flat", kRenderMode_Flat)
			.add("deferredShaded", kRenderMode_DeferredShaded)
			.add("forwardShaded", kRenderMode_ForwardShaded);
		
		typeDB.addEnum<ToneMap>("ToneMap")
			.add("uncharted2", kToneMap_Uncharted2)
			.add("jimrichard", kToneMap_JimRichard)
			.add("reinhard", kToneMap_Reinhard)
			.add("aces", kToneMap_Aces);
		
		DeferredLighting::reflect(typeDB);
		
		ScreenSpaceAmbientOcclusion::reflect(typeDB);
		Fog::reflect(typeDB);
		MotionBlur::reflect(typeDB);
		SimpleScreenSpaceRefraction::reflect(typeDB);
		DepthOfField::reflect(typeDB);
		ChromaticAberration::reflect(typeDB);
		Bloom::reflect(typeDB);
		LightScatter::reflect(typeDB);
		DepthSilhouette::reflect(typeDB);
		ToneMapping::reflect(typeDB);
		ColorGrading::reflect(typeDB);
		Fxaa::reflect(typeDB);
		Anaglyphic::reflect(typeDB);
		
		typeDB.addStructured<RenderOptions>("RenderOptions")
			.add("renderMode", &RenderOptions::renderMode)
			// render pass enables
			.add("enableOpaquePass", &RenderOptions::enableOpaquePass)
			.add("drawNormals", &RenderOptions::drawNormals)
			.add("enableTranslucentPass", &RenderOptions::enableTranslucentPass)
			.add("enableBackgroundPass", &RenderOptions::enableBackgroundPass)
			// deferred shading
			.add("deferredLighting", &RenderOptions::deferredLighting)
			// stage : lighting
			.add("screenSpaceAmbientOcclusion", &RenderOptions::screenSpaceAmbientOcclusion)
			.add("fog", &RenderOptions::fog)
			.add("enableScreenSpaceReflections", &RenderOptions::enableScreenSpaceReflections)
			// stage : camera-exposure
			.add("motionBlur", &RenderOptions::motionBlur)
			.add("simpleScreenSpaceRefraction", &RenderOptions::simpleScreenSpaceRefraction)
			// stage : camera-lense
			.add("depthOfField", &RenderOptions::depthOfField)
			.add("chromaticAberration", &RenderOptions::chromaticAberration)
			.add("bloom", &RenderOptions::bloom)
			.add("lightScatter", &RenderOptions::lightScatter)
			// stage : ?
			.add("depthSilhouette", &RenderOptions::depthSilhouette)
			// stage : tone mapping
			.add("linearColorSpace", &RenderOptions::linearColorSpace)
			.add("toneMapping", &RenderOptions::toneMapping)
			// stage : sRGB color-space operations on the near-final image
			.add("colorGrading", &RenderOptions::colorGrading)
			.add("fxaa", &RenderOptions::fxaa)
			// stereo rendering
			.add("anaglyphic", &RenderOptions::anaglyphic)
			// debug
			.add("debugRenderTargets", &RenderOptions::debugRenderTargets)
			// globals
			.add("backgroundColor", &RenderOptions::backgroundColor);
	}
}

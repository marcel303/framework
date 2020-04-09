#include "reflection.h"
#include "renderOptions.h"

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

void RenderOptions::Bloom::reflect(TypeDB & typeDB)
{
	typeDB.addStructured<RenderOptions::Bloom>("RenderOptions::Bloom")
		.add("enabled", &RenderOptions::Bloom::enabled)
		.add("blurSize", &RenderOptions::Bloom::blurSize)
		.add("strength", &RenderOptions::Bloom::strength)
		.add("brightPassValue", &RenderOptions::Bloom::brightPassValue)
		.add("dropTopLevelImage", &RenderOptions::Bloom::dropTopLevelImage);
}

void RenderOptions::DepthSilhouette::reflect(TypeDB & typeDB)
{
	typeDB.addStructured<RenderOptions::DepthSilhouette>("RenderOptions::DepthSilhouette")
		.add("enabled", &RenderOptions::DepthSilhouette::enabled)
		.add("strength", &RenderOptions::DepthSilhouette::strength)
		.add("color", &RenderOptions::DepthSilhouette::color);
}

void RenderOptions::DeferredLighting::reflect(TypeDB & typeDB)
{
	typeDB.addStructured<RenderOptions::DeferredLighting>("RenderOptions::DeferredLighting")
		.add("enableStencilVolumes", &RenderOptions::DeferredLighting::enableStencilVolumes);
}

void RenderOptions::ToneMapping::reflect(TypeDB & typeDB)
{
	typeDB.addStructured<RenderOptions::ToneMapping>("RenderOptions::ToneMapping")
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
	
	Fog::reflect(typeDB);
	MotionBlur::reflect(typeDB);
	SimpleScreenSpaceRefraction::reflect(typeDB);
	DepthOfField::reflect(typeDB);
	Bloom::reflect(typeDB);
	DepthSilhouette::reflect(typeDB);
	DeferredLighting::reflect(typeDB);
	ToneMapping::reflect(typeDB);
	ColorGrading::reflect(typeDB);
	Fxaa::reflect(typeDB);
	Anaglyphic::reflect(typeDB);
	
	typeDB.addStructured<RenderOptions>("RenderOptions")
		.add("renderMode", &RenderOptions::renderMode)
		.add("enableOpaquePass", &RenderOptions::enableOpaquePass)
		.add("drawNormals", &RenderOptions::drawNormals)
		.add("enableTranslucentPass", &RenderOptions::enableTranslucentPass)
		.add("fog", &RenderOptions::fog)
		.add("enableScreenSpaceReflections", &RenderOptions::enableScreenSpaceReflections)
		.add("motionBlur", &RenderOptions::motionBlur)
		.add("simpleScreenSpaceRefraction", &RenderOptions::simpleScreenSpaceRefraction)
		.add("depthOfField", &RenderOptions::depthOfField)
		.add("bloom", &RenderOptions::bloom)
		.add("depthSilhouette", &RenderOptions::depthSilhouette)
		.add("deferredLighting", &RenderOptions::deferredLighting)
		.add("linearColorSpace", &RenderOptions::linearColorSpace)
		.add("toneMapping", &RenderOptions::toneMapping)
		.add("colorGrading", &RenderOptions::colorGrading)
		.add("fxaa", &RenderOptions::fxaa)
		.add("anaglyphic", &RenderOptions::anaglyphic)
		.add("debugRenderTargets", &RenderOptions::debugRenderTargets)
		.add("backgroundColor", &RenderOptions::backgroundColor);
}

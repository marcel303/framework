#include "framework.h"
#include "lightComponent.h"

static const char * s_deferredLightVs = "";
static const char * s_deferredLightPs = "";
static const char * s_deferredLightWithShadowVs = "";
static const char * s_deferredLightWithShadowPs = "";

static const char * s_lightApplicationVs = "";
static const char * s_lightApplicationPs = "";

bool LightComponentMgr::init()
{
	bool result = true;
	
	shaderSource("lightMgr/deferredLight.vs", s_deferredLightVs);
	shaderSource("lightMgr/deferredLight.ps", s_deferredLightPs);
	shaderSource("lightMgr/deferredLightWithShadow.vs", s_deferredLightWithShadowVs);
	shaderSource("lightMgr/deferredLightWithShadow.ps", s_deferredLightWithShadowPs);
	
	shaderSource("lightMgr/lightApplication.vs", s_lightApplicationVs);
	shaderSource("lightMgr/lightApplication.ps", s_lightApplicationPs);
	
	SurfaceProperties properties;
	properties.dimensions.init(800, 600); // todo : dynamically resize during draw
	properties.colorTarget.init(SURFACE_RGBA16F, false);
	lightAccumulationSurface = new Surface();
	result &= lightAccumulationSurface->init(properties);
	
	return result;
}

void LightComponentMgr::shut()
{
	delete lightAccumulationSurface;
	lightAccumulationSurface = nullptr;
}

void LightComponentMgr::drawDeferredLights() const
{
	pushSurface(lightAccumulationSurface);
	{
		// todo : iterate over all of the lights
		
		for (;;)
		{
			// todo : draw light to light acculation buffer
		}
	}
	popSurface();
}

void LightComponentMgr::drawDeferredLightApplication(const int colorTextureId) const
{
	// apply light accumulation buffer to color texture
	
	Shader shader("lightMgr/lightApplication");
	setShader(shader);
	{
		shader.setTexture("colorTexture", 0, colorTextureId);
		
		pushDepthTest(false, DEPTH_ALWAYS);
		pushBlend(BLEND_OPAQUE);
		{
			drawRect(0, 0, lightAccumulationSurface->getWidth(), lightAccumulationSurface->getHeight());
		}
		popBlend();
		popDepthTest();
	}
	clearShader();
}

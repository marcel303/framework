#include "shadowMapDrawer.h"
#include <algorithm>

namespace rOne
{
	void ShadowMapDrawer::calculateProjectionMatrixForLight(const Light & light, Mat4x4 & projectionMatrix)
	{
		if (light.type == kLightType_Spot)
		{
			const float aspectRatio = 1.f;
			
		#if ENABLE_OPENGL
			projectionMatrix.MakePerspectiveGL(
				light.spotAngle,
				aspectRatio,
				light.nearDistance,
				light.farDistance);
		#else
			projectionMatrix.MakePerspectiveLH(
				light.spotAngle,
				aspectRatio,
				light.nearDistance,
				light.farDistance);
		#endif
		}
		else if (light.type == kLightType_Directional)
		{
		#if ENABLE_OPENGL
			projectionMatrix.MakeOrthoGL(
				-light.directionalExtents,
				+light.directionalExtents,
				-light.directionalExtents,
				+light.directionalExtents,
				light.nearDistance,
				light.farDistance);
		#else
			projectionMatrix.MakeOrthoLH(
				-light.directionalExtents,
				+light.directionalExtents,
				-light.directionalExtents,
				+light.directionalExtents,
				light.nearDistance,
				light.farDistance);
		#endif
		}
		else
		{
			Assert(false);
			projectionMatrix.MakeIdentity();
		}
	}

	void ShadowMapDrawer::generateDepthAtlas()
	{
		if (shadowMapFilter == kShadowMapFilter_Nearest ||
			shadowMapFilter == kShadowMapFilter_PercentageCloser_3x3 ||
			shadowMapFilter == kShadowMapFilter_PercentageCloser_7x7)
		{
			pushRenderPass(&depthAtlas, true, nullptr, false, "Shadow depth atlas");
			pushBlend(BLEND_OPAQUE);
			{
				projectScreen2d();
				
				int x = 0;
				
				for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
				{
					gxLoadIdentity();
					gxTranslatef(x, 0, 0);
					
					auto & depthTarget = depthTargets[i];
					
					Shader shader("renderOne/blit-texture");
					setShader(shader);
					{
						shader.setTexture("source", 0, depthTarget.getTextureId(), false, true);
						drawRect(0, 0, depthTarget.getWidth(), depthTarget.getHeight());
					}
					clearShader();
					
					x += depthTarget.getWidth();
				}
			}
			popBlend();
			popRenderPass();
		}
		else
		{
			Assert(shadowMapFilter == kShadowMapFilter_Variance);
			
			pushRenderPass(&depthAtlas2Channel, true, nullptr, false, "Shadow depth atlas (VSM)");
			pushBlend(BLEND_OPAQUE);
			{
				projectScreen2d();
				
				int x = 0;
				
				for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
				{
					gxLoadIdentity();
					gxTranslatef(x, 0, 0);
					
					auto & depthTarget = depthTargets[i];
					
					Shader shader("renderOne/shadow-mapping/blit-texture-vsm");
					setShader(shader);
					{
						shader.setTexture("source", 0, depthTarget.getTextureId(), false, true);
						drawRect(0, 0, depthTarget.getWidth(), depthTarget.getHeight());
					}
					clearShader();
					
					x += depthTarget.getWidth();
				}
			}
			popBlend();
			popRenderPass();
		}
	}

	void ShadowMapDrawer::generateColorAtlas()
	{
		pushRenderPass(&colorAtlas, true, nullptr, false, "Shadow color atlas");
		pushBlend(BLEND_OPAQUE);
		{
			projectScreen2d();
			
			int x = 0;
		
			for (size_t i = 0; i < lights.size() && i < colorTargets.size(); ++i)
			{
				gxLoadIdentity();
				gxTranslatef(x, 0, 0);
				
				auto & colorTarget = colorTargets[i];
			
				Shader shader("renderOne/blit-texture");
				setShader(shader);
				{
					shader.setTexture("source", 0, colorTarget.getTextureId(), false, true);
					drawRect(0, 0, colorTarget.getWidth(), colorTarget.getHeight());
				}
				clearShader();
				
				x += colorTarget.getWidth();
			}
		}
		popBlend();
		popRenderPass();
	}

	ShadowMapDrawer::~ShadowMapDrawer()
	{
		free();
	}

	void ShadowMapDrawer::alloc(const int maxShadowMaps, const int resolution)
	{
		free();
		
		//
		
		depthTargets.resize(maxShadowMaps);
		for (auto & depthTarget : depthTargets)
			depthTarget.init(resolution, resolution, DEPTH_FLOAT32, true, 1.f);
		
		colorTargets.resize(maxShadowMaps);
		for (auto & colorTarget : colorTargets)
			colorTarget.init(resolution, resolution, SURFACE_RGBA8, colorWhite);
		
		depthAtlas.init(maxShadowMaps * resolution, resolution, SURFACE_R32F, colorBlackTranslucent);
		colorAtlas.init(maxShadowMaps * resolution, resolution, SURFACE_RGBA8, colorWhite);
		
		depthAtlas2Channel.init(maxShadowMaps * resolution, resolution, SURFACE_RG32F, colorBlackTranslucent);
	}

	void ShadowMapDrawer::free()
	{
		for (auto & depthTarget : depthTargets)
			depthTarget.free();
		depthTargets.clear();
		
		for (auto & colorTarget : colorTargets)
			colorTarget.free();
		colorTargets.clear();
		
		depthAtlas.free();
		colorAtlas.free();
	}

	void ShadowMapDrawer::addSpotLight(
		const int id,
		const Mat4x4 & lightToWorld,
		const float angle,
		const float nearDistance,
		const float farDistance)
	{
		Light light;
		light.type = kLightType_Spot;
		light.id = id;
		light.lightToWorld = lightToWorld;
		light.worldToLight = lightToWorld.CalcInv();
		light.spotAngle = angle;
		light.nearDistance = nearDistance;
		light.farDistance = farDistance;
		
		lights.push_back(light);
	}

	void ShadowMapDrawer::addDirectionalLight(
		const int id,
		const Mat4x4 & lightToWorld,
		const float startDistance,
		const float endDistance,
		const float extents)
	{
		Light light;
		light.type = kLightType_Directional;
		light.id = id;
		light.lightToWorld = lightToWorld;
		light.worldToLight = lightToWorld.CalcInv();
		light.nearDistance = startDistance;
		light.farDistance = endDistance;
		light.directionalExtents = extents;
		
		lights.push_back(light);
	}

	void ShadowMapDrawer::drawShadowMaps(const Mat4x4 & worldToView)
	{
		// prioritize lights
		
		for (auto & light : lights)
		{
			if (light.type == kLightType_Spot)
			{
				const Vec3 lightPosition_view = worldToView.Mul4(light.lightToWorld.GetTranslation());
				
				light.viewDistance = lightPosition_view.CalcSize();
			}
			else if (light.type == kLightType_Directional)
			{
				light.viewDistance = 0.f;
			}
			else
			{
				Assert(false);
			}
		}

		std::sort(lights.begin(), lights.end(),
			[](const Light & light1, const Light & light2)
			{
				return light1.viewDistance < light2.viewDistance;
			});

		std::vector<Light> lightsToDraw;
		
		for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
		{
			lightsToDraw.push_back(lights[i]);
		}
		
		// draw shadow maps
		
		const int bias =
			(shadowMapFilter == kShadowMapFilter_Nearest) ? 4 :
			(shadowMapFilter == kShadowMapFilter_PercentageCloser_3x3) ? 3 :
			(shadowMapFilter == kShadowMapFilter_PercentageCloser_7x7) ? 4 :
			(shadowMapFilter == kShadowMapFilter_Variance) ? 0 :
			-1;
		
		Assert(bias != -1);
		
		for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
		{
			auto & light = lights[i];
			
			pushRenderPass(nullptr, false, &depthTargets[i], true, "Shadow depth");
			pushDepthTest(true, DEPTH_LESS);
			pushColorWriteMask(0, 0, 0, 0);
			pushBlend(BLEND_OPAQUE);
			pushDepthBias(bias, bias);
			pushShaderOutputs("");
			{
				if (light.type == kLightType_Spot)
				{
					Mat4x4 viewToProjection;
					calculateProjectionMatrixForLight(light, viewToProjection);
					
					gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
					gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
					
					if (drawOpaque != nullptr)
						drawOpaque();
				}
				else if (light.type == kLightType_Directional)
				{
					Mat4x4 viewToProjection;
					calculateProjectionMatrixForLight(light, viewToProjection);
					
					gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
					gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
					
					if (drawOpaque != nullptr)
						drawOpaque();
				}
			}
			popShaderOutputs();
			popDepthBias();
			popBlend();
			popColorWriteMask();
			popDepthTest();
			popRenderPass();
		}
		
		if (enableColorShadows)
		{
			for (size_t i = 0; i < lights.size() && i < colorTargets.size(); ++i)
			{
				auto & light = lights[i];
				
				pushRenderPass(&colorTargets[i], true, &depthTargets[i], false, "Shadow color");
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ALPHA); // todo : use blend mode where color and alpha are invert multiplied to generate an opacity mask
				{
					if (light.type == kLightType_Spot)
					{
						Mat4x4 viewToProjection;
						calculateProjectionMatrixForLight(light, viewToProjection);
						
						gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
						gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
						
						if (drawTranslucent != nullptr)
							drawTranslucent();
					}
					else if (light.type == kLightType_Directional)
					{
						Mat4x4 viewToProjection;
						calculateProjectionMatrixForLight(light, viewToProjection);
						
						gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
						gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
						
						if (drawOpaque != nullptr)
							drawOpaque();
					}
					else
					{
						Assert(false);
					}
				}
				popBlend();
				popDepthTest();
				popRenderPass();
			}
		}
		
		generateDepthAtlas();
		
		if (enableColorShadows)
		{
			generateColorAtlas();
		}
	}

	void ShadowMapDrawer::setShaderData(Shader & shader, int & nextTextureUnit, const Mat4x4 & worldToView)
	{
		shader.setTexture("shadowDepthAtlas", nextTextureUnit++, depthAtlas.getTextureId(), false, true);
		shader.setTexture("shadowColorAtlas", nextTextureUnit++, enableColorShadows ? colorAtlas.getTextureId() : 0, true, true);

		shader.setTexture("shadowDepthAtlas2Channel", nextTextureUnit++, depthAtlas2Channel.getTextureId(), true, true);
		
		shader.setImmediate("numShadowMaps", depthTargets.size());
		shader.setImmediate("enableColorShadows", enableColorShadows ? 1.f : 0.f);
		shader.setImmediate("shadowMapFilter", shadowMapFilter);

	// todo : compute shadow matrices only once and store in a buffer
		const Mat4x4 viewToWorld = worldToView.CalcInv();
		
		const int numShadowMatrices = lights.size();
		Mat4x4 * shadowMatrices = (Mat4x4*)alloca(numShadowMatrices * sizeof(Mat4x4));
		
		for (size_t i = 0; i < lights.size(); ++i)
		{
			auto & light = lights[i];
			auto & shadowMatrix = shadowMatrices[i];
			
			Mat4x4 lightToProjection;
			calculateProjectionMatrixForLight(light, lightToProjection);
			
			shadowMatrix = lightToProjection * light.worldToLight * viewToWorld;
		}
		
		shader.setImmediateMatrix4x4Array("viewToShadowMatrices", (float*)shadowMatrices, numShadowMatrices);
		
		for (size_t i = 0; i < lights.size(); ++i)
			shadowMatrices[i] = shadowMatrices[i].CalcInv();
		
		shader.setImmediateMatrix4x4Array("shadowToViewMatrices", (float*)shadowMatrices, numShadowMatrices);
	}

	int ShadowMapDrawer::getShadowMapId(const int id) const
	{
		for (size_t i = 0; i < lights.size(); ++i)
			if (lights[i].id == id)
				return i;
		
		return -1;
	}

	void ShadowMapDrawer::reset()
	{
		lights.clear();
	}

	void ShadowMapDrawer::showRenderTargets() const
	{
		const int sx = 40;
		const int sy = 40;
		
		pushBlend(BLEND_OPAQUE);
		gxPushMatrix();
		{
			setColor(colorWhite);
			
			gxPushMatrix();
			{
				for (auto & depthTarget : depthTargets)
				{
					gxSetTexture(depthTarget.getTextureId());
					setLumif(.1f);
					drawRect(0, 0, sx, sy);
					setLumif(1.f);
					gxSetTexture(0);
					gxTranslatef(sx, 0, 0);
				}
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
			
			gxPushMatrix();
			{
				for (auto & colorTarget : colorTargets)
				{
					gxSetTexture(colorTarget.getTextureId());
					drawRect(0, 0, sx, sy);
					gxSetTexture(0);
					gxTranslatef(sx, 0, 0);
				}
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
			
			gxPushMatrix();
			{
				pushColorPost(POST_SET_RGB_TO_R);
				gxSetTexture(depthAtlas.getTextureId());
				setLumif(.2f);
				drawRect(0, 0, sx * depthTargets.size(), sy);
				setLumif(1.f);
				gxSetTexture(0);
				popColorPost();
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
		
			gxPushMatrix();
			{
				gxSetTexture(colorAtlas.getTextureId());
				drawRect(0, 0, sx * depthTargets.size(), sy);
				gxSetTexture(0);
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
		}
		gxPopMatrix();
		popBlend();
	}
}

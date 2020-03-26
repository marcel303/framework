#include "framework.h"
#include "gx_render.h"
#include "lightMgr.h"
#include "renderer.h"

#if 0
	// todo : add screenshot functionality

	#include <SDL2/SDL_opengl.h>
	#include "ImageLoader_Tga.h"

	static bool firstFrame = true;
	if (firstFrame)
	{
		firstFrame = false;
		
		Surface * surface = new Surface(8000, 6000, false);
		
		pushSurface(surface);
		{
			surface->clearf(
				monitorGui.visibility.backgroundColor.r,
				monitorGui.visibility.backgroundColor.g,
				monitorGui.visibility.backgroundColor.b,
				1.f);
			pushLineSmooth(true);
			draw();
			popLineSmooth();
			
			// fetch the pixel data
	
			uint8_t * bytes = new uint8_t[surface->getWidth() * surface->getHeight() * 4];
			
			glReadPixels(
				0, 0,
				surface->getWidth(),
				surface->getHeight(),
				GL_BGRA, GL_UNSIGNED_BYTE,
				bytes);
			checkErrorGL();
			
			ImageLoader_Tga tgaLoader;
			tgaLoader.SaveBGRA_vflipped(bytes, surface->getWidth(), surface->getHeight(), "test.tga", false);
			
			delete [] bytes;
			bytes = nullptr;
		}
		popSurface();
		
		delete surface;
		surface = nullptr;
	}
#endif

static const float kSrgbToLinear = 2.2f;

static Vec3 srgbToLinear(const Vec3 & rgb)
{
	return Vec3(
		powf(rgb[0], kSrgbToLinear),
		powf(rgb[1], kSrgbToLinear),
		powf(rgb[2], kSrgbToLinear));
}

// render passes

static void renderOpaquePass(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
{
	pushDepthTest(true, DEPTH_LESS, true);
	pushBlend(BLEND_OPAQUE);
	{
		if (renderOptions.enableOpaquePass && renderFunctions.drawOpaque != nullptr)
		{
			renderFunctions.drawOpaque();
		}
	}
	popBlend();
	popDepthTest();
}

static void renderTranslucentPass(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
{
	pushDepthTest(true, DEPTH_LESS, false);
	pushBlend(BLEND_ADD);
	{
		if (renderOptions.enableTranslucentPass && renderFunctions.drawTranslucent != nullptr)
		{
			renderFunctions.drawTranslucent();
		}
	}
	popBlend();
	popDepthTest();
}

// render buffers

static void renderLightBuffer(
	const RenderFunctions & renderFunctions,
	const RenderOptions & renderOptions,
	const GxTextureId depth,
	const GxTextureId normals,
	const GxTextureId colors,
	const GxTextureId specularColor,
	const GxTextureId specularExponent,
	const int viewportSx,
	const int viewportSy,
	const Mat4x4 & projectionMatrix,
	const Mat4x4 & viewMatrix)
{
	pushBlend(BLEND_ADD_OPAQUE);
	g_lightDrawer.drawDeferredBegin(
		viewMatrix, projectionMatrix,
		depth, normals, colors, specularColor, specularExponent,
		renderOptions.deferredLighting.enableStencilVolumes);
	{
		if (renderFunctions.drawLights != nullptr)
		{
			renderFunctions.drawLights();
		}
		else
		{
		#if 0
			setColor(colorWhite);
			drawRect(0, 0, viewportSx, viewportSy);
		#endif
		
		#if 1
			{
				const Mat4x4 rotationMatrix = Mat4x4(true).RotateY(framework.time / 2.f).RotateX(sinf(framework.time)/2.f);
				const Vec3 lightDirection = rotationMatrix.Mul3(Vec3(.3f, -3.f, .6f)).CalcNormalized();
				const Vec3 lightColor1 = Vec3(1.f, .8f, .6f); // light color when the light is coming from 'above'
				const Vec3 lightColor2 = Vec3(.1f, .2f, .3f); // light color when the light is coming from 'below'
				
				g_lightDrawer.drawDeferredDirectionalLight(lightDirection, lightColor1, lightColor2);
			}
		#endif
		
		#if 1
			{
				const Vec3 lightPosition = Vec3(cosf(framework.time) * 3.f, 3, sinf(framework.time) * 3.f);
				const float lightAttenuationBegin = 0.f;
				const float lightAttenuationEnd = 10.f;
				const Vec3 lightColor = Vec3(.8f, .9f, 1.f) * 2.f;
				
				g_lightDrawer.drawDeferredPointLight(
					lightPosition,
					lightAttenuationBegin,
					lightAttenuationEnd,
					lightColor);
			}
		#endif
		}
	}
	g_lightDrawer.drawDeferredEnd();
	popBlend();
}

static void renderLightCompositeBuffer(
	const GxTextureId light,
	const GxTextureId colors,
	const GxTextureId emissive,
	const GxTextureId depth,
	const int sx, const int sy,
	const bool linearColorSpace)
{
	pushBlend(BLEND_OPAQUE);
	{
		Shader shader("renderOne/light-application");
		setShader(shader);
		{
			shader.setTexture("colorTexture", 0, colors, false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
			shader.setTexture("lightTexture", 1, light, false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
			shader.setTexture("emissiveTexture", 3, emissive, false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
			shader.setTexture("depthTexture", 4, depth, false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
			shader.setImmediate("linearColorSpace", linearColorSpace ? 1.f : 0.f);
		
			drawRect(0, 0, sx, sy);
		}
		clearShader();
	}
	popBlend();
}

// render modes

static ColorTarget * renderModeFlat(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
{
	pushShaderOutputs(renderOptions.drawNormals ? "n" : "c");
	{
		renderOpaquePass(renderFunctions, renderOptions);
	}
	popShaderOutputs();
	
	renderTranslucentPass(renderFunctions, renderOptions);

	return nullptr;
}

static ColorTarget * renderModeDeferredShaded(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions, RenderBuffers & buffers, const int viewportSx, const int viewportSy, const float timeStep)
{
	Mat4x4 modelViewMatrix;
	Mat4x4 projectionMatrix;
	gxGetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
	gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
	
	// draw colors + normals
	
	ColorTarget * targets[5] =
		{
			buffers.normals,
			buffers.colors,
			buffers.specularColor,
			buffers.specularExponent,
			buffers.emissive
		};
	pushRenderPass(targets, 5, true, buffers.depth, true, "Normals + Colors + Specular + Emissive & Depth");
	pushShaderOutputs("ncSse"); // todo : drawNormals -> "nn". currently bugs when generating shader
	{
		gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
		
	#if ENABLE_OPENGL
		gxMatrixMode(GX_PROJECTION);
		gxScalef(1, -1, 1); // todo : remove the need to scale here
		gxMatrixMode(GX_MODELVIEW);
	#endif
		
		renderOpaquePass(renderFunctions, renderOptions);
	}
	popShaderOutputs();
	popRenderPass();
	
	// create velocity buffer
	
	{
		const int kHistorySize = 100;
		static Mat4x4 projectionToWorld_prev(true); // todo : per-eye storage
		
		static int projectionToWorld_prev_hist_idx = -1; // todo : per-eye storage
		static Mat4x4 projectionToWorld_prev_hist[kHistorySize]; // todo : per-eye storage
		
		const Mat4x4 worldToProjection = projectionMatrix * modelViewMatrix;
		const Mat4x4 projectionToWorld_curr = worldToProjection.CalcInv();
		
		const bool needsVelocityBuffer = renderOptions.motionBlur.enabled;
		
	#if 0 // enable to use a weird effect created by delayed reprojection matrices
		if (needsVelocityBuffer == false)
			projectionToWorld_prev_hist_idx = -1;
		else
		{
			const Mat4x4 projectionToWorld = worldToProjection.CalcInv();
			
			if (projectionToWorld_prev_hist_idx == -1)
			{
				for (auto & m : projectionToWorld_prev_hist)
					m = projectionToWorld;
			}
			else
			{
				projectionToWorld_prev_hist[projectionToWorld_prev_hist_idx] = projectionToWorld;
			}
			
			projectionToWorld_prev_hist_idx = (projectionToWorld_prev_hist_idx + 1) % kHistorySize;
		}
		
		projectionToWorld_prev = projectionToWorld_prev_hist[projectionToWorld_prev_hist_idx];
	#endif
		
		if (needsVelocityBuffer)
		{
			pushRenderPass(buffers.velocity, true, nullptr, false, "Velocity");
			{
				projectScreen2d();
				
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("renderOne/depth-to-velocity");
					setShader(shader);
					{
						shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setImmediateMatrix4x4("projectionToWorld_curr", projectionToWorld_curr.m_v);
						shader.setImmediateMatrix4x4("projectionToWorld_prev", projectionToWorld_prev.m_v);
						shader.setImmediateMatrix4x4("worldToView", modelViewMatrix.m_v);
						shader.setImmediateMatrix4x4("worldToProjection", worldToProjection.m_v);
						shader.setImmediate("timeStepRcp", 1.f / timeStep);
						drawRect(0, 0, viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
		}
		
		projectionToWorld_prev = projectionToWorld_curr;
	}
	
	// apply tri-planar texture projection test
	
	if (false)
	{
		pushRenderPass(buffers.normals, false, nullptr, false, "Tri-planer test");
		{
			projectScreen2d();
			
			pushBlend(BLEND_ADD_OPAQUE);
			{
				const Mat4x4 projectionToView = projectionMatrix.CalcInv();
				const Mat4x4 viewToWorld = modelViewMatrix.CalcInv();
				const Mat4x4 worldToView = modelViewMatrix;
				
				Shader shader("renderOne/tri-planar-texture-projection");
				setShader(shader);
				{
				// todo : ping pong composite buffer for screen space effects
					shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("normalTexture", 1, buffers.normals->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("planarTextureX", 3, getTexture("textures/refraction/droplets.png"), true, false);
					shader.setTexture("planarTextureY", 4, getTexture("textures/refraction/droplets.png"), true, false);
					shader.setTexture("planarTextureZ", 5, getTexture("textures/refraction/droplets.png"), true, false);
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
					shader.setImmediateMatrix4x4("viewToWorld", viewToWorld.m_v);
					shader.setImmediateMatrix4x4("worldToView", worldToView.m_v);
					shader.setImmediate("time", framework.time);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
	}

	// accumulate lights
	
	pushRenderPass(
		buffers.light, true,
		renderOptions.deferredLighting.enableStencilVolumes
		? buffers.depth
		: nullptr, false,
		"Light");
	{
		projectScreen2d();
		
		renderLightBuffer(
			renderFunctions,
			renderOptions,
			buffers.depth->getTextureId(),
			buffers.normals->getTextureId(),
			buffers.colors->getTextureId(),
			buffers.specularColor->getTextureId(),
			buffers.specularExponent->getTextureId(),
			viewportSx,
			viewportSy,
			projectionMatrix,
			modelViewMatrix);
	}
	popRenderPass();
	
	// setup composite ping-pong buffers
	
	ColorTarget * composite[2] =
	{
		buffers.composite1,
		buffers.composite2
	};
	
	int composite_idx = 0;
	
	// apply lighting
	
	pushRenderPass(composite[composite_idx], true, nullptr, false, "Light application");
	{
		projectScreen2d();
		
		renderLightCompositeBuffer(
			buffers.light->getTextureId(),
			buffers.colors->getTextureId(),
			buffers.emissive->getTextureId(),
			buffers.depth->getTextureId(), viewportSx, viewportSy,
			renderOptions.linearColorSpace);
	}
	popRenderPass();
	
	// apply fog
	
	if (renderOptions.fog.enabled)
	{
		pushRenderPass(composite[composite_idx], false, nullptr, false, "Fog");
		{
			projectScreen2d();
			
			pushBlend(BLEND_ALPHA);
			{
				const Mat4x4 projectionToView = projectionMatrix.CalcInv();
				const Vec3 fogColor =
					renderOptions.linearColorSpace
					? srgbToLinear(renderOptions.backgroundColor)
					: renderOptions.backgroundColor;
				
				Shader shader("renderOne/fog-application");
				setShader(shader);
				{
					shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
					shader.setImmediate("fogColor",
						fogColor[0],
						fogColor[1],
						fogColor[2]);
					shader.setImmediate("fogTranslucency", 1.f - renderOptions.fog.thickness);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
	}
	
	// apply screen-space reflections
	
	if (renderOptions.enableScreenSpaceReflections)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Reflections (SSR)");
		{
			projectScreen2d();
			pushBlend(BLEND_OPAQUE);
			{
				const Mat4x4 viewToProjection = projectionMatrix;
				const Mat4x4 projectionToView = projectionMatrix.CalcInv();
				
				Shader shader("renderOne/screen-space-reflection");
				setShader(shader);
				{
					shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("normalTexture", 1, buffers.normals->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("colorTexture", 2, composite[composite_idx]->getTextureId(), true, false); // note : clamp is intentionally turned off, to expose incorrect fading
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
					shader.setImmediateMatrix4x4("viewToProjection", viewToProjection.m_v);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}
	
	// composite translucents on top of the lit opaque
	
	pushRenderPass(composite[composite_idx], false, buffers.depth, false, "Translucent");
	{
		gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
		
	#if ENABLE_OPENGL
		gxMatrixMode(GX_PROJECTION);
		gxScalef(1, -1, 1); // todo : remove the need to scale here
		gxMatrixMode(GX_MODELVIEW);
	#endif
		
		renderTranslucentPass(renderFunctions, renderOptions);
	}
	popRenderPass();
	
	// apply screen-space motion blur
	
	if (renderOptions.motionBlur.enabled)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Motion blur");
		{
			projectScreen2d();
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/screen-space-motion-blur");
				setShader(shader);
				{
					shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), true, true);
					shader.setTexture("velocityTexture", 1, buffers.velocity->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setImmediate("strength", renderOptions.motionBlur.strength);
					shader.setImmediate("viewportSizeRcp", 1.f / viewportSx, 1.f / viewportSy);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}
	
	// apply depth of field
	
	if (renderOptions.depthOfField.enabled)
	{
	#if 0 // todo : add a scatter DoF render option
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Depth of field");
		{
			projectScreen2d();
			
			pushBlend(BLEND_ADD);
			{
				const Mat4x4 projectionToView = projectionMatrix.CalcInv();

				Shader shader("renderOne/scatter-dof");
				setShader(shader);
				{
					shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), true, true);
					shader.setTexture("depthTexture", 1, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
					shader.setImmediate("strength", renderOptions.depthOfField.strength);
					shader.setImmediate("focusDistance", renderOptions.depthOfField.focusDistance);
					gxEmitVertices(GX_TRIANGLES, 6 * 800 * 600);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	#else
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Depth of field");
		{
			projectScreen2d();
			
			pushBlend(BLEND_OPAQUE);
			{
				const Mat4x4 projectionToView = projectionMatrix.CalcInv();
				
				Shader shader("renderOne/simple-depth-of-field");
				setShader(shader);
				{
					shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), true, true);
					shader.setTexture("depthTexture", 1, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
					shader.setImmediate("strength", renderOptions.depthOfField.strength);
					shader.setImmediate("focusDistance", renderOptions.depthOfField.focusDistance);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	#endif
	}
	
	// apply bloom

#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
	if (renderOptions.bloom.enabled)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		ColorTarget * src = composite[composite_idx];
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Bloom blur H");
		{
			pushBlend(BLEND_OPAQUE);
			setShader_GaussianBlurH(src->getTextureId(), 11, renderOptions.bloom.blurSize);
			{
				drawRect(0, 0, src->getWidth(), src->getHeight());
			}
			clearShader();
			popBlend();
		}
		popRenderPass();
		
		src = composite[next_composite_idx];
		
		pushRenderPass(buffers.bloomBuffer, true, nullptr, false, "Bloom blur V");
		{
			pushBlend(BLEND_OPAQUE);
			setShader_GaussianBlurV(src->getTextureId(), 11, 23.f);
			{
				drawRect(0, 0, src->getWidth(), src->getHeight());
			}
			clearShader();
			popBlend();
		}
		popRenderPass();
		
		pushRenderPass(composite[composite_idx], false, nullptr, false, "Bloom apply");
		{
		// todo : add bloom apply shader
		
			pushBlend(BLEND_ADD_OPAQUE);
			gxSetTexture(buffers.bloomBuffer->getTextureId());
			{
				setLumif(renderOptions.bloom.strength);
				drawRect(0, 0, buffers.bloomBuffer->getWidth(), buffers.bloomBuffer->getHeight());
			}
			gxSetTexture(0);
			popBlend();
		}
		popRenderPass();
	}
#endif

#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
	if (renderOptions.bloom.enabled)
	{
		pushBlend(BLEND_OPAQUE);
		{
			Shader shader("renderOne/bloom-downsample");
			setShader(shader);
			{
				ColorTarget * src = composite[composite_idx];
				
				for (int i = 0; i < buffers.bloomChain.numBuffers; ++i)
				{
					ColorTarget * dst = &buffers.bloomChain.buffers[i];
				
					pushRenderPass(dst, true, nullptr, false, "Bloom chain");
					{
						projectScreen2d();
						
						shader.setTexture("source", 0, src->getTextureId(), true, true);
						
						drawRect(0, 0, dst->getWidth(), dst->getHeight());
					}
					popRenderPass();

					src = dst;
				}
			}
			clearShader();
		}
		popBlend();
		
		// todo : apply bloom
	}
#endif

	if (renderOptions.lightScatter.enabled)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Light scatter");
		{
			projectScreen2d();
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/lightScatter");
				setShader(shader);
				{
					const float decayPerSample = powf(renderOptions.lightScatter.decay, 1.f / renderOptions.lightScatter.numSamples);
					
					shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("lightTexture", 1, composite[composite_idx]->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setImmediate("origin", renderOptions.lightScatter.origin[0], renderOptions.lightScatter.origin[1]);
					shader.setImmediate("numSamples", renderOptions.lightScatter.numSamples);
					shader.setImmediate("decayPerSample", decayPerSample);
					shader.setImmediate("strength", renderOptions.lightScatter.strength);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}
	
	// apply tone mapping
	
	if (renderOptions.linearColorSpace)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Tone mapping");
		{
			projectScreen2d();
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/tonemap");
				setShader(shader);
				{
					shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setImmediate("exposure", renderOptions.toneMapping.exposure);
					shader.setImmediate("gamma", renderOptions.toneMapping.gamma);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}
	
	// apply simple screen space refraction
	
	if (renderOptions.simpleScreenSpaceRefraction.enabled)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Refraction (simple)");
		{
			projectScreen2d();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/simple-screen-space-refraction");
				setShader(shader);
				{
					shader.setTexture("normalTexture", 0, getTexture("textures/refraction/droplets.png"), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("colorTexture", 1, composite[composite_idx]->getTextureId(), true, true);
					shader.setImmediate("strength", renderOptions.simpleScreenSpaceRefraction.strength);
				// todo : set uniform to set uv scaling and offset to fit refraction texture to view
					shader.setImmediate("time", framework.time); // todo : remove
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}
	
	// overlay an outline to the rendered image
	
	if (renderOptions.depthSilhouette.enabled)
	{
		pushRenderPass(composite[composite_idx], false, nullptr, false, "Outline");
		{
			projectScreen2d();
			
			pushBlend(BLEND_ALPHA);
			{
				const Mat4x4 projectionToView = projectionMatrix.CalcInv();
				
				Shader shader("renderOne/outline");
				setShader(shader);
				{
					shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, true);
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
					shader.setImmediate("strength", renderOptions.depthSilhouette.strength);
					shader.setImmediate("color",
						renderOptions.depthSilhouette.color[0],
						renderOptions.depthSilhouette.color[1],
						renderOptions.depthSilhouette.color[2],
						renderOptions.depthSilhouette.color[3]);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
	}
	
	if (renderOptions.colorGrading.enabled)
	{
	// todo : create a color grading designer UI, with export option ?
	// todo : write color grading identity LUT to data folder, at startup (when it doesn't exist yet)
	// todo : add a Help Text about color grading
	// todo : add the ability to load/save color grading LUT textures
	// todo : remember last color grading LUT and automatically use it at startup
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Color grading");
		{
			projectScreen2d();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/color-grade");
				setShader(shader);
				{
				#if 0
					const GxTextureId lutTexture = getTexture("color-grading/LUT_Greenish.jpg");
					//const GxTextureId lutTexture = getTexture("color-grading/LUT_Reddish.jpg");
					const bool lutIsDynamic = false;
				#else
					uint8_t lut[16][256][4];
					
					for (int x = 0; x < 256; ++x)
					{
						for (int y = 0; y < 16; ++y)
						{
							const float r = (x % 16) / 15.f;
							const float g = y / 15.f;
							const float b = (x / 16) / 15.f;
							
							Color color(r, g, b);
							color = color.hueShift(framework.time / 10.f);
							
							lut[y][x][0] = color.r * 255.f;
							lut[y][x][1] = color.g * 255.f;
							lut[y][x][2] = color.b * 255.f;
							lut[y][x][3] = 255;
						}
					}
					
					GxTextureId lutTexture = createTextureFromRGBA8(lut, 256, 16, true, false);
					const bool lutIsDynamic = true;
				#endif
				
					shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
					shader.setTexture("lutTexture", 1, lutTexture, true, true);
					drawRect(0, 0, viewportSx, viewportSy);
					
					if (lutIsDynamic)
						freeTexture(lutTexture);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}

	if (renderOptions.fxaa.enabled)
	{
		const int next_composite_idx = 1 - composite_idx;
		
		pushRenderPass(composite[next_composite_idx], true, nullptr, false, "FXAA");
		{
			projectScreen2d();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/fxaa");
				setShader(shader);
				{
					shader.setTexture("source", 0, composite[composite_idx]->getTextureId(), false, true);
					shader.setImmediate("inverseVP",
						1.f / viewportSx,
						1.f / viewportSy);
					drawRect(0, 0, viewportSx, viewportSy);
				}
				clearShader();
			}
			popBlend();
		}
		popRenderPass();
		
		composite_idx = next_composite_idx;
	}
	
	return composite[composite_idx];
}

RenderBuffers::~RenderBuffers()
{
	free();
}

void RenderBuffers::alloc(const int in_sx, const int in_sy, const bool in_linearColorSpace)
{
	if (in_sx != sx || in_sy != sy || in_linearColorSpace != linearColorSpace)
	{
		free();
		
		//
		
		sx = in_sx;
		sy = in_sy;
		linearColorSpace = in_linearColorSpace;
		
		//
		
		colors = new ColorTarget();
		normals = new ColorTarget();
		specularColor = new ColorTarget();
		specularExponent = new ColorTarget();
		emissive = new ColorTarget();
		depth = new DepthTarget();
		light = new ColorTarget();
		composite1 = new ColorTarget();
		composite2 = new ColorTarget();
		velocity = new ColorTarget();
		
	#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
		bloomBuffer = new ColorTarget();
	#endif

		//
		
		colors->init(sx, sy, linearColorSpace ? SURFACE_RGBA8_SRGB : SURFACE_RGBA8, colorBlackTranslucent);
		normals->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent); // todo : on store, encode normals into RG channels, and change format to RG16F
		specularColor->init(sx, sy, linearColorSpace ? SURFACE_RGBA8_SRGB : SURFACE_RGBA8, colorBlackTranslucent);
		specularExponent->init(sx, sy, SURFACE_R16F, colorBlackTranslucent);
		emissive->init(sx, sy, SURFACE_R8, colorBlackTranslucent);
// todo : OpenGL function to set depth target always requires enableTexture to be true, or else it will assert. add a low-level depth buffer access function so we can bind it to the frame buffer object
		// note : we will need the texture for lighting, so we enable the depth texture here
		depth->init(sx, sy, DEPTH_FLOAT32, true, 1.f);
		light->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
		composite1->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
		composite2->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
		velocity->init(sx, sy, SURFACE_RG16F, colorBlackTranslucent);
		
	#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
		bloomBuffer->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
	#endif
	
	#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
		bloomChain.free();
		bloomChain.alloc(sx, sy);
	#endif
	}
}

void RenderBuffers::free()
{
	delete colors;
	colors = nullptr;
	
	delete normals;
	normals = nullptr;
	
	delete specularColor;
	specularColor = nullptr;
	
	delete specularExponent;
	specularExponent = nullptr;
	
	delete emissive;
	emissive = nullptr;
	
	delete depth;
	depth = nullptr;
	
	delete light;
	light = nullptr;
	
	delete composite1;
	composite1 = nullptr;
	
	delete composite2;
	composite2 = nullptr;
	
	delete velocity;
	velocity = nullptr;
	
#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
	delete bloomBuffer;
	bloomBuffer = nullptr;
#endif

#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
	bloomChain.free();
#endif

	//
	
	sx = 0;
	sy = 0;
	linearColorSpace = false;
}

#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN

static int getNearestPowerOfTwo(const int x)
{
	int result = 1;
	
	while (result << 1 < x)
		result <<= 1;
	
	return result;
}

void RenderBuffers::BloomChain::alloc(const int sx, const int sy)
{
	Assert(numBuffers == 0);
	
	int tsx = getNearestPowerOfTwo(sx) << 1;
	int tsy = getNearestPowerOfTwo(sy) << 1;
	
	do
	{
		tsx = tsx/2 > 2 ? tsx/2 : 2;
		tsy = tsy/2 > 2 ? tsy/2 : 2;
		
		logDebug("bloom buffer: %dx%dpx", tsx, tsy);
		
		numBuffers++;
	}
	while (tsx > 2 || tsy > 2);
	
	logDebug("num bloom buffers: %d", numBuffers);
	
	buffers = new ColorTarget[numBuffers];
	
	//
	
	tsx = getNearestPowerOfTwo(sx) << 1;
	tsy = getNearestPowerOfTwo(sy) << 1;
	
	int i = 0;
	
	while (tsx > 2 || tsy > 2)
	{
		tsx = tsx/2 > 2 ? tsx/2 : 2;
		tsy = tsy/2 > 2 ? tsy/2 : 2;
		
		buffers[i].init(tsx, tsy, SURFACE_RGBA16F, colorBlackTranslucent);
		
		i++;
	}
}

void RenderBuffers::BloomChain::free()
{
	delete [] buffers;
	buffers = nullptr;
	
	numBuffers = 0;
}

#endif

static ColorTarget * renderFromEye(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions, const Vec3 & eyeOffset, RenderBuffers & buffers, const int viewportSx, const int viewportSy, const float timeStep)
{
	gxPushMatrix();
	Mat4x4 viewMatrix;
	gxGetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
	viewMatrix =
		Mat4x4(true)
		.Translate(eyeOffset[0], eyeOffset[1], eyeOffset[2])
		.Mul(viewMatrix);
	gxLoadMatrixf(viewMatrix.m_v);
	
	ColorTarget * result = nullptr;
	bool hasResult = false;
	
	switch (renderOptions.renderMode)
	{
	case kRenderMode_Flat:
		result = renderModeFlat(renderFunctions, renderOptions);
		hasResult = true;
		break;
		
	case kRenderMode_DeferredShaded:
		result = renderModeDeferredShaded(renderFunctions, renderOptions, buffers, viewportSx, viewportSy, timeStep);
		hasResult = true;
		break;
	}

	gxPopMatrix();
	
	Assert(hasResult);
	return result;
}

static bool isDirectToFramebufferMode(const RenderMode renderMode)
{
	switch (renderMode)
	{
	case kRenderMode_Flat:
		return true;
		
	case kRenderMode_DeferredShaded:
		return false;
	}

	Assert(false);
	return false;
}

void Renderer::render(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions, const int viewportSx, const int viewportSy, const float timeStep)
{
	buffers.alloc(viewportSx, viewportSy, renderOptions.linearColorSpace);
	
	if (renderOptions.anaglyphic.enabled)
		buffers2.alloc(viewportSx, viewportSy, renderOptions.linearColorSpace);
	else
		buffers2.free();
	
	//
	
	// note : the 'colors' buffer stores contents in sRGB color space, so no need to linearize the background color here
	
	for (auto * buffer : { &buffers, &buffers2 })
	{
		if (buffer->colors != nullptr)
		{
			buffer->colors->setClearColor(
				renderOptions.backgroundColor[0],
				renderOptions.backgroundColor[1],
				renderOptions.backgroundColor[2], 0.f);
		}
	}
	
	//
	
	if (renderOptions.anaglyphic.enabled)
	{
		const bool needsRenderPass = isDirectToFramebufferMode(renderOptions.renderMode);

		ColorTarget * eyeL = nullptr;
		ColorTarget * eyeR = nullptr;
		
		if (needsRenderPass)
		{
			Mat4x4 modelViewMatrix;
			Mat4x4 projectionMatrix;
			gxGetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			
			pushRenderPass(buffers.colors, true, buffers.depth, true, "Eye: Left");
			{
				gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
				gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
		
			#if ENABLE_OPENGL
				gxMatrixMode(GX_PROJECTION);
				gxScalef(1, -1, 1); // todo : remove the need to scale here
				gxMatrixMode(GX_MODELVIEW);
			#endif
			
				renderFromEye(
					renderFunctions,
					renderOptions,
					Vec3(-renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
					buffers,
					viewportSx,
					viewportSy,
					timeStep);
				eyeL = buffers.colors;
			}
			popRenderPass();

			pushRenderPass(buffers2.colors, true, buffers2.depth, true, "Eye: Right");
			{
				gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
				gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
				
			#if ENABLE_OPENGL
				gxMatrixMode(GX_PROJECTION);
				gxScalef(1, -1, 1); // todo : remove the need to scale here
				gxMatrixMode(GX_MODELVIEW);
			#endif
			
				renderFromEye(
					renderFunctions,
					renderOptions,
					Vec3(+renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
					buffers2,
					viewportSx,
					viewportSy,
					timeStep);
				eyeR = buffers2.colors;
			}
			popRenderPass();
		}
		else
		{
			eyeL = renderFromEye(
				renderFunctions,
				renderOptions,
				Vec3(-renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
				buffers,
				viewportSx,
				viewportSy,
				timeStep);
			eyeR = renderFromEye(
				renderFunctions,
				renderOptions,
				Vec3(+renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
				buffers2,
				viewportSx,
				viewportSy,
				timeStep);
		}

		//
		
		projectScreen2d();
		
		pushBlend(BLEND_OPAQUE);
		{
			Shader shader("renderOne/anaglyphic-compose");
			setShader(shader);
			{
				shader.setImmediate("mode", 0);
				shader.setTexture("colormapL", 0, eyeL->getTextureId(), false, false);// note : clamp is intentionally turned off, to expose incorrect sampling
				shader.setTexture("colormapR", 1, eyeR->getTextureId(), false, false);// note : clamp is intentionally turned off, to expose incorrect sampling
				
				drawRect(0, 0, eyeL->getWidth(), eyeL->getHeight());
			}
			clearShader();
		}
		popBlend();
	}
	else
	{
		ColorTarget * result = renderFromEye(
			renderFunctions,
			renderOptions,
			Vec3(),
			buffers,
			viewportSx,
			viewportSy,
			timeStep);

		if (result != nullptr)
		{
			// blit the result to the frame buffer
		
			projectScreen2d();

			pushBlend(BLEND_OPAQUE);
			{
				gxSetTexture(result->getTextureId());
				setColor(colorWhite);
				drawRect(0, 0, result->getWidth(), result->getHeight());
				gxSetTexture(0);
			}
			popBlend();
			
			if (renderOptions.debugRenderTargets)
			{
				gxPushMatrix();
				{
					const int sx = 128;
					const int sy = sx * result->getHeight() / result->getWidth();
					setColor(colorWhite);
					
					int x = 0;
					int y = 0;
					
					auto nextRow = [&]()
					{
						x = 0;
						y += sy;
					};
					
					auto showRenderTarget = [&](const GxTextureId textureId, const char * name)
					{
						if (x + sx > result->getWidth())
						{
							nextRow();
						}
						
						pushBlend(BLEND_OPAQUE);
						{
							gxSetTexture(textureId);
							gxSetTextureSampler(GX_SAMPLE_NEAREST, true);
							setColor(colorWhite);
							drawRect(x, y, x + sx, y + sy);
							gxSetTextureSampler(GX_SAMPLE_LINEAR, false);
							gxSetTexture(0);
						}
						popBlend();
						
						setAlphaf(.5f);
						drawRectLine(x, y, x + sx, y + sy);
						
						setFont("calibri.ttf");
						drawText(x + 4, y + 4, 12, +1, +1, "%s", name);
						
						int textureSx = 0;;
						int textureSy = 0;
						gxGetTextureSize(textureId, textureSx, textureSy);
						drawText(x + 4, y + 20, 12, +1, +1, "%d x %d", textureSx, textureSy);
						
						x += sx;
					};
					
					showRenderTarget(buffers.colors->getTextureId(), "Colors");
					showRenderTarget(buffers.normals->getTextureId(), "Normals");
					showRenderTarget(buffers.specularColor->getTextureId(), "Spec Color");
					showRenderTarget(buffers.specularExponent->getTextureId(), "Spec Exp");
					showRenderTarget(buffers.emissive->getTextureId(), "Emissive");
					showRenderTarget(buffers.depth->getTextureId(), "Depth");
					showRenderTarget(buffers.light->getTextureId(), "Light");
					showRenderTarget(buffers.velocity->getTextureId(), "Velocity");
					
				#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
					if (renderOptions.bloom.enabled)
					{
						nextRow();

						for (int i = 0; i < buffers.bloomChain.numBuffers; ++i)
						{
							showRenderTarget(buffers.bloomChain.buffers[i].getTextureId(), "Bloom down");
						}
					}
				#endif
				
					gxSetTexture(0);
				}
				gxPopMatrix();
			}
		}
	}
}

void Renderer::render(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions, ColorTarget * colorTarget, DepthTarget * depthTarget, const float timeStep)
{
	pushRenderPass(colorTarget, true, depthTarget, true, "Render");
	{
		render(renderFunctions, renderOptions, colorTarget->getWidth(), colorTarget->getHeight(), timeStep);
	}
	popRenderPass();
}

void Renderer::render(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions, const float timeStep)
{
	int viewportSx;
	int viewportSy;
	framework.getCurrentViewportSize(viewportSx, viewportSy);
	
	render(renderFunctions, renderOptions, viewportSx, viewportSy, timeStep);
}

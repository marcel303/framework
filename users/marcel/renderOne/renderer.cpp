#include "framework.h"
#include "gx_render.h"
#include "image.h" // for ColorGrading lookup texture from file
#include "lightDrawer.h"
#include "renderer.h"
#include "srgbFunctions.h"

#define ENABLE_FULLSCREEN_QUAD_OPTIMIZE 1

#if 0
	// todo : add screenshot functionality

	#include "image.h"
	#include <SDL2/SDL_opengl.h>

	void screenshot()
	{
		Surface * surface = new Surface(8000, 6000, false);
		
		pushSurface(surface);
		{
			surface->clearf(0.f, 0.f, 0.f, 1.f);
			pushLineSmooth(true);
			// -> draw();
			popLineSmooth();
			
			// fetch the pixel data
	
			ImageData imageData;
			imageData.sx = surface->getWidth();
			imageData.sy = surface->getHeight();
			imageData.imageData = new ImageData::Pixel[imageData.sx * imageData.sy];
			
			glReadPixels(
				0, 0,
				surface->getWidth(),
				surface->getHeight(),
				GL_RGBA, GL_UNSIGNED_BYTE,
				imageData.imageData);
			checkErrorGL();
			
			saveImage(&imageData, "test.png");
		}
		popSurface();
		
		delete surface;
		surface = nullptr;
	}
#endif

namespace rOne
{
	static void drawFullscreenQuad(const int viewSx, const int viewSy)
	{
	#if ENABLE_FULLSCREEN_QUAD_OPTIMIZE
		// draw a single large triangle covering the view. this is more efficient
		// than drawing a quad, which gets rasterized as two triangles. when
		// drawing two triangles, there will be a seam diagonally across the view,
		// which makes it more difficult for the gpu to schedule wave fronts
		// efficiently. a large triangle doesn't have this issue
		// see: https://michaldrobot.com/2014/04/01/gcn-execution-patterns-in-full-screen-passes/
		const int size = (viewSx > viewSy ? viewSx : viewSy) * 2;
		
		gxBegin(GX_TRIANGLES);
		gxTexCoord2f(0, 0);
		gxVertex2f(0, 0);
		gxTexCoord2f(size / float(viewSx), 0);
		gxVertex2f(size, 0);
		gxTexCoord2f(0, size / float(viewSy));
		gxVertex2f(0, size);
		gxEnd();
	#else
		drawRect(0, 0, viewSx, viewSy);
	#endif
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
	
	static void renderOpaque_ForwardShadedPass(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
	{
		pushDepthTest(true, DEPTH_LESS, true);
		pushBlend(BLEND_OPAQUE);
		{
			if (renderOptions.enableOpaquePass && renderFunctions.drawOpaque_ForwardShaded != nullptr)
			{
				renderFunctions.drawOpaque_ForwardShaded();
			}
		}
		popBlend();
		popDepthTest();
	}
	
	static void renderBackgroundPass(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
	{
		// create a stencil mask for pixels in the background (depth = 1.0)
		
		clearStencil(0x00, 0x01);
		
		pushDepthTest(true, DEPTH_EQUAL, false);
		pushColorWriteMask(0, 0, 0, 0);
		{
			StencilSetter()
				.op(GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_REPLACE)
				.comparison(GX_STENCIL_FUNC_ALWAYS, 0x01, 0x01)
				.writeMask(0x01);
			
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxLoadIdentity();
			gxMatrixMode(GX_MODELVIEW);
			gxPushMatrix();
			gxLoadIdentity();
			{
				gxBegin(GX_QUADS);
				gxVertex3f(-1, -1, +1);
				gxVertex3f(+1, -1, +1);
				gxVertex3f(+1, +1, +1);
				gxVertex3f(-1, +1, +1);
				gxEnd();
			}
			gxMatrixMode(GX_PROJECTION);
			gxPopMatrix();
			gxMatrixMode(GX_MODELVIEW);
			gxPopMatrix();
		}
		popColorWriteMask();
		popDepthTest();
		
		// set up the stencil test to draw only onto the masked areas
		
		StencilSetter()
			.comparison(GX_STENCIL_FUNC_EQUAL, 0x01, 0x01)
			.writeMask(0x00);

	#if 0
		// stencil-test test
		
		pushDepthTest(true, DEPTH_LESS);
		beginCubeBatch();
		{
			for (int i = 0; i < 200; ++i)
			{
				const float r = (cosf(i + framework.time / 2.f) * 40.f + 1.f) / 2.f;
				
				const Vec3 p(
					cosf(i / 1.23f) * r,
					cosf(i / 2.34f) * 4.f,
					cosf(i / 3.45f) * r);
				
				setColor(colorWhite);
				fillCube(p, Vec3(.1, .1, .1));
			}
		}
		endCubeBatch();
		popDepthTest();
	#endif
		
		pushDepthTest(true, DEPTH_LESS);
		pushBlend(BLEND_ALPHA);
		{
			if (renderOptions.enableBackgroundPass && renderFunctions.drawBackground != nullptr)
			{
				renderFunctions.drawBackground();
			}
		}
		popBlend();
		popDepthTest();
		
		clearStencilTest();
	}

	static void renderTranslucentPass(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
	{
		pushDepthTest(true, DEPTH_LESS, false);
		pushBlend(BLEND_ALPHA);
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
				if (renderOptions.deferredLighting.useDefaultLight)
				{
					g_lightDrawer.drawDeferredDirectionalLight(
						renderOptions.deferredLighting.defaultLightDirection,
						renderOptions.deferredLighting.defaultLightColorTop,
						renderOptions.deferredLighting.defaultLightColorBottom,
						1.f);
				}
				
			#if 0 // note : leave this set to '0' when checking in!
				{
					const Vec3 lightColor = Vec3(1, 1, 1);
				
					g_lightDrawer.drawDeferredAmbientLight(lightColor);
				}
			#endif
			
			#if 0 // note : leave this set to '0' when checking in!
				{
					const Mat4x4 rotationMatrix = Mat4x4(true).RotateY(framework.time / 2.f).RotateX(sinf(framework.time)/2.f);
					const Vec3 lightDirection = rotationMatrix.Mul3(Vec3(.3f, -3.f, .6f)).CalcNormalized();
					const Vec3 lightColor1 = Vec3(1.f, .8f, .6f); // light color when the light is coming from 'above'
					const Vec3 lightColor2 = Vec3(.1f, .2f, .3f); // light color when the light is coming from 'below'
					
					g_lightDrawer.drawDeferredDirectionalLight(lightDirection, lightColor1, lightColor2);
				}
			#endif
			
			#if 0 // note : leave this set to '0' when checking in!
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
			
				drawFullscreenQuad(sx, sy);
			}
			clearShader();
		}
		popBlend();
	}

	static void renderReconstructedVelocityBuffer(
		const RenderOptions & renderOptions,
		const RenderBuffers & buffers,
		const int viewportSx,
		const int viewportSy,
		const RenderEyeData & eyeData)
	{
		const bool needsVelocityBuffer = renderOptions.motionBlur.enabled;
		
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
						shader.setImmediateMatrix4x4("projectionToWorld_curr", eyeData.projectionToWorld_curr.m_v);
						shader.setImmediateMatrix4x4("projectionToWorld_prev", eyeData.projectionToWorld_prev.m_v);
						shader.setImmediateMatrix4x4("worldToProjection", eyeData.projectionToWorld_curr.CalcInv().m_v);
						shader.setImmediate("timeStepRcp", eyeData.timeStep > 0.f
							? 1.f / eyeData.timeStep
							: 1.f);
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
		}
	}

	static void renderPostOpaqueEffects(
		const RenderOptions & renderOptions,
		const RenderBuffers & buffers,
		const int viewportSx,
		const int viewportSy,
		const Mat4x4 & projectionMatrix,
		ColorTarget * composite[2],
		int & composite_idx)
	{
		// apply screen-space ambient occlusion
		
		if (renderOptions.screenSpaceAmbientOcclusion.enabled)
		{
			pushRenderPass(composite[composite_idx], false, nullptr, false, "SSAO");
			{
				projectScreen2d();
				
				pushBlend(BLEND_MUL);
				{
					const Mat4x4 projectionToView = projectionMatrix.CalcInv();
					
					Shader shader("renderOne/postprocess/ssao");
					setShader(shader);
					{
						shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, true);
						shader.setTexture("normalTexture", 1, buffers.normals->getTextureId(), false, true);
						shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
						shader.setImmediate("strength", renderOptions.screenSpaceAmbientOcclusion.strength);
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
		}
		
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
					
					Shader shader("renderOne/postprocess/fog-application");
					setShader(shader);
					{
						shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
						shader.setImmediate("fogColor",
							fogColor[0],
							fogColor[1],
							fogColor[2]);
						shader.setImmediate("fogTranslucency", 1.f - renderOptions.fog.thickness);
						drawFullscreenQuad(viewportSx, viewportSy);
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
					
					Shader shader("renderOne/postprocess/screen-space-reflection");
					setShader(shader);
					{
						shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture("normalTexture", 1, buffers.normals->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture("colorTexture", 2, composite[composite_idx]->getTextureId(), true, false); // note : clamp is intentionally turned off, to expose incorrect fading
						shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
						shader.setImmediateMatrix4x4("viewToProjection", viewToProjection.m_v);
						drawFullscreenQuad(viewportSx, viewportSy);
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
					const Vec3 color_linear = srgbToLinear(
						Vec3(
							renderOptions.depthSilhouette.color[0],
							renderOptions.depthSilhouette.color[1],
							renderOptions.depthSilhouette.color[2]));
					
					Shader shader("renderOne/postprocess/outline");
					setShader(shader);
					{
						shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, true);
						shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
						shader.setImmediate("strength", renderOptions.depthSilhouette.strength);
						shader.setImmediate("color",
							color_linear[0],
							color_linear[1],
							color_linear[2],
							renderOptions.depthSilhouette.color[3]);
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
		}
	}

	static void renderPostEffects(
		const RenderOptions & renderOptions,
		const RenderBuffers & buffers,
		const int viewportSx,
		const int viewportSy,
		const Mat4x4 & projectionMatrix,
		ColorTarget * composite[2],
		int & composite_idx)
	{
		// apply screen-space motion blur
		
		if (renderOptions.motionBlur.enabled)
		{
			const int next_composite_idx = 1 - composite_idx;
			
			pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Motion blur");
			{
				projectScreen2d();
				
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("renderOne/postprocess/screen-space-motion-blur");
					setShader(shader);
					{
						shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), true, true);
						shader.setTexture("velocityTexture", 1, buffers.velocity->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setImmediate("strength", renderOptions.motionBlur.strength);
						shader.setImmediate("viewportSizeRcp", 1.f / viewportSx, 1.f / viewportSy);
						drawFullscreenQuad(viewportSx, viewportSy);
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

					Shader shader("renderOne/postprocess/scatter-dof");
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
					
					Shader shader("renderOne/postprocess/simple-depth-of-field");
					setShader(shader);
					{
						shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), true, true);
						shader.setTexture("depthTexture", 1, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
						shader.setImmediate("strength", renderOptions.depthOfField.strength);
						shader.setImmediate("focusDistance", renderOptions.depthOfField.focusDistance);
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
			
			composite_idx = next_composite_idx;
		#endif
		}
		
		if (renderOptions.chromaticAberration.enabled)
		{
			const int next_composite_idx = 1 - composite_idx;
			
			pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Chromatic aberration");
			{
				projectScreen2d();
				
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("renderOne/postprocess/chromatic-aberration");
					setShader(shader);
					{
						shader.setTexture("source", 0, composite[composite_idx]->getTextureId(), true, true);
						shader.setImmediate("strength", renderOptions.chromaticAberration.strength / 20.f); // 100% = 5% image distortion
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
			
			composite_idx = next_composite_idx;
		}
		
		// apply bloom

	#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
		if (renderOptions.bloom.enabled)
		{
			const int next_composite_idx = 1 - composite_idx;
			
			ColorTarget * src = composite[composite_idx];
			
			// blur the bloom buffer
			
			pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Bloom blur H");
			{
				pushBlend(BLEND_OPAQUE);
				setShader_GaussianBlurH(src->getTextureId(), 11, renderOptions.bloom.blurSize);
				{
					drawFullscreenQuad(src->getWidth(), src->getHeight());
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
					drawFullscreenQuad(src->getWidth(), src->getHeight());
				}
				clearShader();
				popBlend();
			}
			popRenderPass();
			
			// apply bloom
			
			pushRenderPass(composite[composite_idx], false, nullptr, false, "Bloom apply");
			{
				pushBlend(BLEND_ADD_OPAQUE);
				Shader shader("renderOne/bloom-apply");
				setShader(shader);
				{
					shader.setTexture("source", 0, buffers.bloomBuffer->getTextureId(), true, true);
					shader.setImmediate("strength", renderOptions.bloom.strength);
					
					drawFullscreenQuad(viewportSx, viewportSy);
				}
				clearShader();
				popBlend();
			}
			popRenderPass();
		}
	#endif

	#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
		if (renderOptions.bloom.enabled)
		{
			int next_composite_idx = 1 - composite_idx;
			
			ColorTarget * src = composite[composite_idx];
			ColorTarget * dst = composite[next_composite_idx];
			
			// mask colors based on brightness
			
			pushRenderPass(dst, true, nullptr, false, "Bloom bright pass");
			{
				pushBlend(BLEND_OPAQUE);
				Shader shader("renderOne/bloom-bright-pass");
				setShader(shader);
				{
					shader.setTexture("source", 0, src->getTextureId(), false, true);
					shader.setImmediate("brightPassValue", renderOptions.bloom.brightPassValue);
					drawFullscreenQuad(src->getWidth(), src->getHeight());
				}
				clearShader();
				popBlend();
			}
			popRenderPass();
			
			src = dst;
		
			// create down sampled buffers
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("renderOne/bloom-downsample");
				setShader(shader);
				{
					for (int i = 0; i < buffers.bloomDownsampleChain.numBuffers; ++i)
					{
						dst = &buffers.bloomDownsampleChain.buffers[i];
						
						pushRenderPass(dst, true, nullptr, false, "Bloom chain");
						{
							shader.setTexture("source", 0, src->getTextureId(), true, true);
							
							drawFullscreenQuad(dst->getWidth(), dst->getHeight());
						}
						popRenderPass();

						src = dst;
					}
				}
				clearShader();
			}
			popBlend();
			
			// add/upscale lower level buffer and blur
			
			const int finalBuffer =
				renderOptions.bloom.dropTopLevelImage
				? (buffers.bloomDownsampleChain.numBuffers >= 2 ? 1 : 0)
				: 0;
			
			for (int i = buffers.bloomDownsampleChain.numBuffers - 1; i >= finalBuffer; --i)
			{
				if (i + 1 < buffers.bloomDownsampleChain.numBuffers)
				{
					ColorTarget * src = &buffers.bloomBlurChain.buffers[i + 1];
					ColorTarget * dst = &buffers.bloomDownsampleChain.buffers[i];
				
					// add the lower resolution buffer to the current buffer
					
					pushRenderPass(dst, false, nullptr, false, "Bloom chain add");
					pushBlend(BLEND_ADD_OPAQUE);
					{
						Shader shader("renderOne/blit-texture");
						setShader(shader);
						{
							shader.setTexture("source", 0, src->getTextureId(), true, true);

							drawFullscreenQuad(dst->getWidth(), dst->getHeight());
						}
						clearShader();
					}
					popBlend();
					popRenderPass();
				}
				
				// blur the buffer
				
				ColorTarget * src = &buffers.bloomDownsampleChain.buffers[i];
				ColorTarget * dst = &buffers.bloomBlurChain.buffers[i];
				
				pushRenderPass(dst, true, nullptr, false, "Bloom chain blur");
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("renderOne/bloom-blur");
					setShader(shader);
					{
						shader.setTexture("source", 0, src->getTextureId(), false, true);
						
						drawFullscreenQuad(dst->getWidth(), dst->getHeight());
					}
					clearShader();
				}
				popBlend();
				popRenderPass();
			}
			
			// apply bloom
			
			pushRenderPass(composite[composite_idx], false, nullptr, false, "Bloom apply");
			{
				pushBlend(BLEND_ADD_OPAQUE);
				Shader shader("renderOne/bloom-apply");
				setShader(shader);
				{
					shader.setTexture("source", 0, buffers.bloomBlurChain.buffers[finalBuffer].getTextureId(), true, true);
					shader.setImmediate("strength", renderOptions.bloom.strength / (buffers.bloomBlurChain.numBuffers - finalBuffer));
					
					drawFullscreenQuad(viewportSx, viewportSy);
				}
				clearShader();
				popBlend();
			}
			popRenderPass();
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
					Shader shader("renderOne/postprocess/light-scatter");
					setShader(shader);
					{
						shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), true, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture("lightTexture", 1, composite[composite_idx]->getTextureId(), true, true);
						shader.setImmediate("origin", renderOptions.lightScatter.origin[0], renderOptions.lightScatter.origin[1]);
						shader.setImmediate("numSamples", renderOptions.lightScatter.numSamples);
						shader.setImmediate("decay", renderOptions.lightScatter.decay);
						shader.setImmediate("strength", renderOptions.lightScatter.strength * renderOptions.lightScatter.strengthMultiplier);
						drawFullscreenQuad(viewportSx, viewportSy);
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
					Shader shader("renderOne/postprocess/tonemap");
					setShader(shader);
					{
						shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setImmediate("tonemap", renderOptions.toneMapping.toneMap);
						shader.setImmediate("exposure", renderOptions.toneMapping.exposure);
						shader.setImmediate("gamma", renderOptions.toneMapping.gamma);
						drawFullscreenQuad(viewportSx, viewportSy);
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
					Shader shader("renderOne/postprocess/simple-screen-space-refraction");
					setShader(shader);
					{
						shader.setTexture("normalTexture", 0, getTexture("textures/refraction/droplets.png"), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture("colorTexture", 1, composite[composite_idx]->getTextureId(), true, true);
						shader.setImmediate("strength", renderOptions.simpleScreenSpaceRefraction.strength);
					// todo : set uniform to set uv scaling and offset to fit refraction texture to view
						shader.setImmediate("time", framework.time); // todo : remove
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
			
			composite_idx = next_composite_idx;
		}
		
		if (renderOptions.colorGrading.enabled && renderOptions.colorGrading.lookupTextureId != 0)
		{
			const int next_composite_idx = 1 - composite_idx;
			
			pushRenderPass(composite[next_composite_idx], true, nullptr, false, "Color grading");
			{
				projectScreen2d();
				
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("renderOne/postprocess/color-grade");
					setShader(shader);
					{
						shader.setTexture("colorTexture", 0, composite[composite_idx]->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture3d("lutTexture", 1, renderOptions.colorGrading.lookupTextureId, true, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						drawFullscreenQuad(viewportSx, viewportSy);
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
					Shader shader("renderOne/postprocess/fxaa");
					setShader(shader);
					{
						shader.setTexture("source", 0, composite[composite_idx]->getTextureId(), false, true);
						shader.setImmediate("inverseVP",
							1.f / viewportSx,
							1.f / viewportSy);
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
			
			composite_idx = next_composite_idx;
		}
	}

	// render modes

	static ColorTarget * renderModeFlat(const RenderFunctions & renderFunctions, const RenderOptions & renderOptions)
	{
		updateCullFlip();
		
		pushShaderOutputs(renderOptions.drawNormals ? "n" : "c");
		{
			renderOpaquePass(
				renderFunctions,
				renderOptions);
			
			renderOpaque_ForwardShadedPass(
				renderFunctions,
				renderOptions);
		}
		popShaderOutputs();
		
		renderBackgroundPass(
			renderFunctions,
			renderOptions);
		
		renderTranslucentPass(
			renderFunctions,
			renderOptions);

		return nullptr;
	}

	static ColorTarget * renderModeDeferredShaded(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		RenderBuffers & buffers,
		const int viewportSx,
		const int viewportSy,
		const RenderEyeData & eyeData)
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
		pushShaderOutputs(
			renderOptions.drawNormals
				? "nnSse"
				: "ncSse");
		{
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			updateCullFlip();
			
		#if ENABLE_OPENGL
			gxMatrixMode(GX_PROJECTION);
			gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
			gxMatrixMode(GX_MODELVIEW);
			pushCullFlip();
		#endif
			
			renderOpaquePass(
				renderFunctions,
				renderOptions);
			
		#if ENABLE_OPENGL
			popCullFlip();
		#endif
		}
		popShaderOutputs();
		popRenderPass();

		// accumulate lights
		
		pushRenderPass(
			buffers.light, true,
			renderOptions.deferredLighting.enableStencilVolumes
			? buffers.depth
			: nullptr, false,
			"Light");
		{
			// note : these are the matrices used when drawing
			//        the 2D quads for the lights. we use identity
			//        matrices, so we can directly use clip-space
			//        coordinates when drawing those quads from
			//        (-1, -1) to (+1, +1)
			//        in a previous version, it would use the 2D
			//        screen projection matrix (projectScreen2d())
			//        which has the down side that it may or may
			//        not flip the Y axis. we want the stenciled
			//        pixels (used to cull pixels not intersecting
			//        the light at all) to correctly match up with
			//        sampling the geometry buffer textures (normal,
			//        depth). the optional Y axis inversion makes
			//        that process a lot more tedious. using clip-
			//        space coordinates keeps things simple
			gxMatrixMode(GX_PROJECTION);
			gxLoadIdentity();
			gxMatrixMode(GX_MODELVIEW);
			gxLoadIdentity();
		#if !ENABLE_OPENGL
			gxScalef(1, -1, 1);
		#endif
			
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
		
		pushRenderPass(composite[composite_idx], true, nullptr, false, "Light Application");
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
		
		// draw forward shaded opaque and background passes
		
		pushRenderPass(composite[composite_idx], false, buffers.depth, false, "Forward Shaded & Background");
		{
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			updateCullFlip();
			
		#if ENABLE_OPENGL
			gxMatrixMode(GX_PROJECTION);
			gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
			pushCullFlip();
			gxMatrixMode(GX_MODELVIEW);
		#endif
		
			renderOpaque_ForwardShadedPass(
				renderFunctions,
				renderOptions);
		
			renderBackgroundPass(
				renderFunctions,
				renderOptions);
			
		#if ENABLE_OPENGL
			popCullFlip();
		#endif
		}
		popRenderPass();
		
		// create velocity buffer
		
		renderReconstructedVelocityBuffer(
			renderOptions,
			buffers,
			viewportSx,
			viewportSy,
			eyeData);
		
		// apply tri-planar texture projection test
		
	// todo : remove
		if (false)
		{
			pushRenderPass(buffers.normals, false, nullptr, false, "Tri-planer Test");
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
						shader.setTexture("depthTexture", 0, buffers.depth->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture("normalTexture", 1, buffers.normals->getTextureId(), false, false); // note : clamp is intentionally turned off, to expose incorrect sampling
						shader.setTexture("planarTextureX", 3, getTexture("textures/refraction/droplets.png"), true, false);
						shader.setTexture("planarTextureY", 4, getTexture("textures/refraction/droplets.png"), true, false);
						shader.setTexture("planarTextureZ", 5, getTexture("textures/refraction/droplets.png"), true, false);
						shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
						shader.setImmediateMatrix4x4("viewToWorld", viewToWorld.m_v);
						shader.setImmediateMatrix4x4("worldToView", worldToView.m_v);
						shader.setImmediate("time", framework.time);
						drawFullscreenQuad(viewportSx, viewportSy);
					}
					clearShader();
				}
				popBlend();
			}
			popRenderPass();
		}
		
		// post-opaque, pre-translucent post-processing
		
		renderPostOpaqueEffects(
			renderOptions,
			buffers,
			viewportSx,
			viewportSy,
			projectionMatrix,
			composite,
			composite_idx);
		
		// composite translucents on top of the lit opaque
		
		pushRenderPass(composite[composite_idx], false, buffers.depth, false, "Translucent");
		{
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			updateCullFlip();
			
		#if ENABLE_OPENGL
			gxMatrixMode(GX_PROJECTION);
			gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
			pushCullFlip();
			gxMatrixMode(GX_MODELVIEW);
		#endif
			
			renderTranslucentPass(
				renderFunctions,
				renderOptions);
			
		#if ENABLE_OPENGL
			popCullFlip();
		#endif
		}
		popRenderPass();
		
		// render post-effects
		
		renderPostEffects(
			renderOptions,
			buffers,
			viewportSx,
			viewportSy,
			projectionMatrix,
			composite,
			composite_idx);
		
		return composite[composite_idx];
	}

	//

	static ColorTarget * renderModeForwardShaded(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		RenderBuffers & buffers,
		const int viewportSx,
		const int viewportSy,
		const RenderEyeData & eyeData)
	{
		Mat4x4 modelViewMatrix;
		Mat4x4 projectionMatrix;
		gxGetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
		gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		
		// draw colors + normals
		
		ColorTarget * targets[2] =
			{
				buffers.normals,
				buffers.composite1,
			};
		
		pushRenderPass(targets, 2, true, buffers.depth, true, "Normals + Colors & Depth");
		pushShaderOutputs(renderOptions.drawNormals ? "nn" : "nc");
		{
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			updateCullFlip();
			
		#if ENABLE_OPENGL
			gxMatrixMode(GX_PROJECTION);
			gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
			pushCullFlip();
			gxMatrixMode(GX_MODELVIEW);
		#endif
			
			renderOpaquePass(
				renderFunctions,
				renderOptions);
			
			renderOpaque_ForwardShadedPass(
				renderFunctions,
				renderOptions);
			
		#if ENABLE_OPENGL
			popCullFlip();
		#endif
		}
		popShaderOutputs();
		popRenderPass();
		
		// draw background pass
		
		pushRenderPass(buffers.composite1, false, buffers.depth, false, "Background");
		{
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			updateCullFlip();
			
		#if ENABLE_OPENGL
			gxMatrixMode(GX_PROJECTION);
			gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
			pushCullFlip();
			gxMatrixMode(GX_MODELVIEW);
		#endif
		
			renderBackgroundPass(
				renderFunctions,
				renderOptions);
		
		#if ENABLE_OPENGL
			popCullFlip();
		#endif
		}
		popRenderPass();
		
		// create velocity buffer

		renderReconstructedVelocityBuffer(
			renderOptions,
			buffers,
			viewportSx,
			viewportSy,
			eyeData);

		// setup composite ping-pong buffers
		
		ColorTarget * composite[2] =
		{
			buffers.composite1,
			buffers.composite2
		};
		
		int composite_idx = 0;
		
		// post-opaque, pre-translucent post-processing
		
		renderPostOpaqueEffects(
			renderOptions,
			buffers,
			viewportSx,
			viewportSy,
			projectionMatrix,
			composite,
			composite_idx);
		
		// composite translucents on top of the lit opaque
		
		pushRenderPass(composite[composite_idx], false, buffers.depth, false, "Translucent");
		{
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			updateCullFlip();
			
		#if ENABLE_OPENGL
			gxMatrixMode(GX_PROJECTION);
			gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
			pushCullFlip();
			gxMatrixMode(GX_MODELVIEW);
		#endif
			
			renderTranslucentPass(
				renderFunctions,
				renderOptions);
			
		#if ENABLE_OPENGL
			popCullFlip();
		#endif
		}
		popRenderPass();
		
		// render post-effects
		
		renderPostEffects(
			renderOptions,
			buffers,
			viewportSx,
			viewportSy,
			projectionMatrix,
			composite,
			composite_idx);
		
		return composite[composite_idx];
	}

	//

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
			depth->init(sx, sy, DEPTH_FLOAT32, true, 1.f); // note : we will need the depth texture for lighting, so we enable texture access here
			light->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
			composite1->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
			composite2->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
			velocity->init(sx, sy, SURFACE_RG16F, colorBlackTranslucent);
			
		#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
			bloomBuffer->init(sx, sy, SURFACE_RGBA16F, colorBlackTranslucent);
		#endif
		
		#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
			bloomDownsampleChain.free();
			bloomDownsampleChain.alloc(sx, sy);
			bloomBlurChain.free();
			bloomBlurChain.alloc(sx, sy);
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
		bloomDownsampleChain.free();
		bloomBlurChain.free();
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
		//while (tsx > 2 || tsy > 2);
		while (tsx > 8 || tsy > 8);
		
		logDebug("num bloom buffers: %d", numBuffers);
		
		buffers = new ColorTarget[numBuffers];
		
		//
		
		tsx = getNearestPowerOfTwo(sx) << 1;
		tsy = getNearestPowerOfTwo(sy) << 1;
		
		for (int i = 0; i < numBuffers; ++i)
		{
			tsx = tsx/2 > 2 ? tsx/2 : 2;
			tsy = tsy/2 > 2 ? tsy/2 : 2;
			
			buffers[i].init(tsx, tsy, SURFACE_RGBA16F, colorBlackTranslucent);
		}
	}

	void RenderBuffers::BloomChain::free()
	{
		delete [] buffers;
		buffers = nullptr;
		
		numBuffers = 0;
	}

	#endif

	static ColorTarget * renderFromEye(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		const Vec3 & eyeOffset,
		RenderBuffers & buffers,
		const int viewportSx,
		const int viewportSy,
		const float timeStep,
		RenderEyeData & eyeData,
		const bool updateHistory)
	{
		gxPushMatrix();
		Mat4x4 viewMatrix;
		gxGetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
		viewMatrix =
			Mat4x4(true)
			.Translate(eyeOffset[0], eyeOffset[1], eyeOffset[2])
			.Mul(viewMatrix);
		gxLoadMatrixf(viewMatrix.m_v);
		
		// update the per-eye data
		
		if (updateHistory || eyeData.isValid == false)
		{
			eyeData.isValid = true;
			eyeData.timeStep = timeStep;
			
			Mat4x4 modelViewMatrix;
			Mat4x4 projectionMatrix;
			gxGetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
			gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			
			const Mat4x4 worldToProjection_curr = projectionMatrix * modelViewMatrix;
			const Mat4x4 projectionToWorld_curr = worldToProjection_curr.CalcInv();
			
			eyeData.projectionToWorld_prev =
				eyeData.isValid
					? eyeData.projectionToWorld_curr
					: projectionToWorld_curr;
			eyeData.projectionToWorld_curr = projectionToWorld_curr;
			
		#if 0 // enable to use a weird effect created by delayed reprojection matrices
			if (eyeData.projectionToWorld_prev_hist_idx == -1)
			{
				for (auto & m : eyeData.projectionToWorld_prev_hist)
					m = projectionToWorld_curr;
			}
			else
			{
				eyeData.projectionToWorld_prev_hist[eyeData.projectionToWorld_prev_hist_idx] = projectionToWorld_curr;
			}
			
			eyeData.projectionToWorld_prev_hist_idx += 1;
			eyeData.projectionToWorld_prev_hist_idx %= RenderEyeData::kHistorySize;
		
			eyeData.projectionToWorld_prev = eyeData.projectionToWorld_prev_hist[eyeData.projectionToWorld_prev_hist_idx];
		#endif
		}
		
		// invoke the render mode specific render function
		
		ColorTarget * result = nullptr;
		bool hasResult = false;
		
		switch (renderOptions.renderMode)
		{
		case kRenderMode_Flat:
			result = renderModeFlat(renderFunctions, renderOptions);
			hasResult = true;
			break;
			
		case kRenderMode_DeferredShaded:
			result = renderModeDeferredShaded(
				renderFunctions,
				renderOptions,
				buffers,
				viewportSx,
				viewportSy,
				eyeData);
			hasResult = true;
			break;
			
		case kRenderMode_ForwardShaded:
			result = renderModeForwardShaded(
				renderFunctions,
				renderOptions,
				buffers,
				viewportSx,
				viewportSy,
				eyeData);
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
			
		case kRenderMode_ForwardShaded:
			return false;
		}

		Assert(false);
		return false;
	}

	void Renderer::registerShaderOutputs()
	{
		framework.registerShaderOutput('e', "emissive", "float", "shader_fragEmissive");
		framework.registerShaderOutput('s', "specularExponent", "float", "shader_fragSpecularExponent");
		framework.registerShaderOutput('S', "specularColor", "vec4", "shader_fragSpecularColor");
	}

	void Renderer::free()
	{
		buffers.free();
		buffers2.free();
	}
	
	void Renderer::render(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		const int viewportSx,
		const int viewportSy,
		const float timeStep,
		const bool updateHistory)
	{
		if (renderOptions.renderMode != kRenderMode_Flat)
			buffers.alloc(viewportSx, viewportSy, renderOptions.linearColorSpace);
		else
			buffers.free();
		
		if (renderOptions.anaglyphic.enabled)
			buffers2.alloc(viewportSx, viewportSy, renderOptions.linearColorSpace);
		else
			buffers2.free();
		
		//
		
		for (auto * buffer : { &buffers, &buffers2 })
		{
			if (buffer->colors != nullptr)
			{
				// note : the 'colors' buffer stores contents in sRGB color space, so no need to linearize the background color here
				
				buffer->colors->setClearColor(
					renderOptions.backgroundColor[0],
					renderOptions.backgroundColor[1],
					renderOptions.backgroundColor[2], 0.f);
			}
			
			// note : we need to set the background color on the composite buffer too, since the forward shaded render mode draws directly into the (high-precision) composite buffer. it needs to draw into a high-precision buffer, since forward shaded materials are expected to output colors in the linear color space
			
			if (buffer->composite1 != nullptr)
			{
				const Vec3 backgroundColor_linear = srgbToLinear(renderOptions.backgroundColor);
				
				buffer->composite1->setClearColor(
					backgroundColor_linear[0],
					backgroundColor_linear[1],
					backgroundColor_linear[2], 0.f);
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
					updateCullFlip();
			
				#if ENABLE_OPENGL
					gxMatrixMode(GX_PROJECTION);
					gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
					pushCullFlip();
					gxMatrixMode(GX_MODELVIEW);
				#endif
				
					renderFromEye(
						renderFunctions,
						renderOptions,
						Vec3(-renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
						buffers,
						viewportSx,
						viewportSy,
						timeStep,
						eyeData[0],
						updateHistory);
					eyeL = buffers.colors;
					
				#if ENABLE_OPENGL
					popCullFlip();
				#endif
				}
				popRenderPass();

				pushRenderPass(buffers2.colors, true, buffers2.depth, true, "Eye: Right");
				{
					gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
					gxSetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
					updateCullFlip();
					
				#if ENABLE_OPENGL
					gxMatrixMode(GX_PROJECTION);
					gxScalef(1, -1, 1); // note : OpenGL UV-origin is at the bottom-left. verically flip rendering to compensate for this
					pushCullFlip();
					gxMatrixMode(GX_MODELVIEW);
				#endif
				
					renderFromEye(
						renderFunctions,
						renderOptions,
						Vec3(+renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
						buffers2,
						viewportSx,
						viewportSy,
						timeStep,
						eyeData[1],
						updateHistory);
					eyeR = buffers2.colors;
					
				#if ENABLE_OPENGL
					popCullFlip();
				#endif
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
					timeStep,
					eyeData[0],
					updateHistory);
				eyeR = renderFromEye(
					renderFunctions,
					renderOptions,
					Vec3(+renderOptions.anaglyphic.eyeDistance/2.f, 0.f, 0.f),
					buffers2,
					viewportSx,
					viewportSy,
					timeStep,
					eyeData[1],
					updateHistory);
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
					
					drawFullscreenQuad(eyeL->getWidth(), eyeL->getHeight());
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
				timeStep,
				eyeData[0],
				updateHistory);

			if (result != nullptr)
			{
				// blit the result to the frame buffer
			
				projectScreen2d();

				pushBlend(BLEND_OPAQUE);
				{
					gxSetTexture(result->getTextureId());
					setColor(colorWhite);
					drawFullscreenQuad(result->getWidth(), result->getHeight());
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

							for (int i = 0; i < buffers.bloomDownsampleChain.numBuffers; ++i)
							{
								showRenderTarget(buffers.bloomDownsampleChain.buffers[i].getTextureId(), "Bloom down");
							}
							
							nextRow();

							for (int i = 0; i < buffers.bloomBlurChain.numBuffers; ++i)
							{
								showRenderTarget(buffers.bloomBlurChain.buffers[i].getTextureId(), "Bloom blur");
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

	void Renderer::render(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		ColorTarget * colorTarget,
		DepthTarget * depthTarget,
		const float timeStep,
		const bool updateHistory)
	{
		pushRenderPass(colorTarget, true, depthTarget, true, "Render");
		{
			render(
				renderFunctions,
				renderOptions,
				colorTarget->getWidth(),
				colorTarget->getHeight(),
				timeStep,
				updateHistory);
		}
		popRenderPass();
	}

	void Renderer::render(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		const float timeStep,
		const bool updateHistory)
	{
		int viewportSx;
		int viewportSy;
		framework.getCurrentViewportSize(viewportSx, viewportSy);
		
		render(
			renderFunctions,
			renderOptions,
			viewportSx,
			viewportSy,
			timeStep,
			updateHistory);
	}
	
	//
	
	void RenderOptions::ColorGrading::lookupTextureFromFile(const char * filename, GxTexture3d & texture)
	{
		ImageData * image = loadImage(filename);
		
		if (image == nullptr)
		{
			logError("failed to load image: %s", filename);
			texture.free();
		}
		else if (image->sx != kLookupSize * kLookupSize || image->sy != kLookupSize)
		{
			logError("image doesn't adhere to the required size of %dx%d pixels for color grading lookup. filename=%s", kLookupSize * kLookupSize, kLookupSize, filename);
			texture.free();
		}
		else
		{
			if (texture.isChanged(kLookupSize, kLookupSize, kLookupSize, GX_RGBA8_UNORM))
			{
				texture.allocate(kLookupSize, kLookupSize, kLookupSize, GX_RGBA8_UNORM);
			}
			
			texture.upload(image->imageData, 4, 0);
		}
		
		delete image;
		image = nullptr;
	}
	
	void RenderOptions::ColorGrading::lookupTextureFromSrgbColorTransform(RenderOptions::ColorGrading::SrgbColorTransform transform, GxTexture3d & texture)
	{
		float * colors = (float*)malloc(kLookupSize * kLookupSize * kLookupSize * (sizeof(float)*4));
		
		for (int z = 0; z < kLookupSize; ++z)
		{
			const float b = z / float(kLookupSize - 1);
			
			float * __restrict slice = colors + z * kLookupSize * kLookupSize * 4;
			
			for (int y = 0; y < kLookupSize; ++y)
			{
				float * __restrict line = slice + y * kLookupSize * 4;
				
				const float g = y / float(kLookupSize - 1);
				
				for (int x = 0; x < kLookupSize; ++x)
				{
					const float r = x / float(kLookupSize - 1);
					
					float * __restrict out_rgb = line + x * 4;
					
					Color & color = *(Color*)out_rgb;
					color.r = r;
					color.g = g;
					color.b = b;
					
					transform(color);
					
					out_rgb[0] = fmaxf(0.f, fminf(1.f, out_rgb[0]));
					out_rgb[1] = fmaxf(0.f, fminf(1.f, out_rgb[1]));
					out_rgb[2] = fmaxf(0.f, fminf(1.f, out_rgb[2]));
				}
			}
		}
		
		if (texture.isChanged(kLookupSize, kLookupSize, kLookupSize, GX_RGBA32_FLOAT))
		{
			texture.allocate(kLookupSize, kLookupSize, kLookupSize, GX_RGBA32_FLOAT);
		}
		
		texture.upload(colors, 4, 0);
		
		free(colors);
		colors = nullptr;
	}
}

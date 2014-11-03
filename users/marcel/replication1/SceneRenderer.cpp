#include <Windows.h>
#include <gl/gl.h> // fixme, remove
#include "Calc.h"
#include "EntityLight.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "Scene.h"
#include "SceneRenderer.h"
#include "ShapeBuilder.h"

#define SHADOW_SSM_CUBE // Cube SSM for omni lights.
//#define SHADOW_PSM // PSM for fast omni lights.
//#define SHADOW_SSM // SSM for spotlights.

SceneRenderer::SceneRenderer(Scene* scene)
{
	FASSERT(scene);

	m_scene = scene;

	// SSM cube map shadow method.
	m_ssmCubeDepthShader = ShShader(new ResShader());
	m_ssmCubeDepthShader->m_vs = RESMGR.GetVS("shaders/ssm_cube_depth_vs.cg");
	m_ssmCubeDepthShader->m_ps = RESMGR.GetPS("shaders/ssm_cube_depth_ps.cg");
	m_ssmCubeLightShader = ShShader(new ResShader());
	m_ssmCubeLightShader->m_vs = RESMGR.GetVS("shaders/ssm_cube_light_vs.cg");
	m_ssmCubeLightShader->m_ps = RESMGR.GetPS("shaders/ssm_cube_light_ps.cg");
	m_ssmCubeLightShader->InitBlend(true, BLEND_ONE, BLEND_ONE);
	//m_ssmCubeLightShader->InitBlend(true, BLEND_ZERO, BLEND_SRC_COLOR);
	m_ssmCubeTex = ShTexCR(new ResTexCR());

	// PSM shadow method.
	m_psmDepthShader = ShShader(new ResShader());
	m_psmDepthShader->m_vs = RESMGR.GetVS("shaders/psm_depth_vs.cg");
	m_psmDepthShader->m_ps = RESMGR.GetPS("shaders/psm_depth_ps.cg");
	m_psmLightShader = ShShader(new ResShader());
	m_psmLightShader->m_vs = RESMGR.GetVS("shaders/psm_light_vs.cg");
	m_psmLightShader->m_ps = RESMGR.GetPS("shaders/psm_light_ps.cg");

	// SSM shadow method.
	m_ssmDepthShader = ShShader(new ResShader());
	m_ssmDepthShader->m_vs = RESMGR.GetVS("shaders/ssm_depth_vs.cg");
	m_ssmDepthShader->m_ps = RESMGR.GetPS("shaders/ssm_depth_ps.cg");
	m_ssmLightShader = ShShader(new ResShader());
	m_ssmLightShader->m_vs = RESMGR.GetVS("shaders/ssm_light_vs.cg");
	m_ssmLightShader->m_ps = RESMGR.GetPS("shaders/ssm_light_ps.cg");
	m_ssmLightShader->InitBlend(true, BLEND_ONE, BLEND_ONE);
	//m_ssmLightShader->InitBlend(true, BLEND_ZERO, BLEND_SRC_COLOR);
	m_ssmTex = ShTexR(new ResTexR());
	//m_ssmTex->SetTarget(TEXR_DEPTH);
	m_ssmTex->SetTarget(TEXR_COLOR32F);

#if TEST_VOLUME_SPHERES
	if (m_vol_sphere.get() == 0)
	{
		m_vol_sphere = ShShader(new ResShader());
		m_vol_sphere->m_vs = RESMGR.GetVS("shaders/vol_sphere_vs.cg");
		m_vol_sphere->m_ps = RESMGR.GetPS("shaders/vol_sphere_ps.cg");
		m_vol_sphere->InitBlend(true, BLEND_SRC, BLEND_ONE);
		m_vol_sphere->InitWriteDepth(false);

		ShapeBuilder sb;
		sb.CreateCube(&g_alloc, m_vol_mesh);

		m_vol_depth = ShTexR(new ResTexR());
		m_vol_depth->SetTarget(TEXR_COLOR32F);

		m_vol_depthShader = m_ssmDepthShader;
	}
#endif

	m_lightTexA = ShTexRectR(new ResTexRectR());
	m_lightTexD = ShTexD(new ResTexD());
	m_blurTex = ShTexRectR(new ResTexRectR());

	m_plainShader = ShShader(new ResShader());
	m_plainShader->m_vs = RESMGR.GetVS("shaders/ov_plain_vs.cg");
	m_plainShader->m_ps = RESMGR.GetPS("shaders/ov_plain_ps.cg");

	m_blurShader = ShShader(new ResShader());
	m_blurShader->m_vs = RESMGR.GetVS("shaders/blur_vs.cg");
	m_blurShader->m_ps = RESMGR.GetPS("shaders/blur_ps.cg");

	SetCameraPlanes(1.0f, 1000.0f);
	//SetShadowMapSize(512);
	SetShadowMapSize(1024);
}

SceneRenderer::~SceneRenderer()
{
}

void SceneRenderer::Render()
{
	FASSERT(m_scene);

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	// Render color.
	gfx->Clear(BUFFER_ALL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	RenderAll();

#if TEST_VOLUME_SPHERES
	// Render ray traced particles.
	//for (int i = 0; i < 30; ++i)
	for (int i = 0; i < m_scene->m_particleSys.m_particles.GetSize(); ++i)
	{
		Particle* particle = &m_scene->m_particleSys.m_particles[i];

		if (particle->IsDead())
			continue;

		//const Vec3 position(0.0f + sin(i * 1.11f + GetTime() * 0.123f) * 30.0f, 10.0f, 0.0f + cos(i * 1.33f + GetTime() * 0.143f) * 30.0f);
		//const float radius = 10.0f + 8.0f * sin(GetTime() * 0.321f + i);

		const Vec3 position = particle->m_position;
		const float radius = particle->m_life * particle->m_size;

		Mat4x4 w1;
		Mat4x4 w2;
		w1.MakeTranslation(position);
		w2.MakeScaling(Vec3(radius, radius, radius));

		Renderer::I().MatW().Push(w1 * w2);
		{
			m_vol_sphere->m_vs->p["wvp"] =
				Renderer::I().MatP().Top() *
				Renderer::I().MatV().Top() *
				Renderer::I().MatW().Top();
			m_vol_sphere->m_vs->p["wv"] =
				Renderer::I().MatV().Top() *
				Renderer::I().MatW().Top();
			m_vol_sphere->m_vs->p["w"] =
				Renderer::I().MatW().Top();

			m_vol_sphere->m_ps->p["sphere"] = Vec4(Renderer::I().MatV().Top().Mul4(position), radius);
			m_vol_sphere->m_ps->p["color"] = Vec3(0.25f, 0.25f, 0.25f);
			m_vol_sphere->m_ps->p["p"] = Renderer::I().MatP().Top();
			//m_vol_sphere->m_ps->p["depth"] = m_vol_depth.get();

			m_vol_sphere->Apply(gfx);

			gfx->RS(RS_CULL, CULL_CCW);
			Renderer::I().RenderMesh(m_vol_mesh);
			gfx->RS(RS_CULL, CULL_NONE);
		}
		Renderer::I().MatW().Pop();
	}
	gfx->RS(RS_WRITE_DEPTH, 1);
#endif
}

void SceneRenderer::RenderLightDepth(EntityLight* light)
{
	FASSERT(light);

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	switch (light->m_type)
	{
		case LT_OMNI:
		{
			#ifdef SHADOW_SSM_CUBE
			{
				// Render shadow using cube/standard shadow mapping.
				gfx->RS(RS_CULL, CULL_CW); // FIXME, This is weird.. Should be CCW.
				m_scene->m_renderList.SetOverrideShader(m_ssmCubeDepthShader.get());

				Mat4x4 matP;
				GetSSMCubeMatrix(light, matP);

				Renderer::I().MatP().PushI(matP);
				{
					for (int i = 0; i < 6; ++i)
					{
						gfx->SetRT(m_ssmCubeTex->GetFace((CUBE_FACE)i).get());
						gfx->Clear(BUFFER_ALL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

						const Mat4x4 matV = Renderer::I().GetCubeSSMMatrix((CUBE_FACE)i, light->m_position);

						Renderer::I().MatV().PushI(matV);
						{
							m_ssmCubeDepthShader->m_ps->p["lt_pos"] = light->m_position;

							m_scene->m_renderList.Render(RM_SOLID);
						}
						Renderer::I().MatV().Pop();
					}
				}
				Renderer::I().MatP().Pop();

				gfx->SetRT(0);

				m_scene->m_renderList.SetOverrideShader(0);
				gfx->RS(RS_CULL, CULL_NONE);
			}
			#endif

			#ifdef SHADOW_PSM
			{
				m_scene->m_renderList.SetOverrideShader(m_psmDepthShader.get());

				gfx->RS(RS_CULL, CULL_CW);

				Renderer::I().MatP().PushI();
				Renderer::I().MatV().PushI();

				Mat4x4 matP;
				Mat4x4 matV;
				GetPSMLightMatrices(light, matP, matV);

				Renderer::I().MatP().Push(matP);

				gfx->SetRT(m_psmTex.get());
				gfx->Clear(BUFFER_ALL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

				Renderer::I().MatV().Push(matV);

				m_psmDepthShader->m_ps->p["lt_pos"] = light->m_position;

				m_scene->m_renderList.Render(RM_SOLID);

				Renderer::I().MatV().Pop();

				//Renderer::I().MatW().Pop();
				Renderer::I().MatP().Pop();

				Renderer::I().MatP().Pop();
				Renderer::I().MatV().Pop();

				gfx->RS(RS_CULL, CULL_NONE);
				gfx->SetVS(0);
				gfx->SetPS(0);
				m_scene->m_renderList.SetOverrideShader(0);

				gfx->SetRT(0);
			}
			#endif
		}
		break;
		case LT_SPOT:
		{
			#ifdef SHADOW_SSM
			{
				// Render shadow using standard shadow mapping.
				gfx->RS(RS_CULL, CULL_CCW);
				m_scene->m_renderList.SetOverrideShader(m_ssmDepthShader.get());

				gfx->SetRT(m_ssmTex.get());
				gfx->Clear(BUFFER_ALL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

				Mat4x4 matP;
				Mat4x4 matV;
				GetSpotLightMatrices(light, matP, matV);

				Renderer::I().MatP().PushI(matP);
				{
					Renderer::I().MatV().PushI(matV);
					{
						m_ssmDepthShader->m_ps->p["lt_pos"] = light->m_position;

						m_scene->m_renderList.Render(RM_SOLID);
					}
					Renderer::I().MatV().Pop();
				}
				Renderer::I().MatP().Pop();

				gfx->SetRT(0);

				m_scene->m_renderList.SetOverrideShader(0);
				gfx->RS(RS_CULL, CULL_NONE);
			}
			#endif
		}
		break;
	}
}

void SceneRenderer::RenderLightLight(EntityLight* light)
{
	FASSERT(light);

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	switch (light->m_type)
	{
		case LT_OMNI:
		{
			#ifdef SHADOW_SSM_CUBE
			{
				// Render light.
				m_scene->m_renderList.SetOverrideShader(m_ssmCubeLightShader.get());
				gfx->RS(RS_CULL, CULL_CW);

				m_ssmCubeLightShader->m_ps->p["lt_cubeSSM"] = m_ssmCubeTex.get();
				m_ssmCubeLightShader->m_ps->p["lt_pos"] = light->m_position;

				m_scene->m_renderList.Render(RM_SOLID);

				gfx->RS(RS_CULL, CULL_NONE);
				m_scene->m_renderList.SetOverrideShader(0);
				gfx->SetRT(0);
			}
			#endif

			#ifdef SHADOW_PSM
			{
				// TODO: Render light.
			}
			#endif
		}
		break;
		case LT_SPOT:
		{
			#ifdef SHADOW_SSM
			{
				// Render light.
				m_scene->m_renderList.SetOverrideShader(m_ssmLightShader.get());
				gfx->RS(RS_CULL, CULL_CW);

				Mat4x4 matP;
				Mat4x4 matV;
				GetSpotLightMatrices(light, matP, matV);

				// TODO: Translate/scale.
				Mat4x4 texJ;
				Mat4x4 texT;
				Mat4x4 texS;
				texJ.MakeTranslation(Vec3(0.05f + 0.5f / m_ssmTex->GetW(), 0.05f + 0.5f / m_ssmTex->GetH(), 0.0f));
				texJ.MakeIdentity();
				texT.MakeTranslation(Vec3(0.5f, 0.5f, 0.5f));
				texS.MakeScaling(Vec3(0.5f, 0.5f, 0.5f));
				m_ssmLightShader->m_vs->p["tex"] =
					texJ *
					texT *
					texS *
					matP *
					matV;

				m_ssmLightShader->m_ps->p["lt_SSM"] = m_ssmTex.get();
				m_ssmLightShader->m_ps->p["lt_pos"] = light->m_position;
				m_ssmLightShader->m_ps->p["lt_dir0"] = light->m_direction;

				m_scene->m_renderList.Render(RM_SOLID);

				gfx->RS(RS_CULL, CULL_NONE);
				m_scene->m_renderList.SetOverrideShader(0);
				gfx->SetRT(0);
			}
			#endif
		}
		break;
	}
}

void SceneRenderer::RenderLightPass(EntityLight* light)
{
	FASSERT(light);

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	// Render shadow map(s).
	RenderLightDepth(light);

	// Apply lighting.
	gfx->SetRTM(m_lightTexA.get(), 0, 0, 0, 1, m_lightTexD.get());
	gfx->RS(RS_WRITE_DEPTH, 0);
	gfx->RS(RS_DEPTHTEST, 1);
	gfx->RS(RS_DEPTHTEST_FUNC, CMP_EQ);
	gfx->RS(RS_CULL, CULL_CW);

	RenderLightLight(light);

	gfx->SetRT(0);
	gfx->RS(RS_WRITE_DEPTH, 1);
	gfx->RS(RS_DEPTHTEST_FUNC, CMP_LE);
	gfx->RS(RS_CULL, CULL_NONE);
	gfx->RS(RS_BLEND, 0);
}

void SceneRenderer::RenderSolid()
{
	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	gfx->RS(RS_DEPTHTEST, 1);
	gfx->RS(RS_DEPTHTEST_FUNC, CMP_LE);
	gfx->RS(RS_CULL, CULL_CW);

	m_scene->m_renderList.Render(RM_SOLID);

	gfx->RS(RS_CULL, CULL_NONE);
}

void SceneRenderer::RenderLight()
{
	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	FASSERT(gfx);

	std::vector<ShEntity> lights = m_scene->FindEntitiesByClassName("Light");

	float ambient;

	if (lights.size() > 0)
		ambient = 0.25f;
	else
		ambient = 1.0f;

	m_lightTexA->SetSize(gfx->GetRTW(), gfx->GetRTH());
	m_lightTexA->SetTarget(TEXR_COLOR);
	m_lightTexD->SetSize(gfx->GetRTW(), gfx->GetRTH());

	gfx->SetRTM(m_lightTexA.get(), 0, 0, 0, 1, m_lightTexD.get());
	gfx->Clear(BUFFER_ALL, ambient, ambient, ambient, 0.0f, 1.0f);

	// Fill z buffer, required for CMP_EQ + blending.
	// FIXME: wasting fillrate here. ;)
	gfx->RS(RS_WRITE_COLOR, 0);
	m_scene->m_renderList.SetOverrideShader(m_plainShader.get());
	RenderSolid();
	m_scene->m_renderList.SetOverrideShader(0);
	gfx->RS(RS_WRITE_COLOR, 1);

	gfx->SetRT(0);

	for (size_t i = 0; i < lights.size(); ++i)
	{
		EntityLight* light = (EntityLight*)lights[i].get();

		RenderLightPass(light);
	}

#if 0
	gfx->SetRT(0);
	m_blurTex->SetSize(gfx->GetRTW(), gfx->GetRTH());
	BlurTexRectR(m_lightTex.get(), m_blurTex.get());
	BlurTexRectR(m_blurTex.get(), m_lightTex.get());
	gfx->SetRT(m_lightTex.get());
	//gfx->Clear(BUFFER_ALL, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	gfx->SetRT(0);
	//gfx->Clear(BUFFER_ALL, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
#endif

	//gfx->Clear(BUFFER_ALL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	gfx->SetVS(0);
	gfx->SetPS(0);
	gfx->SetTex(0, m_lightTexA.get());
	gfx->RS(RS_CULL, CULL_NONE);
	gfx->RS(RS_DEPTHTEST, 0);
	gfx->RS(RS_BLEND, 1);
	//gfx->RS(RS_BLEND, 0);
	gfx->RS(RS_BLEND_SRC, BLEND_ZERO);
	gfx->RS(RS_BLEND_DST, BLEND_SRC_COLOR);
	Renderer::I().MatP().PushI();
	Renderer::I().MatV().PushI();
	Renderer::I().MatW().PushI();
	// FIXME, OpenGL code.
	glBegin(GL_QUADS);
	{
		//float s = 10.0f;
		float s = 1.0f;
		float w = (float)m_lightTexA->GetW();
		float h = (float)m_lightTexA->GetH();
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-s, -s, 0.0f);
		glTexCoord2f(w, 0.0f);
		glVertex3f(+s, -s, 0.0f);
		glTexCoord2f(w, h);
		glVertex3f(+s, +s, 0.0f);
		glTexCoord2f(0.0f, h);
		glVertex3f(-s, +s, 0.0f);
	}
	glEnd();
	Renderer::I().MatP().Pop();
	Renderer::I().MatV().Pop();
	Renderer::I().MatW().Pop();
	gfx->SetTex(0, 0);
	gfx->RS(RS_BLEND, 1);
	gfx->RS(RS_DEPTHTEST, 1);
}

void SceneRenderer::RenderTransparent()
{
	// Render transparent objects.
	m_scene->m_renderList.Render(RM_BLENDED);
}

void SceneRenderer::RenderAll()
{
	RenderSolid();

#if 0 // fixme, revert (remove opengl code!).
	RenderLight();
#endif

	RenderTransparent();
}

void SceneRenderer::GetSpotLightMatrices(EntityLight* light, Mat4x4& out_matP, Mat4x4& out_matV)
{
	out_matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, m_camNear, m_camFar);

	// FIXME: Up vector unstable.
	out_matV.MakeLookat(light->m_position, light->m_position + light->m_direction, Vec3(0.0f, 1.0f, 0.0f));
}

void SceneRenderer::GetPSMLightMatrices(EntityLight* light, Mat4x4& out_matP, Mat4x4& out_matV)
{
	out_matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, m_camNear, m_camFar);
	out_matV.MakeIdentity(); // TODO.
}

void SceneRenderer::GetSSMCubeMatrix(EntityLight* light, Mat4x4& out_matP)
{
	out_matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, m_camNear, m_camFar);
}

float SceneRenderer::GetAspect()
{
	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	return gfx->GetRTH() / float(gfx->GetRTW());
}

void SceneRenderer::SetCameraPlanes(float nearDistance, float farDistance)
{
	m_camNear = nearDistance;
	m_camFar = farDistance;
}

void SceneRenderer::SetShadowMapSize(int size)
{
	m_ssmCubeTex->SetSize(size, size);
	m_ssmTex->SetSize(size, size);

#if TEST_VOLUME_SPHERES
	m_vol_depth->SetSize(size, size);
#endif
}

void SceneRenderer::BlurTexRectR(ResTexRectR* src, ResTexRectR* dst)
{
	//return;

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	//gfx->SetRT(src);
	//gfx->Clear(BUFFER_ALL, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

	gfx->SetRT(dst);
	//gfx->Clear(BUFFER_ALL, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

	float w = (float)src->GetW();
	float h = (float)src->GetH();

	//m_blurShader->m_ps->p["size"] = 2.0f;
	m_blurShader->m_ps->p["tex"] = src;

	m_blurShader->Apply(gfx);

	Renderer::I().MatP().PushI();
	Renderer::I().MatV().PushI();
	Renderer::I().MatW().PushI();

	gfx->RS(RS_DEPTHTEST, 0);
	gfx->RS(RS_CULL, CULL_NONE);
	gfx->RS(RS_BLEND, 0);

	// FIXME, create once..
#if 1
	Renderer::I().RenderQuad();
#elif 0
	Mesh temp;
	temp.Initialize(PT_TRIANGLE_LIST, 4, FVF_XYZ | FVF_TEX1, 6);
	temp.GetVB()->position[0] = Vec3(-1.0f, +1.0f, 0.5f);
	temp.GetVB()->position[1] = Vec3(+1.0f, +1.0f, 0.5f);
	temp.GetVB()->position[2] = Vec3(+1.0f, -1.0f, 0.5f);
	temp.GetVB()->position[3] = Vec3(-1.0f, -1.0f, 0.5f);
	temp.GetVB()->tex[0][0] = Vec2(0.0f, 0.0f);
	temp.GetVB()->tex[0][1] = Vec2(w,    0.0f);
	temp.GetVB()->tex[0][2] = Vec2(w,    h   );
	temp.GetVB()->tex[0][3] = Vec2(0.0f, h   );
	temp.GetIB()->index[0] = 0;
	temp.GetIB()->index[1] = 1;
	temp.GetIB()->index[2] = 2;
	temp.GetIB()->index[3] = 0;
	temp.GetIB()->index[4] = 2;
	temp.GetIB()->index[5] = 3;
	Renderer::I().RenderMesh(temp);
#else
#if 1
	// FIXME, OpenGL code.
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-1.0f, +1.0f, 0.5f);
		glTexCoord2f(w, 0.0f);
		glVertex3f(+1.0f, +1.0f, 0.5f);
		glTexCoord2f(w, h);
		glVertex3f(+1.0f, -1.0f, 0.5f);
		glTexCoord2f(0.0f, h);
		glVertex3f(-1.0f, -1.0f, 0.5f);
	}
	glEnd();
#endif
#endif

	gfx->RS(RS_DEPTHTEST, 1);

	Renderer::I().MatP().Pop();
	Renderer::I().MatV().Pop();
	Renderer::I().MatW().Pop();

	//gfx->Clear(BUFFER_ALL, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

	gfx->SetRT(0);
}

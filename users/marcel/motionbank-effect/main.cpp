#include "forwardLighting.h"
#include "framework.h"
#include "oscReceiver.h"
#include "Quat.h"
#include "renderer.h"
#include "shadowMapDrawer.h"
#include "StringEx.h"
#include <limits>
#include <map>

using namespace rOne;

struct Joint
{
	Vec3 position;
	Vec4 rotation;
};

struct Body
{
	std::map<std::string, Joint> joints;
	
	float groundLevel = std::numeric_limits<float>::max();
	bool toeWasDown = false;
};

struct Scene
{
	std::map<std::string, Body> bodies;
};

static Scene scene;

struct OscHandler : OscReceiveHandler
{
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		//const char * p = m.AddressPattern();
		
		char temp[256];
		strcpy_s(temp, sizeof(temp), m.AddressPattern());
		for (int i = 0; temp[i] != 0; ++i)
			temp[i] = tolower(temp[i]);
		
		const char * p = temp;
		
		std::string bodyName;
		std::string jointName;
		
		while (p[0] != 0)
		{
			if (bodyName.empty())
			{
				if (p[0] != '/')
					break;
				
				p++;
				
				const char * end = strchr(p, '/');
				
				if (end == nullptr || end == p)
					break;
				
				bodyName.assign(p, end - p);
				
				p = end;
			}
			else if (jointName.empty())
			{
				if (p[0] != '/')
					break;
				
				p++;
				
				const char * end = strchr(p, '/');
				
				if (end != nullptr)
					break;
				
				end = strchr(p, 0);
				
				jointName.assign(p, end - p);
				
				p = end;
				
				if (p[0] != 0)
					break;
				
				auto & joint = scene.bodies[bodyName].joints[jointName];
				
				int index = 0;
				
				for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
				{
					auto & a = *i;
				
					if (index == 0 && a.IsFloat())
						joint.position[0] = a.AsFloatUnchecked();
					if (index == 1 && a.IsFloat())
						joint.position[1] = a.AsFloatUnchecked();
					if (index == 2 && a.IsFloat())
						joint.position[2] = a.AsFloatUnchecked();
					
					if (index == 3 && a.IsFloat())
						joint.rotation[0] = a.AsFloatUnchecked();
					if (index == 4 && a.IsFloat())
						joint.rotation[1] = a.AsFloatUnchecked();
					if (index == 5 && a.IsFloat())
						joint.rotation[2] = a.AsFloatUnchecked();
					if (index == 6 && a.IsFloat())
						joint.rotation[3] = a.AsFloatUnchecked();
					
					index++;
				}
				
				joint.position /= 1000.f; // positions from Effect Player are in mm
				
				break;
			}
		}
	}
};

struct Connection
{
	const char * from;
	const char * to;
};

static Connection s_bodyConnections[] =
{
	// right leg
	{ "hips", "rightupleg" },
	{ "rightupleg", "rightleg" },
	{ "rightleg", "rightfoot" },
	{ "rightfoot", "righttoebase" },
	{ "righttoebase", "righttoebase_endsite" },
	
	// left leg
	{ "hips", "leftupleg" },
	{ "leftupleg", "leftleg" },
	{ "leftleg", "leftfoot" },
	{ "leftfoot", "lefttoebase" },
	{ "lefttoebase", "lefttoebase_endsite" },
	
	// spine and neck
	{ "hips", "spine" },
	{ "spine", "spine1" },
	{ "spine1", "spine2" },
	{ "spine2", "spine3" },
	{ "spine3", "spine4" },
	{ "spine4", "neck" },
	{ "neck", "head" },
	{ "head", "head_endsite" },
	
	// rigth arm
	{ "spine4", "rightshoulder" },
	{ "rightshoulder", "rightarm" },
	{ "rightarm", "rightforearm" },
	{ "rightforearm", "righthand" },
	{ "righthand", "righthand_endsite" },
	
	// left arm
	{ "spine4", "leftshoulder" },
	{ "leftshoulder", "leftarm" },
	{ "leftarm", "leftforearm" },
	{ "leftforearm", "lefthand" },
	{ "lefthand", "lefthand_endsite" }
};

struct Particle
{
	Vec3 position;
	
	float life = 0.f;
	float lifeRcp = 0.f;
};

static std::vector<Particle> particles;

static void tickParticles(const float dt)
{
	// tick particles
	
	for (auto & p : particles)
	{
		p.life = fmaxf(0.f, p.life - dt * p.lifeRcp);
	}
	
	// clean up dead particles
	
	size_t j = 0;
	
	for (size_t i = 0; i < particles.size(); ++i)
	{
		if (particles[i].life == 0.f)
			continue;
		
		particles[j++] = particles[i];
	}
	
	particles.resize(j);
}

static void drawParticles()
{
	beginCubeBatch();
	{
		for (auto & p : particles)
		{
			setColor(colorWhite);
			fillCube(p.position, Vec3(.1f, .01f, .1f) * p.life);
		}
	}
	endCubeBatch();
}

static void addParticle(Vec3Arg position, const float life)
{
	if (life <= 0.f)
		return;
	
	Particle p;
	p.position = position;
	p.life = 1.f;
	p.lifeRcp = 1.f / life;
	
	particles.push_back(p);
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	//if (!framework.init(1200, 600))
	if (!framework.init(800, 400))
	//if (!framework.init(400, 300))
		return -1;
	
	OscReceiver oscReceiver;
	
	if (!oscReceiver.init("127.0.0.1", 8888))
		return -1;
	
	OscHandler oscHandler;
	
	Camera3d camera;
	camera.mouseSmooth = .97f;

	Renderer renderer;
	
	RenderOptions renderOptions;
	renderOptions.renderMode = kRenderMode_ForwardShaded;
	//renderOptions.renderMode = kRenderMode_Flat;
	renderOptions.linearColorSpace = true;
	renderOptions.bloom.enabled = true;
	renderOptions.bloom.strength = .4f;
	//renderOptions.screenSpaceAmbientOcclusion.enabled = true;
	//renderOptions.lightScatter.enabled = true;
	renderOptions.lightScatter.strength = .1f;
	//renderOptions.depthSilhouette.enabled = true;
	renderOptions.depthSilhouette.color.Set(1.f, .1f, .1f, .1f);
	//renderOptions.fog.enabled = true;
	renderOptions.fog.thickness = .1f;
	//renderOptions.colorGrading.enabled = true;
	renderOptions.enableScreenSpaceReflections = true;
	if (renderOptions.fog.enabled)
		renderOptions.backgroundColor.Set(.35f, .25f, .15f);
	else
		renderOptions.backgroundColor.Set(.05f, .05f, .05f);
	renderOptions.fxaa.enabled = true;
	
	ForwardLightingHelper helper;
	
	ShadowMapDrawer shadowMapDrawer;
	shadowMapDrawer.alloc(4, 2048);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		oscReceiver.flushMessages(&oscHandler);
		
		// tick bodies
		
		for (auto & body_itr : scene.bodies)
		{
			auto & body = body_itr.second;
			
			const char * toes[2] = { "righttoebase", "lefttoebase" };
			
			for (int i = 0; i < 2; ++i)
			{
				auto j_itr = body.joints.find(toes[i]);
				
				if (j_itr != body.joints.end())
				{
					auto & j = j_itr->second;
					
					const float height = j.position[1];
					//logDebug("toe height: %.2f", heigth);
					
					body.groundLevel = lerp<float>(height, body.groundLevel, powf(.5f, framework.timeStep/3.f));
					body.groundLevel = fminf(body.groundLevel, height);
					
					const bool toeIsDown = height <= body.groundLevel + .02f;
					
					if (!body.toeWasDown && toeIsDown)
					{
						addParticle(j.position, 4.f);
					}
					else
					{
						addParticle(j.position + Vec3(0, 2, 0), 1.f);
					}
					
					body.toeWasDown = toeIsDown;
				}
			}
		}
		
		// tick particles
		
		tickParticles(framework.timeStep);
		
		// tick camera
		
		camera.maxForwardSpeed = 4.f;
		camera.maxStrafeSpeed = 4.f;
		camera.tick(framework.timeStep, true);
		
		//
		
		Mat4x4 viewMatrix = camera.getViewMatrix();
		
		if (true)
		{
			if (!scene.bodies.empty() > 0)
			{
				auto & body = scene.bodies.begin()->second;
				auto & target = body.joints["hips"].position;
				
				viewMatrix.MakeLookat(Vec3(3, .4f, 0), target, Vec3(0, 1, 0));
			}
		}
		
		// prepare forward lighting data
		
		shadowMapDrawer.reset();
		helper.reset();
		
		for (int i = 0; i < 2; ++i)
		{
			int index = 0;
			
			for (auto & body_itr : scene.bodies)
			{
				const Vec3 position(0, 3, 0);
				const Vec3 target = body_itr.second.joints["hips"].position;
				const Vec3 direction = (target - position).CalcNormalized();
				const float angle = 36.f * float(M_PI/180.0);
				
				Color color = Color::fromHSL(index / 6.f, .2f, .5f);
				
				if (i == 0)
				{
					if (true)
					{
						Mat4x4 lightToWorld;
						lightToWorld.MakeLookat(position, target, Vec3(0, 1, 0));
						lightToWorld = lightToWorld.CalcInv();
						
						shadowMapDrawer.addSpotLight(
							index,
							lightToWorld,
							angle,
							.01f,
							6.f);
					}
				}
				else
				{
					helper.addSpotLight(
						position,
						direction,
						angle,
						6.f,
						Vec3(color.r, color.g, color.b),
						10.f,
						shadowMapDrawer.getShadowMapId(index));
				}
				
				index++;
			}
			
			//helper.addSpotLight(Vec3(0, 3, 0), Vec3(0, -1, 0), float(M_PI/2.f), 3.f, Vec3(1, 1, 1), 10.f);
			//index++;
			
			if (i == 0)
			{
				shadowMapDrawer.drawShadowMaps(viewMatrix);
			}
		}
		
		helper.prepareShaderData(16, 32.f, true, viewMatrix);
		
		// draw scene
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(70.f, .01f, 100.f);
			
			gxPushMatrix();
			{
				gxSetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
				
				auto drawOpaqueBase = [&](const bool isMainPass)
				{
					Shader shader(isMainPass ? "shader-forward" : "shader-shadow");
					setShader(shader);
					int nextTextureUnit = 0;
					helper.setShaderData(shader, nextTextureUnit);
					if (isMainPass)
						shadowMapDrawer.setShaderData(shader, nextTextureUnit, viewMatrix);
					
					gxPushMatrix();
					gxScalef(10, 10, 10);
					setColor(10, 10, 20);
					//setColor(200, 200, 255);
					//drawGrid3dLine(100, 100, 0, 2);
					drawGrid3d(100, 100, 0, 2);
					gxPopMatrix();
					
					for (auto & body_itr : scene.bodies)
					{
						auto & body = body_itr.second;
						
						beginCubeBatch();
						{
							for (auto & joint_itr : body.joints)
							{
								auto & joint = joint_itr.second;
								
								setColorClamp(false);
								setColor(colorWhite, 4.f);
								fillCube(joint.position, Vec3(.03f, .03f, .03f));
							}
						}
						endCubeBatch();
						
						gxBegin(GX_LINES);
						{
							for (int i = 0; i < sizeof(s_bodyConnections) / sizeof(s_bodyConnections[0]); ++i)
							{
								if (body.joints.find(s_bodyConnections[i].from) == body.joints.end() ||
									body.joints.find(s_bodyConnections[i].to)   == body.joints.end())
									continue;
								
								auto & j1 = body.joints[s_bodyConnections[i].from];
								auto & j2 = body.joints[s_bodyConnections[i].to];
								
								gxVertex3fv(&j1.position[0]);
								gxVertex3fv(&j2.position[0]);
							}
						}
						gxEnd();
					}
					
					drawParticles();
					
					clearShader();
				};
				
				auto drawOpaque = [&]()
				{
					drawOpaqueBase(true);
				};
				
				auto drawOpaqueShadow = [&]()
				{
					drawOpaqueBase(false);
				};
				
				shadowMapDrawer.drawOpaque = drawOpaqueShadow;
				
				RenderFunctions renderFunctions;
				renderFunctions.drawOpaque = drawOpaque;
				
				renderer.render(renderFunctions, renderOptions, framework.timeStep);
			}
			gxPopMatrix();
		}
		framework.endDraw();
	}
	
	oscReceiver.shut();
	
	framework.shutdown();
	
	return 0;
}

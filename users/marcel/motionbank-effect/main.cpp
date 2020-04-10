#include "framework.h"
#include "oscReceiver.h"
#include "Quat.h"
#include "renderer.h"
#include "StringEx.h"
#include <map>

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
	for (auto & p : particles)
	{
		setColor(colorWhite);
		fillCube(p.position, Vec3(.1f, .01f, .1f) * p.life);
	}
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
	
	//if (!framework.init(800, 600))
	if (!framework.init(400, 300))
		return -1;
	
	OscReceiver oscReceiver;
	
	if (!oscReceiver.init("127.0.0.1", 8888))
		return -1;
	
	OscHandler oscHandler;
	
	Camera3d camera;

	Renderer renderer;
	
	RenderOptions renderOptions;
	renderOptions.renderMode = kRenderMode_ForwardShaded;
	//renderOptions.renderMode = kRenderMode_Flat;
	renderOptions.linearColorSpace = true;
	renderOptions.bloom.enabled = true;
	renderOptions.bloom.strength = .02f;
	//renderOptions.screenSpaceAmbientOcclusion.enabled = true;
	//renderOptions.lightScatter.enabled = true;
	//renderOptions.depthSilhouette.enabled = true;
	renderOptions.depthSilhouette.color.Set(.1f, .1f, .1f, .5f);
	//renderOptions.fog.enabled = true;
	renderOptions.fog.thickness = .2f;
	renderOptions.backgroundColor.Set(.05f, .05f, .05f);
	
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
		
		// draw scene
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(70.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				auto drawOpaque = [&]()
				{
					drawGrid3dLine(10, 10, 0, 2);
					
					for (auto & body_itr : scene.bodies)
					{
						auto & body = body_itr.second;
						
						for (auto & joint_itr : body.joints)
						{
							auto & joint = joint_itr.second;
							
							setColorClamp(false);
							setColor(colorWhite, 4.f);
							fillCube(joint.position, Vec3(.03f, .03f, .03f));
							
						#if 1
							Mat4x4 r = Mat4x4(true).RotateY(joint.rotation[3]);
						#else
							Quat q;
							q.fromAngleAxis(
								-joint.rotation[3],
								Vec3(
									joint.rotation[0],
									joint.rotation[1],
									joint.rotation[2]).CalcNormalized());
							Mat4x4 r;
							q.toMatrix3x3(r);
						#endif
							const Vec3 d = r.GetAxis(2);
							const Vec3 p1 = joint.position;
							const Vec3 p2 = joint.position - d * .03f;
							gxBegin(GX_LINES);
							gxVertex3fv(&p1[0]);
							gxVertex3fv(&p2[0]);
							gxEnd();
						}
						
						gxBegin(GX_LINES);
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
						gxEnd();
					}
					
					drawParticles();
				};
				
				RenderFunctions renderFunctions;
				renderFunctions.drawOpaque = drawOpaque;
				
				renderer.render(renderFunctions, renderOptions, framework.timeStep);
			}
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	oscReceiver.shut();
	
	framework.shutdown();
	
	return 0;
}

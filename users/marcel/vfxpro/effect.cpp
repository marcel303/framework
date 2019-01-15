#include <GL/glew.h> // GL_TEXTURE_2D_ARRAY
#include "effect.h"
#include "Parse.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "videoloop.h"
#include "xml.h"
#include <cmath>

//

using namespace tinyxml2;

// from internal.h
void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

bool g_isReplay = false;

//

EffectInfosByName g_effectInfosByName;

bool EffectInfosByName::load(const char * filename)
{
	clear();

	bool result = true;

	tinyxml2::XMLDocument xmlDoc;

	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load effect infos by name: %s", filename);

		result = false;
	}
	else
	{
		const XMLElement * xmlEffects = xmlDoc.FirstChildElement("effects");

		if (xmlEffects == nullptr)
		{
			logError("effects element not found");

			result = false;
		}
		else
		{
			for (const XMLElement * xmlEffect = xmlEffects->FirstChildElement("effect"); xmlEffect; xmlEffect = xmlEffect->NextSiblingElement())
			{
				const std::string name = stringAttrib(xmlEffect, "name", "");
				const std::string image = stringAttrib(xmlEffect, "image", "");
				const std::string param1 = stringAttrib(xmlEffect, "param1", "");
				const std::string param2 = stringAttrib(xmlEffect, "param2", "");
				const std::string param3 = stringAttrib(xmlEffect, "param3", "");
				const std::string param4 = stringAttrib(xmlEffect, "param4", "");

				if (name.empty())
				{
					logError("effect name not set");

					result = false;
				}
				else
				{
					EffectInfo effectInfo;

					effectInfo.imageName = image;
					effectInfo.paramName[0] = param1;
					effectInfo.paramName[1] = param2;
					effectInfo.paramName[2] = param3;
					effectInfo.paramName[3] = param4;

					// check not already set
					
					if (count(name) != 0)
					{
						logError("effect with name %s already exists", name.c_str());
						
						result = false;
					}
					else
					{
						(*this)[name] = effectInfo;
					}
				}
			}
		}
	}

	if (!result)
	{
		clear();
	}

	return result;
}

std::string effectParamToName(const std::string & effectName, const std::string & param)
{
	const auto effectInfoItr = g_effectInfosByName.find(effectName);

	if (effectInfoItr == g_effectInfosByName.end())
		return param;

	const EffectInfo & info = effectInfoItr->second;

	if (param == "param1" && !info.paramName[0].empty())
		return info.paramName[0];
	else if (param == "param2" && !info.paramName[1].empty())
		return info.paramName[1];
	else if (param == "param3" && !info.paramName[2].empty())
		return info.paramName[2];
	else if (param == "param4" && !info.paramName[3].empty())
		return info.paramName[3];
	else
		return param;
}

std::string nameToEffectParam(const std::string & effectName, const std::string & name)
{
	const auto effectInfoItr = g_effectInfosByName.find(effectName);

	if (effectInfoItr == g_effectInfosByName.end())
		return name;

	const EffectInfo & info = effectInfoItr->second;

	for (int i = 0; i < 4; ++i)
	{
		if (info.paramName[i] == name)
		{
			char temp[32];
			sprintf_s(temp, sizeof(temp), "param%d", i + 1);
			return temp;
		}
	}

	return name;
}

//

Effect::Effect(const char * name)
	: visible(1.f)
	, is3D(false)
	, is2D(false)
	, is2DAbsolute(false)
	, blendMode(kBlendMode_Add)
	, screenX(0.f)
	, screenY(0.f)
	, scaleX(1.f)
	, scaleY(1.f)
	, scale(1.f)
	, angle(0.f)
	, z(0.f)
	, timeMultiplier(1.f)
	, debugEnabled(true)
{
	addVar("visible", visible);
	addVar("x", screenX);
	addVar("y", screenY);
	addVar("scale_x", scaleX);
	addVar("scale_y", scaleY);
	addVar("scale", scale);
	addVar("angle", angle);
	addVar("z", z);
	addVar("time_multiplier", timeMultiplier);

	transform.MakeIdentity();
}

Effect::~Effect()
{
}

Vec2 Effect::screenToLocal(Vec2Arg v) const
{
	return Vec2(
		(v[0] - screenX) / scaleX,
		(v[1] - screenY) / scaleY);
}

Vec2 Effect::localToScreen(Vec2Arg v) const
{
	return Vec2(
		v[0] * scaleX + screenX,
		v[1] * scaleY + screenY);
}

Vec3 Effect::worldToLocal(Vec3Arg v, const bool withTranslation) const
{
	fassert(is3D);

	const Mat4x4 invTransform = transform.CalcInv();

	if (withTranslation)
		return invTransform.Mul4(v);
	else
		return invTransform.Mul3(v);
}

Vec3 Effect::localToWorld(Vec3Arg v, const bool withTranslation) const
{
	fassert(is3D);

	if (withTranslation)
		return transform.Mul4(v);
	else
		return transform.Mul3(v);
}

void Effect::setTextures(Shader & shader)
{
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
	// fixme-vfxpro : setting colormap_clamp will override sampler settings colormap
	//shader.setTexture("colormap_clamp", 1, g_currentSurface->getTexture(), true, true);
	shader.setTexture("colormap_clamp", 1, g_currentSurface->getTexture(), true, false);
	shader.setTexture("pcm", 2, g_pcmTexture.id, true, false);
	shader.setTexture("fft", 3, g_fftTexture.id, true, false);
	shader.setTexture("fft_faded", 4, g_fftTextureWithFade.id, true, false);
}

void Effect::applyBlendMode() const
{
	switch (blendMode)
	{
	case kBlendMode_Add:
		setBlend(BLEND_ADD);
		break;
	case kBlendMode_Subtract:
		setBlend(BLEND_SUBTRACT);
		break;
	case kBlendMode_Alpha:
		setBlend(BLEND_ALPHA);
		break;
	case kBlendMode_PremultipliedAlpha:
		setBlend(BLEND_PREMULTIPLIED_ALPHA);
		break;
	case kBlendMode_Opaque:
		//setBlend(BLEND_OPAQUE);
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
		break;
	case kBlendMode_Multiply:
		setBlend(BLEND_MUL);
		break;
	default:
		Assert(false);
		break;
	}
}

void Effect::tickBase(const float dt)
{
	const float timeStep = dt * timeMultiplier;

	TweenFloatCollection::tick(timeStep);

	tick(timeStep);
}

TweenFloat * Effect::getVar(const char * name)
{
	const std::string resolvedName = nameToEffectParam(typeName, name);

	return TweenFloatCollection::getVar(resolvedName.c_str());
}

//

EffectDrawable::EffectDrawable(Effect * effect)
	: Drawable(effect->z)
	, m_effect(effect)
{
}

void EffectDrawable::draw()
{
	if (!m_effect->debugEnabled)
		return;
	if (!m_effect->visible)
		return;

	gxPushMatrix();
	{
		if (m_effect->is3D)
			gxMultMatrixf(m_effect->transform.m_v);
		else if (m_effect->is2D)
		{
			gxTranslatef(virtualToScreenX(m_effect->screenX), virtualToScreenY(m_effect->screenY), 0.f);
			gxScalef(m_effect->scaleX * m_effect->scale, m_effect->scaleY * m_effect->scale, 1.f);
			gxRotatef(m_effect->angle, 0.f, 0.f, 1.f);
		}
		else if (m_effect->is2DAbsolute)
		{
			gxTranslatef(m_effect->screenX, m_effect->screenY, 0.f);
			gxScalef(m_effect->scaleX * m_effect->scale, m_effect->scaleY * m_effect->scale, 1.f);
			gxRotatef(m_effect->angle, 0.f, 0.f, 1.f);
		}

		m_effect->applyBlendMode();

		m_effect->draw();

		setBlend(BLEND_ADD);
	}
	gxPopMatrix();
}

//

Effect_StarCluster::Effect_StarCluster(const char * name, const int numStars)
	: Effect(name)
	, m_particleSystem(numStars)
	, m_alpha(1.f)
{
	is2D = true;

	addVar("alpha", m_alpha);
	addVar("gravity_x", m_gravityX);
	addVar("gravity_y", m_gravityY);

	for (int i = 0; i < numStars; ++i)
	{
		int id;

		if (m_particleSystem.alloc(false, 0.f, id))
		{
			const float angle = random(0.f, pi2);
			const float radius = random(10.f, 200.f);
			//const float arcSpeed = radius / 10.f;
			const float arcSpeed = radius / 1.f;

			m_particleSystem.x[id] = cosf(angle) * radius;
			m_particleSystem.y[id] = sinf(angle) * radius;
			//m_particleSystem.vx[id] = cosf(angle + pi2/4.f) * arcSpeed;
			//m_particleSystem.vy[id] = sinf(angle + pi2/4.f) * arcSpeed;
			m_particleSystem.vx[id] = random(-arcSpeed, +arcSpeed);
			m_particleSystem.vy[id] = random(-arcSpeed, +arcSpeed);
			m_particleSystem.sx[id] = 10.f;
			m_particleSystem.sy[id] = 10.f;
		}
	}
}

void Effect_StarCluster::tick(const float dt)
{
	// affect stars based on force from center

	for (int i = 0; i < m_particleSystem.numParticles; ++i)
	{
		if (!m_particleSystem.alive[i])
			continue;

		const float dx = m_particleSystem.x[i] - m_gravityX;
		const float dy = m_particleSystem.y[i] - m_gravityY;
		const float ds = sqrtf(dx * dx + dy * dy) + eps;

#if 0
		const float as = 100.f;
		const float ax = -dx / ds * as;
		const float ay = -dy / ds * as;
#else
		const float ax = -dx;
		const float ay = -dy;
#endif

		m_particleSystem.vx[i] += ax * dt;
		m_particleSystem.vy[i] += ay * dt;

		const float size = ds / 10.f;
		m_particleSystem.sx[i] = size;
		m_particleSystem.sy[i] = size;
	}

	m_particleSystem.tick(dt);
}

void Effect_StarCluster::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_StarCluster::draw()
{
	if (m_alpha <= 0.f)
		return;

	gxSetTexture(Sprite("prayer.png").getTexture());
	{
		setColor(colorWhite);

		m_particleSystem.draw(m_alpha);
	}
	gxSetTexture(0);
}

//

Effect_Cloth::Effect_Cloth(const char * name)
	: Effect(name)
{
	// todo-vfxpro : set is2D (?)

	sx = 0;
	sy = 0;

	memset(vertices, 0, sizeof(vertices));
}

void Effect_Cloth::setup(int _sx, int _sy)
{
	sx = _sx;
	sy = _sy;

	for (int x = 0; x < sx; ++x)
	{
		for (int y = 0; y < sy; ++y)
		{
			Vertex & v = vertices[x][y];

			//v.isFixed = (y == 0) || (y == sy - 1);
			v.isFixed = (y == 0);

			v.x = x;
			v.y = y;
			v.vx = 0.f;
			v.vy = 0.f;

			v.baseX = v.x;
			v.baseY = v.y;
		}
	}
}

Effect_Cloth::Vertex * Effect_Cloth::getVertex(int x, int y)
{
	if (x >= 0 && x < sx && y >= 0 && y < sy)
		return &vertices[x][y];
	else
		return nullptr;
}

void Effect_Cloth::tick(const float dt)
{
	const float gravityX = 0.f;
	const float gravityY = keyboard.isDown(SDLK_g) ? 10.f : 0.f;

	const float springConstant = 100.f;
	const float falloff = .9f;
	//const float falloff = .3f;
	const float falloffThisTick = powf(1.f - falloff, dt);
	const float rigidity = .9f;
	const float rigidityThisTick = powf(1.f - rigidity, dt);

	// update constraints

	const int offsets[4][2] =
	{
		{ -1, 0 },
		{ +1, 0 },
		{ 0, -1 },
		{ 0, +1 }
	};

	for (int x = 0; x < sx; ++x)
	{
		for (int y = 0; y < sy; ++y)
		{
			Vertex & v = vertices[x][y];

			if (v.isFixed)
				continue;

			for (int i = 0; i < 4; ++i)
			{
				const Vertex * other = getVertex(x + offsets[i][0], y + offsets[i][1]);

				if (other == nullptr)
					continue;

				const float dx = other->x - v.x;
				const float dy = other->y - v.y;
				const float ds = sqrtf(dx * dx + dy * dy);

				if (ds > 1.f)
				{
					const float a = (ds - 1.f) * springConstant;

					const float ax = gravityX + dx / (ds + eps) * a;
					const float ay = gravityY + dy / (ds + eps) * a;

					v.vx += ax * dt;
					v.vy += ay * dt;
				}
			}
		}
	}

	// integrate velocity

	for (int x = 0; x < sx; ++x)
	{
		for (int y = 0; y < sy; ++y)
		{
			Vertex & v = vertices[x][y];

			if (v.isFixed)
				continue;

			v.x += v.vx * dt;
			v.y += v.vy * dt;

			v.x = v.x * rigidityThisTick + v.baseX * (1.f - rigidityThisTick);
			v.y = v.y * rigidityThisTick + v.baseY * (1.f - rigidityThisTick);

			v.vx *= falloffThisTick;
			v.vy *= falloffThisTick;
		}
	}
}

void Effect_Cloth::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

// todo-vfxpro : make the transform a part of the drawable or effect

void Effect_Cloth::draw()
{
	gxPushMatrix();
	{
		gxScalef(40.f, 40.f, 1.f);

		//for (int i = 3; i >= 1; --i)
		for (int i = 1; i >= 1; --i)
		{
			glLineWidth(i);
			doDraw();
		}
	}
	gxPopMatrix();
}

void Effect_Cloth::doDraw()
{
	gxColor4f(1.f, 1.f, 1.f, .2f);

	gxBegin(GX_LINES);
	{
		for (int x = 0; x < sx - 1; ++x)
		{
			for (int y = 0; y < sy - 1; ++y)
			{
				const Vertex & v00 = vertices[x + 0][y + 0];
				const Vertex & v10 = vertices[x + 1][y + 0];
				const Vertex & v11 = vertices[x + 1][y + 1];
				const Vertex & v01 = vertices[x + 0][y + 1];

				gxVertex2f(v00.x, v00.y);
				gxVertex2f(v10.x, v10.y);

				gxVertex2f(v10.x, v10.y);
				gxVertex2f(v11.x, v11.y);

				gxVertex2f(v11.x, v11.y);
				gxVertex2f(v01.x, v01.y);

				gxVertex2f(v01.x, v01.y);
				gxVertex2f(v00.x, v00.y);
			}
		}
	}
	gxEnd();
}

//

Effect_Fsfx::Effect_Fsfx(const char * name, const char * shader, const char * image, const std::vector<std::string> & images, const std::vector<Color> & colors)
	: Effect(name)
	, m_alpha(1.f)
	, m_param1(0.f)
	, m_param2(0.f)
	, m_param3(0.f)
	, m_param4(0.f)
	, m_image(image)
	, m_images(images)
	, m_colors(colors)
	, m_textureArray(0)
	, m_time(0.f)
{
	addVar("alpha", m_alpha);
	addVar("param1", m_param1);
	addVar("param2", m_param2);
	addVar("param3", m_param3);
	addVar("param4", m_param4);

	m_shader = shader;

	//

	if (!images.empty())
	{
		auto filenames = images;
		for (size_t i = 0; i < images.size(); ++i)
			filenames[i] = String::Trim(images[i]);

		const Sprite baseSprite(filenames[0].c_str());
		const int sx = baseSprite.getWidth();
		const int sy = baseSprite.getHeight();

		glGenTextures(1, &m_textureArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray);
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY,
			0,
			GL_RGBA8,
			sx, sy,
			images.size(),
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			0);
		checkErrorGL();

		GLuint fbo[2] = { };
		glGenFramebuffers(2, fbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo[0]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[1]);
		checkErrorGL();

		for (size_t i = 0; i < images.size(); ++i)
		{
			const Sprite sprite(filenames[i].c_str());

			glFramebufferTexture(
				GL_READ_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				sprite.getTexture(), 0);
			checkErrorGL();

			glFramebufferTextureLayer(
				GL_DRAW_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				m_textureArray, 0, i);
			checkErrorGL();

			const GLenum readStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
			const GLenum drawStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
			Assert(readStatus == GL_FRAMEBUFFER_COMPLETE);
			Assert(drawStatus == GL_FRAMEBUFFER_COMPLETE);

			glBlitFramebuffer(
				0, 0, sx, sy,
				0, 0, sx, sy,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
			checkErrorGL();
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDeleteFramebuffers(2, fbo);
		checkErrorGL();
	}

	applyTransform();
}

Effect_Fsfx::~Effect_Fsfx()
{
	if (m_textureArray != 0)
	{
		glDeleteTextures(1, &m_textureArray);
		m_textureArray = 0;
	}
}

void Effect_Fsfx::tick(const float dt)
{
	m_time += dt;
}

void Effect_Fsfx::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Fsfx::draw()
{
	if (m_alpha <= 0.f)
		return;

	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	Shader shader(m_shader.c_str(), "fsfx.vs", m_shader.c_str());
	setShader(shader);
	setTextures(shader);
	ShaderBuffer buffer;
	FsfxData data;
	data.alpha = m_alpha;
	//data._time = g_currentScene->m_time;
	data._time = m_time;
	data._param1 = m_param1;
	data._param2 = m_param2;
	data._param3 = m_param3;
	data._param4 = m_param4;
	data._pcmVolume = g_pcmVolume;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("FsfxBlock", buffer);
	if (!m_image.empty())
		shader.setTexture("image", 5, Sprite(m_image.c_str()).getTexture(), true, false);
	shader.setTextureArray("textures", 6, m_textureArray, true, false);
	for (size_t i = 0; i < m_colors.size(); ++i)
	{
		char name[64];
		sprintf_s(name, sizeof(name), "color%d", int(i) + 1);
		const Color & c = m_colors[i];
		shader.setImmediate(name, c.r, c.g, c.b, c.a);
	}
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_Rain::Effect_Rain(const char * name, const int numRainDrops)
	: Effect(name)
	, m_particleSystem(numRainDrops)
	, m_alpha(1.f)
	, m_gravity(100.f)
	, m_falloff(0.f)
	, m_spawnRate(1.f)
	, m_spawnLife(1.f)
	, m_spawnY(0.f)
	, m_bounce(1.f)
	, m_sizeX(1.f)
	, m_sizeY(1.f)
	, m_speedScaleX(0.f)
	, m_speedScaleY(0.f)
	, m_size1(1.f)
	, m_size2(1.f)
{
	is2DAbsolute = true;

	addVar("alpha", m_alpha);
	addVar("gravity", m_gravity);
	addVar("falloff", m_falloff);
	addVar("spawn_rate", m_spawnRate);
	addVar("spawn_life", m_spawnLife);
	addVar("spawn_y", m_spawnY);
	addVar("bounce", m_bounce);
	addVar("size_x", m_sizeX);
	addVar("size_y", m_sizeY);
	addVar("speed_scale_x", m_speedScaleX);
	addVar("speed_scale_y", m_speedScaleY);
	addVar("size1", m_size1);
	addVar("size2", m_size2);

	m_particleSizes.resize(numRainDrops, true);
}

void Effect_Rain::tick(const float dt)
{
	const float gravityY = m_gravity;
	const float falloff = Calc::Max(0.f, 1.f - m_falloff);
	const float falloffThisTick = powf(falloff, dt);

	const Sprite sprite("rain.png");
	const float spriteSx = sprite.getWidth();
	const float spriteSy = sprite.getHeight();

	// spawn particles

	m_spawnTimer.tick(dt);

	while (m_spawnRate != 0.f && m_spawnTimer.consume(1.f / m_spawnRate))
	{
		int id;

		if (!m_particleSystem.alloc(false, m_spawnLife, id))
			continue;

		m_particleSystem.x[id] = rand() % GFX_SX;
		m_particleSystem.y[id] = m_spawnY;
		m_particleSystem.vx[id] = 0.f;
		m_particleSystem.vy[id] = 0.f;
		m_particleSystem.sx[id] = 1.f;
		m_particleSystem.sy[id] = 1.f;

		//m_particleSizes[id] = random(.1f, 1.f) * .25f;
		m_particleSizes[id] = 1.f;
	}

	// update particles

	for (int i = 0; i < m_particleSystem.numParticles; ++i)
	{
		if (!m_particleSystem.alive[i])
			continue;

		// integrate gravity

		m_particleSystem.vy[i] += gravityY * dt;

		// collision and bounce

		if (m_particleSystem.y[i] > GFX_SY)
		{
			m_particleSystem.y[i] = GFX_SY;
			m_particleSystem.vy[i] *= -m_bounce;
		}

		// velocity falloff

		m_particleSystem.vx[i] *= falloffThisTick;
		m_particleSystem.vy[i] *= falloffThisTick;

		// size

		const float life = m_particleSystem.life[i] * m_particleSystem.lifeRcp[i];

		float size = m_particleSizes[i];

		size *= lerp((float)m_size2, (float)m_size1, life);

		m_particleSystem.sx[i] = /*size * */ spriteSx * m_sizeX;
		m_particleSystem.sy[i] = size * spriteSy * m_sizeY;

		if (m_speedScaleX != 0.f)
			m_particleSystem.sx[i] *= m_particleSystem.vx[i] * m_speedScaleX;
		if (m_speedScaleY != 0.f)
			m_particleSystem.sy[i] *= m_particleSystem.vy[i] * m_speedScaleY;

		// check if the particle is dead

		if (m_particleSystem.life[i] == 0.f)
		{
			m_particleSystem.free(i);
		}
	}

	m_particleSystem.tick(dt);
}

void Effect_Rain::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Rain::draw()
{
	if (m_alpha <= 0.f)
		return;

	gxSetTexture(Sprite("rain.png").getTexture());
	{
		setColor(colorWhite);

		m_particleSystem.draw(m_alpha);
	}
	gxSetTexture(0);
}

//

Effect_SpriteSystem::SpriteInfo::SpriteInfo()
	: alive(false)
	, z(0.f)
{
}

Effect_SpriteSystem::SpriteDrawable::SpriteDrawable(SpriteInfo * spriteInfo)
		: Drawable(spriteInfo->z)
		, m_spriteInfo(spriteInfo)
{
}

void Effect_SpriteSystem::SpriteDrawable::draw()
{
	setColorf(1.f, 1.f, 1.f, 1.f);

	Spriter(m_spriteInfo->filename.c_str()).draw(m_spriteInfo->spriterState);
}

Effect_SpriteSystem::Effect_SpriteSystem(const char * name)
	: Effect(name)
{
}

void Effect_SpriteSystem::tick(const float dt)
{
	for (int i = 0; i < kMaxSprites; ++i)
	{
		SpriteInfo & s = m_sprites[i];

		if (!s.alive)
			continue;

		if (s.spriterState.updateAnim(Spriter(s.filename.c_str()), dt))
		{
			// the animation is done. clear the sprite

			s = SpriteInfo();
		}
	}
}

void Effect_SpriteSystem::draw(DrawableList & list)
{
	for (int i = 0; i < kMaxSprites; ++i)
	{
		SpriteInfo & s = m_sprites[i];

		if (!s.alive)
			continue;

		new (list) SpriteDrawable(&s);
	}
}

void Effect_SpriteSystem::draw()
{
	// nop
}

void Effect_SpriteSystem::addSprite(const char * filename, const int animIndex, const float x, const float y, const float z, const float scale)
{
	for (int i = 0; i < kMaxSprites; ++i)
	{
		SpriteInfo & s = m_sprites[i];

		if (s.alive)
			continue;

		s.alive = true;
		s.filename = filename;
		s.spriterState.x = x;
		s.spriterState.y = y;
		s.spriterState.scale = scale;

		s.spriterState.startAnim(Spriter(filename), animIndex);

		return;
	}

	logWarning("failed to find a free sprite! cannot play %s", filename);
}

//

Effect_Boxes::Box::Box()
{
	addVar("x", m_tx);
	addVar("y", m_ty);
	addVar("z", m_tz);

	addVar("sx", m_sx);
	addVar("sy", m_sy);
	addVar("sz", m_sz);

	addVar("rx", m_rx);
	addVar("ry", m_ry);
	addVar("rz", m_rz);
}

Effect_Boxes::Box::~Box()
{
}

bool Effect_Boxes::Box::tick(const float dt)
{
	TweenFloatCollection::tick(dt);

	return true;
}

Effect_Boxes::Effect_Boxes(const char * name, const bool screenSpace, const char * shader)
	: Effect(name)
	, m_shader(shader)
	, m_outline(0.f)
{
	if (screenSpace)
		is2DAbsolute = true;

	addVar("outline", m_outline);
}

Effect_Boxes::~Effect_Boxes()
{
	for (Box * box : m_boxes)
		delete box;
	m_boxes.clear();
}

Effect_Boxes::Box * Effect_Boxes::addBox(
	const char * name,
	const float tx, const float ty, const float tz,
	const float sx, const float sy, const float sz,
	const int axis)
{
	Box * b = new Box();

	b->m_name = name;

	b->m_tx = tx;
	b->m_ty = ty;
	b->m_tz = tz;

	b->m_sx = sx;
	b->m_sy = sy;
	b->m_sz = sz;

	b->m_axis = axis;

	m_boxes.push_back(b);

	return b;
}

Effect_Boxes::Box * Effect_Boxes::findBoxByName(const char * name)
{
	for (auto b : m_boxes)
		if (b->m_name == name)
			return b;

	return nullptr;
}

TweenFloat * Effect_Boxes::getVar(const char * name)
{
	TweenFloat * var = Effect::getVar(name);

	if (var)
		return var;

	std::vector<std::string> parts;
	splitString(name, parts, '.');

	if (parts.size() == 2)
	{
		Box * box = findBoxByName(parts[0].c_str());

		if (box)
			return box->getVar(parts[1].c_str());
		else
			return nullptr;
	}
	else
	{
		return nullptr;
	}
}

void Effect_Boxes::tick(const float dt)
{
	for (auto i = m_boxes.begin(); i != m_boxes.end();)
	{
		Box * b = *i;

		if (!b->tick(dt))
		{
			delete b;
			b = nullptr;

			i = m_boxes.erase(i);
		}
		else
		{
			++i;
		}
	}
}

void Effect_Boxes::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Boxes::draw()
{
	if (m_outline)
		pushWireframe(true);

	gxPushMatrix();
	{
		Light lights[kMaxLights];

		lights[0].setup(kLightType_Omni, 0.f, 0.f, 0.f, .25f, .5f, 1.f, 500.f);
		//lights[0].setup(kLightType_Omni, 0.f, 0.f, 0.f, .25f, .5f, 1.f, config.midiGetValue(100, 1.f) * 5.f);
		//lights[1].setup(kLightType_Omni, -1.f, 0.f, 0.f, 1.f, 1.f, .125f, config.midiGetValue(100, 1.f) * 5.f);

		Shader shader(m_shader.empty() ? "basic_lit" : m_shader.c_str());

		if (!m_shader.empty())
			setShader(shader);

		//ShaderBuffer buffer;
		//buffer.setData(lights, sizeof(lights));
		//shader.setBuffer("lightsBlock", buffer);

		for (auto i = m_boxes.begin(); i != m_boxes.end(); ++i)
		{
			Box & b = **i;

			setColor(colorWhite);

			gxPushMatrix();
			{
				gxScalef(1.f, 1.f, 0.f);

				gxTranslatef(b.m_tx, b.m_ty, b.m_tz);

				gxRotatef(b.m_rx, 1.f, 0.f, 0.f);
				gxRotatef(b.m_ry, 0.f, 1.f, 0.f);
				gxRotatef(b.m_rz, 0.f, 0.f, 1.f);

				gxScalef(b.m_sx, b.m_sy, b.m_sz);

				if (b.m_axis == 0)
					gxRotatef(90, 0, 1, 0);
				//else if (b.m_axis == 1)
				//	gxRotatef(90, 0, 0, 1);
				else if (b.m_axis == 2)
					gxRotatef(90, 1, 0, 0);

				gxBegin(GX_QUADS);
				{
					gxNormal3f(0.f, 0.f, -1.f);
					gxVertex3f(-1.f, -1.f, -1.f);
					gxVertex3f(+1.f, -1.f, -1.f);
					gxVertex3f(+1.f, +1.f, -1.f);
					gxVertex3f(-1.f, +1.f, -1.f);

					gxNormal3f(0.f, 0.f, +1.f);
					gxVertex3f(-1.f, -1.f, +1.f);
					gxVertex3f(+1.f, -1.f, +1.f);
					gxVertex3f(+1.f, +1.f, +1.f);
					gxVertex3f(-1.f, +1.f, +1.f);

					gxNormal3f(-1.f, 0.f, 0.f);
					gxVertex3f(-1.f, -1.f, -1.f);
					gxVertex3f(-1.f, +1.f, -1.f);
					gxVertex3f(-1.f, +1.f, +1.f);
					gxVertex3f(-1.f, -1.f, +1.f);

					gxNormal3f(+1.f, 0.f, 0.f);
					gxVertex3f(+1.f, -1.f, -1.f);
					gxVertex3f(+1.f, +1.f, -1.f);
					gxVertex3f(+1.f, +1.f, +1.f);
					gxVertex3f(+1.f, -1.f, +1.f);
				}
				gxEnd();
			}
			gxPopMatrix();
		}

		clearShader();
	}
	gxPopMatrix();

	if (m_outline)
		popWireframe();
}

void Effect_Boxes::handleSignal(const std::string & message)
{
	if (String::StartsWith(message, "addbox"))
	{
		std::vector<std::string> args;
		splitString(message, args, ',');

		if (args.size() < 2)
		{
			logError("missing parameters! %s", message.c_str());
		}
		else
		{
			Dictionary d;
			d.parse(args[1]);

			Box * box = addBox(
				d.getString("name", "").c_str(),
				d.getFloat("x", 0.f),
				d.getFloat("y", 0.f),
				d.getFloat("z", 0.f),
				d.getFloat("sx", 1.f),
				d.getFloat("sy", 1.f),
				d.getFloat("sz", 1.f),
				d.getInt("axis", 0.f));
			(void)box;
		}
	}
	if (String::StartsWith(message, "transform"))
	{
		std::vector<std::string> args;
		splitString(message, args, ',');

		if (args.size() < 2)
		{
			logError("missing parameters! %s", message.c_str());
		}
		else
		{
			Dictionary d;
			d.parse(args[1]);

			const std::string name = d.getString("name", "");

			Box * box = findBoxByName(name.c_str());

			if (!box)
			{
				logError("box not found: %s", name.c_str());
			}
			else
			{
				const float time = d.getFloat("time", 0.f);
				const EaseType easeType = kEaseType_Linear;
				const float easeParam = 0.f;

				if (d.contains("x"))
					box->m_tx.to(d.getFloat("x", 0.f), time, easeType, easeParam);
				if (d.contains("y"))
					box->m_ty.to(d.getFloat("y", 0.f), time, easeType, easeParam);
				if (d.contains("z"))
					box->m_tz.to(d.getFloat("z", 0.f), time, easeType, easeParam);

				if (d.contains("sx"))
					box->m_sx.to(d.getFloat("sx", 0.f), time, easeType, easeParam);
				if (d.contains("sy"))
					box->m_sy.to(d.getFloat("sy", 0.f), time, easeType, easeParam);
				if (d.contains("sz"))
					box->m_sz.to(d.getFloat("sz", 0.f), time, easeType, easeParam);

				if (d.contains("rx"))
					box->m_rx.to(d.getFloat("rx", 0.f), time, easeType, easeParam);
				if (d.contains("ry"))
					box->m_ry.to(d.getFloat("ry", 0.f), time, easeType, easeParam);
				if (d.contains("rz"))
					box->m_rz.to(d.getFloat("rz", 0.f), time, easeType, easeParam);
			}
		}
	}
}

//

Effect_Picture::Effect_Picture(const char * name, const char * filename, const char * filename2, const char * shader, bool centered)
	: Effect(name)
	, m_alpha(1.f)
	, m_centered(true)
{
	is2D = true;

	addVar("alpha", m_alpha);

	m_filename = filename;
	m_filename2 = filename2;
	m_shader = shader;
	m_centered = centered;
}

void Effect_Picture::tick(const float dt)
{
}

void Effect_Picture::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Picture::draw()
{
	if (m_alpha <= 0.f)
		return;

	Shader shader;

	if (!m_shader.empty())
	{
		shader = Shader(m_shader.c_str());
		setShader(shader);

		shader.setTexture("image", 0, Sprite(m_filename.c_str()).getTexture(), true, true);

		if (!m_filename2.empty())
			shader.setTexture("image2", 1, Sprite(m_filename2.c_str()).getTexture(), true, true);	
	}

	gxPushMatrix();
	{
		Sprite sprite(m_filename.c_str());

		const int sx = sprite.getWidth();
		const int sy = sprite.getHeight();
		const float scaleX = SCREEN_SX / float(sx);
		const float scaleY = SCREEN_SY / float(sy);
		const float scale = Calc::Min(scaleX, scaleY);

		gxScalef(scale, scale, 1.f);
		if (m_centered)
			gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

		setColorf(1.f, 1.f, 1.f, m_alpha);
		sprite.drawEx(0.f, 0.f, 0.f, 1.f, 1.f, false, FILTER_LINEAR);
	}
	gxPopMatrix();

	if (!m_shader.empty())
		clearShader();
}

//

#if ENABLE_VIDEO

const bool kVideoPreload = true;

Effect_Video::Effect_Video(const char * name, const char * filename, const char * shader, const bool yuv,const bool centered, const bool play)
	: Effect(name)
	, m_alpha(1.f)
	, m_yuv(false)
	, m_centered(true)
	, m_hideWhenDone(0.f)
	, m_playing(false)
	, m_time(0.f)
	, m_startTime(0.f)
{
	is2D = true;

	addVar("alpha", m_alpha);
	addVar("hide_when_done", m_hideWhenDone);

	m_filename = filename;
	m_shader = shader;
	m_yuv = yuv;
	m_centered = centered;

	if (kVideoPreload)
	{
		m_mediaPlayer.openAsync(m_filename.c_str(), m_yuv ? MP::kOutputMode_PlanarYUV : MP::kOutputMode_RGBA);
	}

	if (play)
	{
		handleSignal("start");
	}
}

void Effect_Video::tick(const float dt)
{
	if (m_playing)
	{
		m_time += dt;
	}

	if (m_playing && m_mediaPlayer.isActive(m_mediaPlayer.context))
	{
		m_mediaPlayer.presentTime = m_time;

		m_mediaPlayer.tick(m_mediaPlayer.context, true);

		if (m_hideWhenDone && m_mediaPlayer.presentedLastFrame(m_mediaPlayer.context))
		{
			m_mediaPlayer.close(false);

			m_playing = false;
		}
	}
}

void Effect_Video::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Video::draw()
{
	if (m_alpha <= 0.f)
		return;
	if (!m_mediaPlayer.isActive(m_mediaPlayer.context))
		return;
	if (!m_playing)
		return;

	const uint32_t texture = m_mediaPlayer.getTexture();

	if (texture != 0)
	{
		gxPushMatrix();
		{
			const int sx = m_mediaPlayer.texture->sx;
			const int sy = m_mediaPlayer.texture->sy;
			const float scaleX = SCREEN_SX / float(sx);
			
			const float scaleY = SCREEN_SY / float(sy);
			const float scale = Calc::Min(scaleX, scaleY);

			gxScalef(scale, scale, 1.f);
			if (m_centered)
				gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

			if (!m_shader.empty())
			{
				Shader shader(m_shader.c_str());
				setShader(shader);
				shader.setTexture("colormap", 0, texture, true, true);
				setColorf(1.f, 1.f, 1.f, m_alpha);
				drawRect(0, 0, m_mediaPlayer.texture->sx, m_mediaPlayer.texture->sy);
				clearShader();
			}
			else
			{
				setColorf(1.f, 1.f, 1.f, m_alpha);
				gxSetTexture(texture);
				drawRect(0, 0, m_mediaPlayer.texture->sx, m_mediaPlayer.texture->sy);
				gxSetTexture(0);
			}
		}
		gxPopMatrix();
	}
}

void Effect_Video::handleSignal(const std::string & name)
{
	if (name == "start")
	{
		if (!kVideoPreload)
		{
			if (m_mediaPlayer.isActive(m_mediaPlayer.context))
			{
				m_mediaPlayer.close(false);
			}

			m_mediaPlayer.openAsync(m_filename.c_str(), m_yuv ? MP::kOutputMode_PlanarYUV : MP::kOutputMode_RGBA);
		}

		m_startTime = g_currentScene->m_time;
		m_time = 0.0;
		m_playing = true;
	}
}

void Effect_Video::syncTime(const float time)
{
	if (m_playing && m_mediaPlayer.isActive(m_mediaPlayer.context))
	{
		const double videoTime = (time - m_startTime) * timeMultiplier;

		if (videoTime >= 0.0)
		{
			m_mediaPlayer.presentTime = -1.0;
			m_mediaPlayer.seek(videoTime, true);
			m_mediaPlayer.presentTime = videoTime;

			m_time = videoTime;
		}
	}
}

//

Effect_VideoLoop::Effect_VideoLoop(const char * name, const char * filename, const char * shader, const bool yuv,const bool centered, const bool play)
	: Effect(name)
	, m_alpha(1.f)
	, m_yuv(false)
	, m_centered(true)
	, m_videoLoop(nullptr)
	, m_playing(false)
{
	is2D = true;

	addVar("alpha", m_alpha);

	m_filename = filename;
	m_shader = shader;
	m_yuv = yuv;
	m_centered = centered;
	
	m_videoLoop = new VideoLoop(m_filename.c_str(), false);

	if (play)
	{
		handleSignal("start");
	}
}

Effect_VideoLoop::~Effect_VideoLoop()
{
	delete m_videoLoop;
	m_videoLoop = nullptr;
}

void Effect_VideoLoop::tick(const float dt)
{
	if (m_playing)
	{
		m_videoLoop->tick(dt);
	}
	else
	{
		m_videoLoop->tick(0.f);
	}
}

void Effect_VideoLoop::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_VideoLoop::draw()
{
	if (m_alpha <= 0.f)
		return;

	int sx;
	int sy;
	double duration;
	double sampleAspectRatio;
	
	const uint32_t texture = m_videoLoop->getTexture();

	if (texture != 0 && m_videoLoop->getVideoProperties(sx, sy, duration, sampleAspectRatio))
	{
		Assert(sampleAspectRatio == 1.0);
		
		gxPushMatrix();
		{
			const float scaleX = SCREEN_SX / float(sx);
			const float scaleY = SCREEN_SY / float(sy);
			const float scale = Calc::Min(scaleX, scaleY);

			gxScalef(scale, scale, 1.f);
			if (m_centered)
				gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

			if (!m_shader.empty())
			{
				Shader shader(m_shader.c_str());
				setShader(shader);
				shader.setTexture("colormap", 0, texture, true, true);
				shader.setImmediate("alpha", m_alpha);
				drawRect(0, 0, sx, sy);
				clearShader();
			}
			else
			{
				setColorf(1.f, 1.f, 1.f, m_alpha);
				gxSetTexture(texture);
				drawRect(0, 0, sx, sy);
				gxSetTexture(0);
			}
		}
		gxPopMatrix();
	}
}

void Effect_VideoLoop::handleSignal(const std::string & name)
{
	if (name == "start")
	{
		m_playing = true;
	}
}

#endif

//

Effect_Luminance::Effect_Luminance(const char * name)
	: Effect(name)
	, m_alpha(1.f)
	, m_power(1.f)
	, m_mul(1.f)
	, m_darken(0.f)
	, m_darkenAlpha(0.f)
{
	addVar("alpha", m_alpha);
	addVar("power", m_power);
	addVar("mul", m_mul);
	addVar("darken", m_darken);
	addVar("darken_alpha", m_darkenAlpha);
}

void Effect_Luminance::tick(const float dt)
{
}

void Effect_Luminance::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Luminance::draw()
{
	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	Shader shader("luminance");
	setShader(shader);
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
	ShaderBuffer buffer;
	ShaderBuffer buffer2;
	LuminanceData data;
	data.alpha = m_alpha;
	data.power = m_power;
	data.scale = m_mul;
	data.darken = m_darken;
	LuminanceData2 data2;
	data2.darkenAlpha = m_darkenAlpha;
	//logDebug("p=%g, m=%g", (float)m_power, (float)m_mul);
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("LuminanceBlock", buffer);
	buffer2.setData(&data2, sizeof(data2));
	shader.setBuffer("LuminanceBlock2", buffer2);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_ColorLut2D::Effect_ColorLut2D(const char * name, const char * lut)
	: Effect(name)
	, m_lut(lut)
	, m_alpha(1.f)
	, m_lutStart(0.f)
	, m_lutEnd(1.f)
	, m_numTaps(1.f)
{
	addVar("alpha", m_alpha);
	addVar("lut_start", m_lutStart);
	addVar("lut_end", m_lutEnd);
	addVar("num_taps", m_numTaps);
}

void Effect_ColorLut2D::tick(const float dt)
{
}

void Effect_ColorLut2D::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_ColorLut2D::draw()
{
	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	const GLuint lutTexture = Sprite(m_lut.c_str()).getTexture();

	Shader shader("colorlut2d");
	setShader(shader);
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
	shader.setTexture("lut", 1, lutTexture, true, true);
	ShaderBuffer buffer;
	ColorLut2DData data;
	data.alpha = m_alpha;
	data.lutStart = m_lutStart;
	data.lutEnd = m_lutEnd;
	data.numTaps = m_numTaps;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("ColorLut2DBlock", buffer);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_Flowmap::Effect_Flowmap(const char * name, const char * map)
	: Effect(name)
	, m_alpha(1.f)
	, m_strength(0.f)
	, m_darken(0.f)
{
	addVar("alpha", m_alpha);
	addVar("strength", m_strength);
	addVar("darken", m_darken);

	m_map = map;
}

void Effect_Flowmap::tick(const float dt)
{
}

void Effect_Flowmap::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Flowmap::draw()
{
	if (m_alpha <= 0.f)
		return;

	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	Sprite mapSprite(m_map.c_str());

	Shader shader("flowmap");
	setShader(shader);
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
	shader.setTexture("flowmap", 1, mapSprite.getTexture(), true, false);
	shader.setImmediate("flow_time", g_currentScene->m_time);
	ShaderBuffer buffer;
	FlowmapData data;
	data.alpha = m_alpha;
	data.strength = m_strength;
	data.darken = m_darken;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("FlowmapBlock", buffer);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_Vignette::Effect_Vignette(const char * name)
	: Effect(name)
	, m_alpha(1.f)
	, m_innerRadius(0.f)
	, m_distance(100.f)
{
	addVar("alpha", m_alpha);
	addVar("inner_radius", m_innerRadius);
	addVar("distance", m_distance);
}

void Effect_Vignette::tick(const float dt)
{
}

void Effect_Vignette::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Vignette::draw()
{
	if (m_alpha <= 0.f)
		return;

	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	Shader shader("vignette");
	setShader(shader);
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
	ShaderBuffer buffer;
	VignetteData data;
	data.alpha = m_alpha;
	data.innerRadius = m_innerRadius;
	data.distanceRcp = 1.f / m_distance;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("VignetteBlock", buffer);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_Clockwork::Effect_Clockwork(const char * name)
	: Effect(name)
	, m_alpha(1.f)
	, m_innerRadius(0.f)
	, m_distance(100.f)
{
	addVar("alpha", m_alpha);
	addVar("inner_radius", m_innerRadius);
	addVar("distance", m_distance);
}

void Effect_Clockwork::tick(const float dt)
{
}

void Effect_Clockwork::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Clockwork::draw()
{
	if (m_alpha <= 0.f)
		return;

	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	Shader shader("vignette");
	setShader(shader);
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
	ShaderBuffer buffer;
	VignetteData data;
	data.alpha = m_alpha;
	data.innerRadius = m_innerRadius;
	data.distanceRcp = 1.f / m_distance;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("VignetteBlock", buffer);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_DrawPicture::Effect_DrawPicture(const char * name, const char * map)
	: Effect(name)
	, m_alpha(1.f)
	, m_step(10.f)
	, m_draw(false)
	, m_distance(0.f)
{
	addVar("alpha", m_alpha);
	addVar("step", m_step);

	m_map = map;
}

void Effect_DrawPicture::tick(const float dt)
{
	const bool draw = mouse.isDown(BUTTON_LEFT);

	if (draw)
	{
		Coord coord;
		coord.x = mouse.x;
		coord.y = mouse.y;

		if (m_draw)
		{
			const float dx = coord.x - m_lastCoord.x;
			const float dy = coord.y - m_lastCoord.y;
			const float ds = sqrtf(dx * dx + dy * dy);

			if (ds > 0.f)
			{
				float distance = m_distance + ds;

				while (distance >= m_step)
				{
					distance -= m_step;
					m_lastCoord.x += dx / ds * m_step;
					m_lastCoord.y += dy / ds * m_step;
					m_coords.push_back(m_lastCoord);
				}

				const float dx = coord.x - m_lastCoord.x;
				const float dy = coord.y - m_lastCoord.y;
				const float ds = sqrtf(dx * dx + dy * dy);
				m_distance = ds;
			}
		}
		else
		{
			m_lastCoord = coord;
			m_coords.push_back(m_lastCoord);
		}
	}
	else
	{
	}

	m_draw = draw;
}

void Effect_DrawPicture::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_DrawPicture::draw()
{
	if (!m_coords.empty())
	{
		Sprite sprite(m_map.c_str());

		const int sx = sprite.getWidth();
		const int sy = sprite.getHeight();

		gxPushMatrix();
		{
			gxTranslatef(-sx / 2.f * scale, -sy / 2.f * scale, 0.f);
			gxColor4f(1.f, 1.f, 1.f, 1.f);

			for (auto coord : m_coords)
			{
				sprite.drawEx(coord.x, coord.y, 0.f, scale, scale, false, FILTER_LINEAR);
			}
		}
		gxPopMatrix();

		m_coords.clear();
	}
}

//

Effect_Blit::Effect_Blit(const char * name, const char * layer)
	: Effect(name)
	, m_alpha(1.f)
	, m_centered(0.f)
	, m_absolute(1.f)
	, m_srcX(-1.f)
	, m_srcY(-1.f)
	, m_srcSx(0.f)
	, m_srcSy(0.f)
	, m_layer(layer)
{
	addVar("alpha", m_alpha);
	addVar("centered", m_centered);
	addVar("absolute", m_absolute);
	addVar("src_x", m_srcX);
	addVar("src_y", m_srcY);
	addVar("src_sx", m_srcSx);
	addVar("src_sy", m_srcSy);
}

void Effect_Blit::transformCoords(float x, float y, bool addSize, float & out_x, float & out_y, float & out_u, float & out_v)
{
	float srcX = m_srcX;
	float srcY = m_srcY;

	float dstX = 0.f;
	float dstY = 0.f;

	if (addSize)
	{
		srcX += m_srcSx;
		srcY += m_srcSy;

		dstX += m_srcSx;
		dstY += m_srcSy;
	}

	if (m_absolute <= 0.f)
	{
		srcX = virtualToScreenX(srcX);
		srcY = virtualToScreenY(srcY);
	}

	out_u = srcX / SCREEN_SX;
	out_v = srcY / SCREEN_SY;

	out_x = dstX;
	out_y = dstY;
}

void Effect_Blit::tick(const float dt)
{
}

void Effect_Blit::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Blit::draw()
{
	if (m_alpha <= 0.f)
		return;
	if (m_srcSx <= 0.f || m_srcSy <= 0.f)
		return;

	const SceneLayer * layer = g_currentScene->findLayerByName(m_layer.c_str());

	gxPushMatrix();
	{
		if (m_absolute > 0.f)
			gxTranslatef(screenX, screenY, 0.f);
		else
			gxTranslatef(virtualToScreenX(screenX), virtualToScreenY(screenY), 0.f);
		gxScalef(scaleX * scale, scaleY * scale, 1.f);

		gxRotatef(angle, 0.f, 0.f, 1.f);

		if (m_centered)
			gxTranslatef(-m_srcSx / 2.f, -m_srcSy / 2.f, 0.f);

		gxColor4f(1.f, 1.f, 1.f, m_alpha);
		gxSetTexture(layer->m_surface->getTexture());
		{
			gxBegin(GX_QUADS);
			{
				float x1, y1, x2, y2;
				float u1, v1, u2, v2;

				transformCoords(0.f, 0.f, false, x1, y1, u1, v1);
				transformCoords(0.f, 0.f, true,  x2, y2, u2, v2);

				gxTexCoord2f(u1, v1); gxVertex2f(x1, y1);
				gxTexCoord2f(u2, v1); gxVertex2f(x2, y1);
				gxTexCoord2f(u2, v2); gxVertex2f(x2, y2);
				gxTexCoord2f(u1, v2); gxVertex2f(x1, y2);
			}
			gxEnd();
		}
		gxSetTexture(0);
	}
	gxPopMatrix();
}

//

Effect_Blocks::Block::Block()
{
	memset(this, 0, sizeof(*this));
}

Effect_Blocks::Effect_Blocks(const char * name)
	: Effect(name)
	, m_alpha(1.f)
	, m_numBlocks(1)
	, m_minSpeed(1.f)
	, m_maxSpeed(1.f)
	, m_minSize(100.f)
	, m_maxSize(100.f)
{
	addVar("alpha", m_alpha);
	addVar("num_blocks", m_numBlocks);
	addVar("min_speed", m_minSpeed);
	addVar("max_speed", m_maxSpeed);
	addVar("min_size", m_minSize);
	addVar("max_size", m_maxSize);
}

void Effect_Blocks::spawnBlock()
{
	Block b;

	b.x = random(0.f, (float)GFX_SX);
	b.y = random(0.f, (float)GFX_SY);
	b.size = random((float)m_minSize, (float)m_maxSize);
	b.speed = random((float)m_minSpeed, (float)m_maxSpeed);
	b.value = random(0.f, 1.f);

	m_blocks.push_back(b);
}

void Effect_Blocks::tick(const float dt)
{
	for (size_t i = 0; i < m_blocks.size(); ++i)
	{
		Block & b = m_blocks[i];

		b.value += b.speed * dt;
	}

	const size_t numBlocks = (size_t)m_numBlocks;

	while (m_blocks.size() > numBlocks)
		m_blocks.pop_back();
	while (m_blocks.size() < numBlocks)
		spawnBlock();
}

void Effect_Blocks::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Blocks::draw()
{
	if (m_alpha <= 0.f)
		return;

	const GLuint image1 = Sprite("track-healer/block01.png").getTexture();
	const GLuint image2 = Sprite("track-healer/block02.png").getTexture();

	Shader shader("blocks");
	setShader(shader);

	for (size_t i = 0; i < m_blocks.size(); ++i)
	{
		const Block & b = m_blocks[i];

		const float t = (std::sin(b.value * 2.f * M_PI) + 1.f) / 2.f;
		const float scale = Calc::Lerp(1.f, 1.5f, t);
		const float alpha = Calc::Lerp(.2f, .5f, t) * m_alpha;
		const float imageAlpha = t;

		const float sx = scale * b.size / 2.f;
		const float sy = scale * b.size / 2.f;

		shader.setTexture("image1", 0, image1, true, true);
		shader.setTexture("image2", 1, image2, true, true);
		shader.setImmediate("imageAlpha", imageAlpha);
		shader.setImmediate("alpha", alpha);

		gxBegin(GX_QUADS);
		{
			gxTexCoord2f(0.f, 0.f); gxVertex2f(b.x - sx, b.y - sy);
			gxTexCoord2f(1.f, 0.f); gxVertex2f(b.x + sx, b.y - sy);
			gxTexCoord2f(1.f, 1.f); gxVertex2f(b.x + sx, b.y + sy);
			gxTexCoord2f(0.f, 1.f); gxVertex2f(b.x - sx, b.y + sy);
		}
		gxEnd();
	}

	clearShader();
}

//

Effect_Lines::Line::Line()
{
	memset(this, 0, sizeof(*this));
}

Effect_Lines::Effect_Lines(const char * name, const int numLines)
	: Effect(name)
	, m_alpha(1.f)
	, m_spawnRate(0.f)
	, m_minSpeed(100.f)
	, m_maxSpeed(100.f)
	, m_minSize(100.f)
	, m_maxSize(100.f)
	, m_color1(0.f)
	, m_color2(1.f)
	, m_thickness(10.f)
	, m_spawnTimer(0.f)
{
	addVar("alpha", m_alpha);
	addVar("spawn_rate", m_spawnRate);
	addVar("min_speed", m_minSpeed);
	addVar("max_speed", m_maxSpeed);
	addVar("min_size", m_minSize);
	addVar("max_size", m_maxSize);
	addVar("color1", m_color1);
	addVar("color2", m_color2);
	addVar("thickness", m_thickness);
	addVar("spawn_timer", m_spawnTimer);

	m_lines.resize(numLines);
}

void Effect_Lines::spawnLine()
{
	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		if (!m_lines[i].active)
		{
			const bool h = (rand() % 2) != 0;
			const int d = (rand() % 2) ? -1 : +1;
			const float speed = random((float)m_minSpeed, (float)m_maxSpeed);

			m_lines[i].active = true;
			m_lines[i].speedX = ( h ? speed : 0.f) * d;
			m_lines[i].speedY = (!h ? speed : 0.f) * d;
			m_lines[i].sx =  h ? random((float)m_minSize, (float)m_maxSize) : m_thickness;
			m_lines[i].sy = !h ? random((float)m_minSize, (float)m_maxSize) : m_thickness;
			m_lines[i].x =  h ? (d > 0 ? -m_lines[i].sx : GFX_SX) : (rand() % GFX_SX);
			m_lines[i].y = !h ? (d > 0 ? -m_lines[i].sy : GFX_SY) : (rand() % GFX_SY);
			m_lines[i].c = random((float)m_color1, (float)m_color2);

			break;
		}
	}
}

void Effect_Lines::tick(const float dt)
{
	const float spawnRate = m_spawnRate;

	if (spawnRate > 0)
	{
		const float spawnTime = 1.f / spawnRate;

		while (m_spawnTimer >= spawnTime)
		{
			m_spawnTimer = m_spawnTimer - spawnTime;

			spawnLine();
		}
	
		m_spawnTimer = m_spawnTimer + dt;
	}

	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		if (m_lines[i].active)
		{
			m_lines[i].x += m_lines[i].speedX * dt;
			m_lines[i].y += m_lines[i].speedY * dt;

			//

			if ((m_lines[i].x + m_lines[i].sx < 0.f && m_lines[i].speedX < 0.f) ||
				(m_lines[i].y + m_lines[i].sy < 0.f && m_lines[i].speedY < 0.f) ||
				(m_lines[i].x > GFX_SX && m_lines[i].speedX > 0.f) ||
				(m_lines[i].y > GFX_SY && m_lines[i].speedY > 0.f))
			{
				m_lines[i] = Line();
			}
		}
	}
}

void Effect_Lines::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Lines::draw()
{
	if (m_alpha <= 0.f)
		return;

	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		if (m_lines[i].active)
		{
			setColorf(
				m_lines[i].c,
				m_lines[i].c,
				m_lines[i].c,
				m_alpha);

			drawRect(
				m_lines[i].x,
				m_lines[i].y,
				m_lines[i].x + m_lines[i].sx,
				m_lines[i].y + m_lines[i].sy);
		}
	}
}

//

Effect_Bars::Bar::Bar()
{
	memset(this, 0, sizeof(*this));
}

Effect_Bars::Effect_Bars(const char * name)
	: Effect(name)
	, m_alpha(1.f)
	, m_shuffleRate(0.f)
	, m_shuffleTimer(0.f)
	, m_shuffle(0.f)
	, m_minSize(100.f)
	, m_maxSize(100.f)
	, m_sizePow(1.f)
{
	addVar("alpha", m_alpha);
	addVar("shuffle_rate", m_shuffleRate);
	addVar("shuffle_timer", m_shuffleTimer);
	addVar("shuffle", m_shuffle);
	addVar("base_size", m_baseSize);
	addVar("min_size", m_minSize);
	addVar("max_size", m_maxSize);
	addVar("size_pow", m_sizePow);
	addVar("top_alpha", m_topAlpha);
	addVar("bottom_alpha", m_bottomAlpha);

#if 1
	m_colorBarColors.push_back(Color::fromHex("000000"));
	m_colorBarColors.push_back(Color::fromHex("ffffff"));
	m_colorBarColors.push_back(Color::fromHex("9c9c9c"));
	m_colorBarColors.push_back(Color::fromHex("1e1e1e"));
	m_colorBarColors.push_back(Color::fromHex("575556"));
#else
	const static Color s_colorBarColors[] =
	{
		Color::fromHex("000000"),
		Color::fromHex("ffffff"),
		Color::fromHex("a94801"),
		Color::fromHex("9c9c9c"),
		Color::fromHex("c78b06"), // ?
		Color::fromHex("f7c600"),
		Color::fromHex("575556"),
		Color::fromHex("1e1e1e")
	};
#endif
}

void Effect_Bars::initializeBars()
{
	for (int layer = 0; layer < kNumLayers; ++layer)
	{
		float x = 0.f;

		while (x < GFX_SX)
		{
			const float t = std::powf(random(0.f, 1.f), m_sizePow);

			Bar bar;

		#if 1
			bar.skipSize = Calc::Lerp(m_minSize, m_maxSize, t);
			bar.drawSize = bar.skipSize;
		#else
			if (layer == 0)
			{
				bar.skipSize = m_baseSize;
				bar.drawSize = m_baseSize;
			}
			else
			{
				const float s = 1.f / std::powf(layer + 1.f, m_sizePow);

				bar.skipSize = m_baseSize * s;
				bar.drawSize = Calc::Lerp(m_minSize, m_maxSize, t) * s;
			}
		#endif

			bar.color = rand() % m_colorBarColors.size();

			m_bars[layer].push_back(bar);

			x += bar.skipSize;
		}
	}
}

void Effect_Bars::shuffleBar(int layer)
{
	if (m_bars[layer].size() >= 2)
	{
		for (;;)
		{
			const int index1 = rand() % m_bars[layer].size();
			const int index2 = rand() % m_bars[layer].size();

			if (index1 != index2)
			{
				std::swap(m_bars[layer][index1], m_bars[layer][index2]);

				break;
			}
		}
	}
}

void Effect_Bars::tick(const float dt)
{
	if (m_bars[0].empty())
		initializeBars();

	const float shuffleRate = m_shuffleRate;

	if (shuffleRate > 0)
	{
		const float shuffleTime = 1.f / shuffleRate;

		while (m_shuffleTimer >= shuffleTime)
		{
			m_shuffleTimer = m_shuffleTimer - shuffleTime;

			m_shuffle = m_shuffle + 1.f;
		}

		m_shuffleTimer = m_shuffleTimer + dt;
	}

	while (m_shuffle >= 0.f)
	{
		m_shuffle = m_shuffle - 1;

		for (int i = 0; i < kNumLayers; ++i)
			shuffleBar(i);
	}
}

void Effect_Bars::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Bars::draw()
{
	if (m_alpha <= 0.f)
		return;

	gxBegin(GX_QUADS);
	{
		for (int layer = 0; layer < kNumLayers; ++layer)
		{
			float x = 0.f;

			for (size_t i = 0; i < m_bars[layer].size(); ++i)
			{
				const Bar & b = m_bars[layer][i];

				const Color & color = m_colorBarColors[b.color];

				x += b.skipSize/2.f;

				gxColor4f(color.r, color.g, color.b, m_topAlpha * m_alpha);
				gxVertex2f(x - b.drawSize/2.f, 0.f);
				gxVertex2f(x + b.drawSize/2.f, 0.f);
				gxColor4f(color.r, color.g, color.b, m_bottomAlpha * m_alpha);
				gxVertex2f(x + b.drawSize/2.f, GFX_SY);
				gxVertex2f(x - b.drawSize/2.f, GFX_SY);

				x += b.skipSize/2.f;
			}
		}
	}
	gxEnd();
}

void Effect_Bars::handleSignal(const std::string & message)
{
	if (String::StartsWith(message, "replace_color"))
	{
		std::vector<std::string> args;
		splitString(message, args, ',');

		if (args.size() == 3)
		{
			const int index = Parse::Int32(args[1]);
			const Color color = Color::fromHex(args[2].c_str());

			if (index >= 0 && index < (int)m_colorBarColors.size())
				m_colorBarColors[index] = color;
		}
	}
}

//

Effect_Text::Effect_Text(const char * name, const Color & color, const char * font, const int fontSize, const char * text)
	: Effect(name)
	, m_alpha(1.f)
	, m_color(color)
	, m_font(font)
	, m_fontSize(fontSize)
	, m_text(text)
	, m_textAlignX(0.f)
	, m_textAlignY(0.f)
{
	addVar("alpha", m_alpha);
	addVar("align_x", m_textAlignX);
	addVar("align_y", m_textAlignY);
}

void Effect_Text::tick(const float dt)
{
}

void Effect_Text::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Text::draw()
{
	if (m_alpha <= 0.f)
		return;

	setColorf(m_color.r, m_color.g, m_color.b, m_color.a * m_alpha);
	setFont(m_font.c_str());
	drawText(screenX, screenY, m_fontSize, m_textAlignX, m_textAlignY, "%s", m_text.c_str());
}

//

Effect_Bezier::Effect_Bezier(const char * name, const char * colors)
	: Effect(name)
	, m_alpha(1.f)
{
	is2DAbsolute = true;

	addVar("alpha", m_alpha);

	//

	std::vector<std::string> colorArgs;
	splitString(colors, colorArgs, ',');
	if (colorArgs.empty())
		colorArgs.push_back("ffffffff");

	for (size_t i = 0; i < colorArgs.size(); ++i)
	{
		ColorCurve::Key * key;
		if (colorCurve.allocKey(key))
		{
			key->t = i / float(colorArgs.size() - 1);
			key->color = Color::fromHex(colorArgs[i].c_str());
		}
	}

	colorCurve.sortKeys();
}

void Effect_Bezier::tick(const float dt)
{
	// evolve bezier paths

	for (auto si = segments.begin(); si != segments.end();)
	{
		auto & s = *si;

		s.time -= dt;

		if (s.time < 0.f)
		{
			si = segments.erase(si);
		}
		else
		{
			s.growTime -= dt;
			
			if (s.growTime < 0.f)
				s.growTime = 0.f;

			for (auto & n : s.nodes)
			{
				n.moveDelay -= dt;

				if (n.moveDelay <= 0.f)
				{
					n.moveDelay = 0.f;
					
					n.time += dt;

					n.bezierNode.m_Position += n.positionSpeed * dt;
					n.bezierNode.m_Tangent[0] += n.tangentSpeed * dt;
				}
			}

			++si;
		}
	}
}

void Effect_Bezier::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

static float mixValue(const float t1, const float t2, const float o, const float t)
{
	const float o1 = t1 - o;
	const float o2 = t2 + o;

	if (t < o1)
		return 0.f;
	if (t > o2)
		return 0.f;

	const float v = (t - o1) / (o2 - o1);

	return (1.f - cosf(v * Calc::m2PI)) / 2.f;
}

void Effect_Bezier::draw()
{
	if (m_alpha <= 0.f)
		return;

	for (auto & s : segments)
	{
		const float a = s.time * s.timeRcp;
		const float t = 1.f - a;

		const float l = 1.f - (s.growTime * s.growTimeRcp);

		Color baseColor;
		colorCurve.sample(t, baseColor);
		baseColor.a *= m_alpha;

		std::vector<BezierNode> nodes;
		nodes.resize(s.nodes.size());

		const int numLoops = s.numGhosts * 2 + 1;

		for (int g = 0; g < numLoops; ++g)
		{
			const float jt = numLoops >= 2 ? (g / float(numLoops - 1)) : .5f;
			const float ja = lerp(-1.f, +1.f, jt);
			//const float jb = ja * t * s.ghostSize;

			for (size_t i = 0; i < s.nodes.size(); ++i)
			{
				const float jb = ja * s.nodes[i].time * s.timeRcp * s.ghostSize;

				nodes[i] = s.nodes[i].bezierNode;

				nodes[i].m_Position += s.nodes[i].positionSpeed * jb;
				nodes[i].m_Tangent[0] += s.nodes[i].tangentSpeed * jb;

				nodes[i].m_Tangent[1] = - nodes[i].m_Tangent[0];
			}

			BezierPath path;
			path.ConstructFromNodes(&nodes.front(), nodes.size());

			pushLineSmooth(true);

			gxBegin(GX_LINE_STRIP);
			{
				const Color color = baseColor.mulRGBA(s.color);

				gxColor4f(color.r, color.g, color.b, color.a);

				const float end = nodes.size() * l;

				for (float i = 0.f; i <= end; i += 0.01f)
				{
					const Vec2F p = path.Interpolate(i);

					gxVertex2f(p[0], p[1]);
				}
			}
			gxEnd();

			popLineSmooth();
		}
	}
}

void Effect_Bezier::handleSignal(const std::string & name)
{
	if (String::StartsWith(name, "addpart"))
	{
		std::vector<std::string> args;
		splitString(name, args, ',');

		if (args.size() < 2)
		{
			logError("missing segment parameters! %s", name.c_str());
		}
		else
		{
			Dictionary d;
			d.parse(args[1]);

			const float x1 = d.getFloat("x1", 0.f);
			const float y1 = d.getFloat("y1", 0.f);
			const float x2 = d.getFloat("x2", 0.f);
			const float y2 = d.getFloat("y2", 0.f);

			const std::string colorHex = d.getString("color", "");
			const Color color = colorHex.empty() ? colorWhite : Color::fromHex(colorHex.c_str());
			const int numSegments = Calc::Max(2, d.getInt("num_segments", 2));
			const int numGhosts = Calc::Max(0, d.getInt("num_ghosts", 0));
			const float ghostSize = d.getFloat("ghost_size", 1.f);
			const float lifeTime = d.getFloat("life", 1.f);
			const float growTime = d.getFloat("grow_time", 0.f);
			const bool fixedEnds = d.getBool("fixed_ends", false);

			const float posSpeed = d.getFloat("pos_speed", 0.f);
			const float tanSpeed = d.getFloat("tan_speed", 0.f);
			const float posVary = d.getFloat("pos_vary", 0.f);
			const float tanVary = d.getFloat("tan_vary", 0.f);

			generateSegment(Vec2F(x1, y1), Vec2F(x2, y2), posSpeed, tanSpeed, posVary, tanVary, numSegments, numGhosts, ghostSize, color, lifeTime, growTime, fixedEnds);
		}
	}
	else if (name == "addthrow")
	{
		generateThrow();
	}
	else
	{
		logError("unknown signal: %s", name.c_str());
	}
}

Vec2F sampleThrowPoint(const float t)
{
	const float o = .1f;

	const float d0 = 0.f;
	const float d1 = 1.f;
	const float d2 = 3.f;
	const float d3 = 4.f;

	const float h = 100.f;

	const float mt = t * 4.f;

	float x[3];
	float y[3];
	float w[3];

	// right -> center
	x[0] = 500.f - (mt - d0) * 500.f;
	y[0] = + cosf((mt - d0 + 0.f) * Calc::mPI) * h;
	w[0] = mixValue(d0, d1, o, mt);

	// center circle
	x[1] = - sinf((mt - d1) * Calc::mPI) * h;
	y[1] = - cosf((mt - d1) * Calc::mPI) * h;
	w[1] = mixValue(d1, d2, o, mt);

	// center -> left
	x[2] = - (mt - d2) * 500.f;
	y[2] = + cosf((mt - d2 + 1.f) * Calc::mPI) * h;
	w[2] = mixValue(d2, d3, o, mt);

	// sample
	float wt = 0.f;
	for (int j = 0; j < 3; ++j)
		wt += w[j];

	Assert(wt != 0.f);
	if (wt == 0.f)
		return Vec2F(0.f, 0.f);

	float wx = 0.f;
	float wy = 0.f;

	for (int j = 0; j < 3; ++j)
	{
		wx += x[j] * w[j] / wt;
		wy += y[j] * w[j] / wt;
	}

	return Vec2F(wx, wy);
}

void generateThrowPoints(Vec2F * points, const int numPoints)
{
	for (int i = 0; i < numPoints; ++i)
	{
		const float t = i / float(numPoints - 1);

		points[i] = sampleThrowPoint(t);
	}
}

void Effect_Bezier::generateThrow()
{
	logDebug("generateThrow. sceneTime=%f", g_currentScene->m_time);

	const int numPoints = 9;

	const Color color = colorWhite;
	const float duration = 8.f;
	const float growTime = 4.f;
	const int numGhosts = 2;
	const float ghostSize = 20.f;

	const bool fixedEnds = false;
	const float posVary = 0.f;
	const float tanVary = 0.f;
	const float posSpeed = 5.f;
	const float tanSpeed = 20.f;

	Segment segment;

	segment.color = color;
	segment.time = duration;
	segment.timeRcp = 1.f / duration;
	segment.growTime = growTime;
	segment.growTimeRcp = growTime != 0.f ? 1.f / growTime : 0.f;
	segment.numGhosts = numGhosts;
	segment.ghostSize = ghostSize;

	for (int i = 0; i < numPoints; ++i)
	{
		const float t = i / float(numPoints - 1);

		const float eps = .01f;

		const Vec2F p1 = sampleThrowPoint(t - eps);
		const Vec2F p2 = sampleThrowPoint(t + eps);

		Vec2F p = (p1 + p2) / 2.f;

		if (!fixedEnds || (i != 0 && i != numPoints - 1))
		{
			p[0] += random(-posVary/2.f, +posVary/2.f);
			p[1] += random(-posVary/2.f, +posVary/2.f);
		}

		const Vec2F d = - (p2 - p1) / eps * .02f;

		Node node;

		node.bezierNode.m_Position = p;
		node.bezierNode.m_Tangent[0][0] = d[0] + random(-tanVary/2.f, +tanVary/2.f);
		node.bezierNode.m_Tangent[0][1] = d[1] + random(-tanVary/2.f, +tanVary/2.f);
		node.bezierNode.m_Tangent[1] = - node.bezierNode.m_Tangent[0];

		node.positionSpeed[0] = random(-posSpeed/2.f, +posSpeed/2.f);
		node.positionSpeed[1] = random(-posSpeed/2.f, +posSpeed/2.f);

		node.tangentSpeed[0] = random(-tanSpeed/2.f, +tanSpeed/2.f);
		node.tangentSpeed[1] = random(-tanSpeed/2.f, +tanSpeed/2.f);

		node.moveDelay = t * growTime;

		segment.nodes.push_back(node);
	}

	segments.push_back(segment);
}

void Effect_Bezier::generateSegment(const Vec2F & p1, const Vec2F & p2, const float posSpeed, const float tanSpeed, const float posVary, const float tanVary, const int numNodes, const int numGhosts, const float ghostSize, const Color & color, const float duration, const float growTime, const bool fixedEnds)
{
	Segment segment;

	segment.color = color;
	segment.time = duration;
	segment.timeRcp = 1.f / duration;
	segment.growTime = growTime;
	segment.growTimeRcp = growTime != 0.f ? 1.f / growTime : 0.f;
	segment.numGhosts = numGhosts;
	segment.ghostSize = ghostSize;

	for (int i = 0; i < numNodes; ++i)
	{
		const float v = i / float(numNodes - 1.f);
		
		Vec2F p = lerp(p1, p2, v);

		if (!fixedEnds || (i != 0 && i != numNodes - 1))
		{
			p[0] += random(-posVary/2.f, +posVary/2.f);
			p[1] += random(-posVary/2.f, +posVary/2.f);
		}

		Node node;

		node.bezierNode.m_Position = p;
		node.bezierNode.m_Tangent[0][0] = random(-tanVary/2.f, +tanVary/2.f);
		node.bezierNode.m_Tangent[0][1] = random(-tanVary/2.f, +tanVary/2.f);
		node.bezierNode.m_Tangent[1] = - node.bezierNode.m_Tangent[0];

		node.positionSpeed[0] = random(-posSpeed/2.f, +posSpeed/2.f);
		node.positionSpeed[1] = random(-posSpeed/2.f, +posSpeed/2.f);

		node.tangentSpeed[0] = random(-tanSpeed/2.f, +tanSpeed/2.f);
		node.tangentSpeed[1] = random(-tanSpeed/2.f, +tanSpeed/2.f);

		node.moveDelay = 0.f;

		segment.nodes.push_back(node);
	}

	segments.push_back(segment);
}

//

Effect_Smoke::Effect_Smoke(const char * name, const char * layer)
	: Effect(name)
	, m_surface(nullptr)
	, m_capture(false)
	, m_layer(layer)
	, m_alpha(1.f)
	, m_strength(0.f)
	, m_darken(0.f)
	, m_darkenAlpha(0.f)
	, m_multiply(1.f)
{
	addVar("alpha", m_alpha);
	addVar("strength", m_strength);
	addVar("darken", m_darken);
	addVar("darken_alpha", m_darkenAlpha);
	addVar("multiply", m_multiply);

	m_surface = new Surface(GFX_SX, GFX_SY, true);
	m_surface->clear();
}

Effect_Smoke::~Effect_Smoke()
{
	delete m_surface;
	m_surface = nullptr;
}

void Effect_Smoke::tick(const float dt)
{
}

void Effect_Smoke::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Smoke::draw()
{
	if (m_capture)
	{
		m_capture = false;
		captureSurface();
	}

	if (m_alpha > 0.f)
	{
		// draw the current surface

		setColorf(1.f, 1.f, 1.f, m_alpha);
		gxSetTexture(m_surface->getTexture());
		drawRect(0, 0, g_currentSurface->getWidth(), g_currentSurface->getHeight());
		gxSetTexture(0);
	}

	// apply flow map to current surface contents

	pushSurface(m_surface);
	{
		ScopedSurfaceBlock surfaceScope(m_surface);

		setBlend(BLEND_OPAQUE);
		setColor(colorWhite);

		Shader shader("smoke");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, true);
		shader.setTexture("flowmap", 1, 0, true, false);
		shader.setImmediate("flow_time", g_currentScene->m_time);
		ShaderBuffer buffer;
		SmokeData data;
		data.alpha = 1.f;
		data.strength = m_strength;
		data.darken = m_darken;
		data.darkenAlpha = m_darkenAlpha;
		data.mul = m_multiply;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("SmokeBlock", buffer);
		g_currentSurface->postprocess(shader);

		applyBlendMode();
	}
	popSurface();
}

void Effect_Smoke::handleSignal(const std::string & name)
{
	if (name == "capture")
	{
		m_capture = true;
	}

	if (m_capture)
	{
		m_capture = false;
		captureSurface();
	}
}

void Effect_Smoke::captureSurface()
{
	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	SceneLayer * layer = m_layer.empty() ? g_currentSceneLayer : g_currentScene->findLayerByName(m_layer.c_str());

	if (layer)
	{
		pushSurface(m_surface);
		{
			gxSetTexture(layer->m_surface->getTexture());
			drawRect(0, 0, m_surface->getWidth(), m_surface->getHeight());
			gxSetTexture(0);
		}
		popSurface();
	}

	applyBlendMode();
}

//

Effect_Beams::Effect_Beams(const char * name)
	: Effect(name)
	, m_alpha(1.f)
	, m_beamTime(0.f)
	, m_beamTimer(0.f)
	, m_beamTimerRcp(0.f)
	, m_beamSpeed(1.f)
	, m_beamSize1(0.f)
	, m_beamSize2(100.f)
	, m_beamOffset(0.f)
{
	is2DAbsolute = true;

	addVar("alpha", m_alpha);
	addVar("beam_time", m_beamTime);
	addVar("beam_speed", m_beamSpeed);
	addVar("beam_size1", m_beamSize1);
	addVar("beam_size2", m_beamSize2);
	addVar("beam_offset", m_beamOffset);
}

Effect_Beams::~Effect_Beams()
{
}

void Effect_Beams::tick(const float dt)
{
	m_beamTimer = m_beamTimer - dt;
	if (m_beamTimer < 0.f)
		m_beamTimer = 0.f;

	if (m_beamTimer == 0.f)
	{
		if (m_beamTimerRcp != 0.f)
		{
			// add a beam
			Beam b;
			//const float as = Calc::mPI * 3/4;
			const float as = Calc::mPI * 4/4;
			b.angle = random(-as/2, +as/2);
			b.thickness = random(3.f, 6.f);
			b.offsetX = random(-m_beamOffset, +m_beamOffset);
			b.offsetY = random(-m_beamOffset, +m_beamOffset);
			b.length2Speed = random(.8f, 1.f);
			m_beams.push_back(b);
		}

		if (m_beamTime > 0.f)
		{
			// next beam
			const float duration = lerp(m_beamTime/2.f, (float)m_beamTime, random(0.f, 1.f));
			m_beamTimer = duration;
			m_beamTimerRcp = 1.f / duration;
		}
		else
		{
			m_beamTimer = 0.f;
			m_beamTimerRcp = 0.f;
		}
	}

	for (auto & b : m_beams)
	{
		b.length += dt * m_beamSpeed * b.length1Speed;
		if (b.length > 1.f)
			b.length = 1.f;

		if (m_beamTimerRcp == 0.f)
		{
			b.length2 += dt * m_beamSpeed * b.length2Speed;
			if (b.length2 > 1.f)
				b.length2 = 1.f;
		}
	}
}

void Effect_Beams::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Beams::draw()
{
	if (m_alpha <= 0.f)
		return;

	const float size1 = m_beamSize1;
	const float size2 = m_beamSize2;

	setColorf(1, 1, 1, m_alpha);

	for (auto & b : m_beams)
	{
		if (b.length2 >= b.length)
			continue;

		const float length1 = lerp(size1, size2, b.length2);
		const float length2 = lerp(size1, size2, b.length);

		for (int i = -1; i <= +1; i += 2)
		{
			gxPushMatrix();
			{
				gxScalef(i, 1.f, 1.f);
				gxTranslatef(b.offsetX, b.offsetY, 0.f);
				gxRotatef(Calc::RadToDeg(b.angle), 0.f, 0.f, 1.f);
				drawRect(-b.thickness/2.f, length1, +b.thickness/2.f, length2);
			}
			gxPopMatrix();
		}
	}
}

void Effect_Beams::handleSignal(const std::string & name)
{
	if (String::StartsWith(name, "addline"))
	{
		std::vector<std::string> args;
		splitString(name, args, ',');

		if (args.size() < 2)
		{
			logError("missing segment parameters! %s", name.c_str());
		}
		else
		{
			Dictionary d;
			d.parse(args[1]);

			Beam b;
			b.angle = d.getFloat("angle", 0.f);
			b.thickness = d.getFloat("thickness", 1.f);
			b.length1Speed = d.getFloat("speed1", 1.f);
			b.length2Speed = d.getFloat("speed2", 1.f);
			m_beams.push_back(b);
		}
	}
}

//

Effect_FXAA::Effect_FXAA(const char * name)
	: Effect(name)
{
}

void Effect_FXAA::tick(const float dt)
{
}

void Effect_FXAA::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_FXAA::draw()
{
	setBlend(BLEND_OPAQUE);

	Shader shader("fxaa");
	setShader(shader);
	shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, true);
	shader.setImmediate("inverseVP", 1.f / (g_currentSurface->getWidth() / framework.minification), 1.f / (g_currentSurface->getHeight() / framework.minification));
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_Fireworks::Effect_Fireworks(const char * name)
	: Effect(name)
	, spawnValue(0.f)
	, spawnRate(1.f)
	, nextParticleIndex(0)
	, m_rootSpeed(160.f)
	, m_rootSpeedVar(540.f)
	, m_rootLife(1.f)
	, m_child1Speed(50.f)
	, m_child1SpeedVar(15.f)
	, m_child1Life(1.5f)
	, m_child2Speed(16.f)
	, m_child2SpeedVar(10.f)
	, m_child2Life(1.f)
{
	is2DAbsolute = true;

	addVar("spawn_rate",       spawnRate);
	addVar("root_speed",       m_rootSpeed);
	addVar("root_speed_var",   m_rootSpeedVar);
	addVar("root_life",        m_rootLife);
	addVar("child1_speed",     m_child1Speed);
	addVar("child1_speed_var", m_child1SpeedVar);
	addVar("child1_life",      m_child1Life);
	addVar("child2_speed",     m_child2Speed);
	addVar("child2_speed_var", m_child2SpeedVar);
	addVar("child2_life",      m_child2SpeedVar);

	memset(ps, 0, sizeof(ps));
}

Effect_Fireworks::P * Effect_Fireworks::nextParticle()
{
	P * p = &ps[nextParticleIndex];

	nextParticleIndex = (nextParticleIndex + 1) % PS;

	return p;
}

void Effect_Fireworks::tick(const float dt)
{
	if (spawnRate == 0.f)
	{
		spawnValue = 0.f;
	}
	else
	{
		spawnValue += dt;
		
		const float spawnInterval = 1.f / spawnRate;

		while (spawnValue >= spawnInterval)
		{
			spawnValue -= spawnInterval;

			P & p = *nextParticle();

			const float h = pow(random(0.f, 1.f), .5f);
			const float r = Calc::DegToRad(35);
			const float a = Calc::m2PI*3/4 + random(-r/2, +r/2);
			const float v = lerp(float(m_rootSpeed), float(m_rootSpeed + m_rootSpeedVar), h);

			p.type = kPT_Root;
			p.x = GFX_SX/2.f + random(-80.f, +80.f);
			p.y = GFX_SY;
			p.oldX = p.x;
			p.oldY = p.y;
			p.life = m_rootLife;
			p.lifeRcp = 1.f / p.life;
			p.vx = std::cos(a) * v;
			p.vy = std::sin(a) * v;
			///p.color = Color::fromHSL(.5f + random(0.f, .3f), .5f, .5f);
			p.color = Color::fromHSL(.5f + random(0.f, .3f) + g_currentScene->m_time * .5f, .5f, .5f);
		}
	}

	for (int i = 0; i < PS; ++i)
	{
		P & p = ps[i];

		if (p.life > 0.f)
		{
			p.life -= dt;

			p.x += p.vx * dt;
			p.y += p.vy * dt;

			p.vy += 40.f * dt;

			if (p.life <= 0.f)
			{
				p.life = 0.f;

				if (p.type == kPT_Root)
				{
					for (int i = 0; i < 200; ++i)
					{
						P & pc = *nextParticle();

						const float a = random(0.f, Calc::m2PI);
						const float v = random(float(m_child1Speed), float(m_child1Speed + m_child1SpeedVar));

						const float pv = .05f;

						pc.type = kPT_Child1;
						pc.x = p.x;
						pc.y = p.y;
						pc.oldX = pc.x;
						pc.oldY = pc.y;
						pc.life = m_child1Life;
						pc.lifeRcp = 1.f / pc.life;
						pc.vx = std::cos(a) * v + p.vx * pv;
						pc.vy = std::sin(a) * v + p.vy * pv;
						pc.color = p.color;

						P & pm = *nextParticle();
						pm = pc;
						pm.vx *= -1.f;
					}
				}

				if (p.type == kPT_Child1)
				{
					//if ((rand() % 20) == 0)
					if (false)
					{
						for (int i = 0; i < 10; ++i)
						{
							P & pc = *nextParticle();

							const float a = random(0.f, Calc::m2PI);
							const float v = random(float(m_child2Speed), float(m_child2Speed + m_child2SpeedVar));
							
							pc.type = kPT_Child2;
							pc.x = p.x;
							pc.y = p.y;
							pc.oldX = pc.x;
							pc.oldY = pc.y;
							pc.life = m_child2Life;
							pc.lifeRcp = 1.f / pc.life;
							pc.vx = std::cos(a) * v;
							pc.vy = std::sin(a) * v - 10.f;
							pc.color = p.color;

							P & pm = *nextParticle();
							pm = pc;
							pm.vx *= -1.f;
						}
					}
				}
			}
		}
	}
}

void Effect_Fireworks::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Fireworks::draw()
{
	pushLineSmooth(true);

	gxBegin(GX_LINES);
	{
		for (int i = 0; i < PS; ++i)
		{
			P & p = ps[i];

			if (p.life != 0.f)
			{
				gxColor4f(p.color.r, p.color.g, p.color.b, std::powf(p.life * p.lifeRcp, .25f));

				gxVertex2f(+p.oldX, p.oldY);
				gxVertex2f(+p.x,    p.y);

				p.oldX = p.x;
				p.oldY = p.y;
			}
		}
	}
	gxEnd();

	popLineSmooth();
}

//

Effect_Sparklies::Effect_Sparklies(const char * name)
	: Effect(name)
	, m_particleSystem(4096)
	, m_alpha(1.f)
{
	addVar("alpha", m_alpha);
}

void Effect_Sparklies::tick(const float dt)
{
	static float m_spawnTimer = 0.f;
	float m_spawnInterval = .005f;
	float m_life = 6.f;
	float m_size = 15.f;
	float m_speed = 10.f;
	
	m_particleSystem.tick(dt);

	if (m_spawnInterval > 0.f)
	{
		m_spawnTimer += dt;

		while (m_spawnTimer >= m_spawnInterval)
		{
			m_spawnTimer -= m_spawnInterval;

			int id;

			if (m_particleSystem.alloc(true, m_life, id))
			{
				const float speedAngle = random(0.f, Calc::m2PI);
				m_particleSystem.x[id] = random(0.f, (float)GFX_SX);
				m_particleSystem.y[id] = random(0.f, (float)GFX_SY);
				m_particleSystem.sx[id] = m_size;
				m_particleSystem.sy[id] = m_size;
				m_particleSystem.vx[id] = cosf(speedAngle) * m_speed;
				m_particleSystem.vy[id] = sinf(speedAngle) * m_speed;
			}
		}
	}
}

void Effect_Sparklies::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Sparklies::draw()
{
	std::string m_shader = "sparklies";
	std::string m_image = "track-intro/explosion_mask.png";

	{
		setBlend(BLEND_OPAQUE);
		Shader copyShader("copy");
		copyShader.setTexture("colormap", 0, g_currentSurface->getTexture(), false, true);
		g_currentSurface->postprocess(copyShader);
		applyBlendMode();
	}

	Shader shader(m_shader.c_str());
	setTextures(shader);
	setShader(shader);
	shader.setTexture("image", 5, Sprite(m_image.c_str()).getTexture(), true, true);

	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < m_particleSystem.numParticles; ++i)
		{
			if (m_particleSystem.alive[i])
			{
				const float value = m_particleSystem.life[i] * m_particleSystem.lifeRcp[i];
				const float x = m_particleSystem.x[i];
				const float y = m_particleSystem.y[i];

				gxColor4f(x, y, 1.f, (1.f - cosf(value * Calc::m2PI)) / 2.f * m_alpha);

				const float s = sinf(m_particleSystem.angle[i]);
				const float c = cosf(m_particleSystem.angle[i]);

				const float sx_2 = m_particleSystem.sx[i] * .5f;
				const float sy_2 = m_particleSystem.sy[i] * .5f;

				const float s_sx_2 = s * sx_2;
				const float s_sy_2 = s * sy_2;
				const float c_sx_2 = c * sx_2;
				const float c_sy_2 = c * sy_2;

				gxTexCoord2f(0.f, 0.f); gxVertex2f(x + (-c_sx_2 - s_sy_2), y + (+s_sx_2 - c_sy_2));
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x + (+c_sx_2 - s_sy_2), y + (-s_sx_2 - c_sy_2));
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x + (+c_sx_2 + s_sy_2), y + (-s_sx_2 + c_sy_2));
				gxTexCoord2f(0.f, 1.f); gxVertex2f(x + (-c_sx_2 + s_sy_2), y + (+s_sx_2 + c_sy_2));
			}
		}
	}
	gxEnd();

	clearShader();
}

//

Effect_Wobbly::WaterSim::WaterSim()
{
	memset(p, 0, sizeof(p));
	memset(v, 0, sizeof(v));
}

void Effect_Wobbly::WaterSim::tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds)
{
	const double vRetain = std::pow(vRetainPerSecond, dt);
	const double pRetain = std::pow(pRetainPerSecond, dt);
	
	for (int i = 0; i < kNumElems; ++i)
	{
		int i1, i2, i3;
		
		if (closedEnds)
		{
			i1 = i - 1 >= 0             ? i - 1 : i;
			i2 = i;
			i3 = i + 1 <= kNumElems - 1 ? i + 1 : i;
		}
		else
		{
			i1 = i - 1 >= 0             ? i - 1 : kNumElems - 1;
			i2 = i;
			i3 = i + 1 <= kNumElems - 1 ? i + 1 : 0;
		}
		
		const double p1 = p[i1];
		const double p2 = p[i2];
		const double p3 = p[i3];
		
		const double d1 = p1 - p2;
		const double d2 = p3 - p2;
		
		double a = 0.f;
		
		a += d1 * c;
		a += d2 * c;
		
		v[i] += a * dt;
	}
	
	for (int i = 0; i < kNumElems; ++i)
	{
		p[i] += v[i] * dt;
		
		p[i] *= pRetain;
		v[i] *= vRetain;
	}
}

static void applyWaterDrop(Effect_Wobbly::WaterSim & sim, const int spot, const int r, const double s)
{
	for (int i = -r; i <= +r; ++i)
	{
		const int x = (spot + i + sim.kNumElems) % sim.kNumElems;
		const double value = (1.0 + std::cos(i / double(r) * Calc::mPI)) / 2.0;
		
		sim.p[x] += value * s;
	}
}

Effect_Wobbly::WaterDrop::WaterDrop()
	: isAlive(false)
	, isApplying(false)
	, x(0.0)
	, y(0.0)
	, vx(0.0)
	, direction(0.0)
	, strength(0.0)
	, applyRadius(0.0)
	, applyTime(0.0)
	, applyTimeRcp(0.0)
	, fadeInTime(0.0)
	, fadeInTimeRcp(0.0)
{
}
		
void Effect_Wobbly::WaterDrop::tick(const double dt, const double stretch, WaterSim & sim)
{
	x += vx * dt;
	
	const double intersection = checkIntersection(sim, stretch, applyRadius, -1.0);
	
	if (intersection > -1.0)
		isApplying = true;
	
	fadeInTime -= dt;
	if (fadeInTime < 0.0)
		fadeInTime = 0.0;
	
	if (isApplying)
	{
		applyTime -= dt;
	}
	
	if (applyTime <= 0.0)
	{
		isAlive = false;
		applyTime = 0.0;
	}
	else if (isApplying)
	{
		const int yi = int(GFX_SY - 1 - y);
		
		//const double applyStrength = strength * (1.0 - std::abs(intersection));
		const double applyStrength = strength * applyTime * applyTimeRcp;
		
		applyWaterDrop(sim, yi, 200, applyStrength * direction * dt);
	}
}

double Effect_Wobbly::WaterDrop::toWaterP(const WaterSim & sim, const double x, const double stretch) const
{
	const double p = (x - GFX_SX/2) / stretch;
	
	return p;
}

double Effect_Wobbly::WaterDrop::checkIntersection(const WaterSim & sim, const double stretch, const double radius, const double bias) const
{
	const double p1 = x;
	
	const int yi = int(GFX_SY - 1 - y);
	const double p2 = GFX_SX/2 + sim.p[yi % sim.kNumElems] * stretch;
	
	const double pd = (p1 - p2) * direction;
	
	const double intersection = pd / radius + bias;
	
	return intersection < -1.0 ? -1.0 : intersection > +1.0 ? +1.0 : intersection;
}

Effect_Wobbly::Effect_Wobbly(const char * name, const char * shader)
	: Effect(name)
	, m_drop(0.f)
	, m_showDrops(1.f)
	, m_wobbliness(20000.f)
	, m_closedEnds(1.f)
	, m_stretch(1.f)
	, m_numIterations(100)
	, m_alpha(1.f)
	, m_shader()
	, m_waterSim(nullptr)
	, m_waterDrops()
	, elementsTexture(0)
{
	addVar("drop", m_drop);
	addVar("show_drops", m_showDrops);
	addVar("wobbliness", m_wobbliness);
	addVar("closed", m_closedEnds);
	addVar("stretch", m_stretch);
	addVar("num_iterations", m_numIterations);
	addVar("alpha", m_alpha);
	
	m_shader = shader;
	if (m_shader.empty())
		m_shader = "track-wobbly/fsfx_wobbly.ps";
	
	m_waterSim = new WaterSim();
	
	glGenTextures(1, &elementsTexture);
}

Effect_Wobbly::~Effect_Wobbly()
{
	glDeleteTextures(1, &elementsTexture);
	elementsTexture = 0;
	
	delete m_waterSim;
	m_waterSim = nullptr;
}

void Effect_Wobbly::tick(const float dt)
{
	fade.tick(dt);
	
	//
	
	const double vRetainPerSecond = 0.99 * (1.0 - fade.falloff);
	const double pRetainPerSecond = 0.99 * (1.0 - fade.falloff);
	
	const int numIterations = std::max(1, int(m_numIterations));
	
	const double dtSub = double(dt) / numIterations;
	
	static int spot = -1;
	
	if (keyboard.wentDown(SDLK_SPACE))
	{
		const int r = 200;
		const int v = WaterSim::kNumElems - r * 2;
		
		if (v > 0)
		{
			spot = r + (rand() % v);
			
			const double s = random(-1.f, +1.f);
			
			applyWaterDrop(*m_waterSim, spot, r, s);
			/*
			for (int i = -r; i <= +r; ++i)
			{
				const int x = spot + i;
				const double value = (1.0 + std::cos(i / double(r) * Calc::mPI)) / 2.0;
				
				if (x >= 0 && x < WaterSim::kNumElems)
					m_waterSim->p[x] += value * s;
			}
			*/
		}
	}
	
	const bool closedEnds = m_closedEnds != 0.f;
	
	for (int i = 0; i < numIterations; ++i)
	{
		m_waterSim->tick(dtSub, m_wobbliness, vRetainPerSecond, pRetainPerSecond, closedEnds);
		
		for (auto w = m_waterDrops.begin(); w != m_waterDrops.end(); )
		{
			auto & drop = *w;
			
			drop.tick(dtSub, m_stretch, *m_waterSim);
			
			if (drop.isAlive)
				++w;
			else
				w = m_waterDrops.erase(w);
		}
	}
}

void Effect_Wobbly::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Wobbly::draw()
{
	float * data = (float*)alloca(sizeof(float) * WaterSim::kNumElems * 2);
	
	for (int i = 0; i < WaterSim::kNumElems; ++i)
	{
		data[i * 2 + 0] = float(m_waterSim->p[i]);
		data[i * 2 + 1] = float(m_waterSim->v[i]);
	}
	
	glBindTexture(GL_TEXTURE_2D, elementsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, WaterSim::kNumElems, 1, 0, GL_RG, GL_FLOAT, data);
	checkErrorGL();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	checkErrorGL();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	checkErrorGL();

	glBindTexture(GL_TEXTURE_2D, 0);
	checkErrorGL();
	
	//
	
	SceneLayer * video1Layer = g_currentScene->findLayerByName("video1");
	SceneLayer * video2Layer = g_currentScene->findLayerByName("video2");
	
	const GLuint video1Texture = video1Layer ? video1Layer->m_surface->getTexture() : 0;
	const GLuint video2Texture = video2Layer ? video2Layer->m_surface->getTexture() : 0;
	
	Shader shader(m_shader.c_str(), "fsfx.vs", m_shader.c_str());
	setShader(shader);
	{
		//shader.setTexture("colormap", 0, g_currentSurface->getTexture());
		shader.setTexture("elements", 1, elementsTexture);
		shader.setTexture("video1", 2, video1Texture, true, true);
		shader.setTexture("video2", 3, video2Texture, true, true);
		shader.setImmediate("colormapSize", GFX_SX, GFX_SY);
		shader.setImmediate("stretch", m_stretch);
		shader.setImmediate("distort", .05f, .05f);
		shader.setImmediate("alpha", m_alpha);
		
		drawRect(0.f, 0.f, GFX_SX, GFX_SY);
	}
	clearShader();
	
	if (m_showDrops)
	{
		for (auto & drop : m_waterDrops)
		{
			//const double intersection = drop.checkIntersection(*m_waterSim, m_stretch, drop.applyRadius, -1.0);
			
			double opacity = 1.0;
			opacity *= drop.applyTime * drop.applyTimeRcp;
			opacity *= 1.0 - drop.fadeInTime * drop.fadeInTimeRcp;
			
			double radius = drop.applyRadius;
			radius *= Calc::Lerp(1.f, .7f, (cosf(drop.x / 40.f) + 1.f) / 2.f);
			radius *= 0.5;
			radius *= drop.applyTime * drop.applyTimeRcp;
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColorf(1.f, 1.f, 1.f, opacity);
				hqFillCircle(drop.x, drop.y, radius);
			}
			hqEnd();
		}
	}
}

void Effect_Wobbly::handleSignal(const std::string & name)
{
	Dictionary d;
	d.parse(name);
	
	const std::string action = d.getString("action", "");
	
	if (action == "drop")
	{
		const float y1 = d.getFloat("y1", 0.f);
		const float y2 = d.getFloat("y2", 0.f);
		const float position = d.getFloat("position", 100.f);
		const float speed = d.getFloat("speed", 0.f);
		const float strength = d.getFloat("strength", 0.f);
		const int direction = d.getInt("direction", 1) >= 0 ? +1 : -1;
		const int radius = d.getFloat("radius", 100.f);
		const float time = d.getFloat("time", 1.f);
		const float fadeInTime = d.getFloat("intime", 1.f);
		
		if (position > 0.f && speed > 0.f)
		{
			WaterDrop drop;
			drop.isAlive = true;
			drop.x = GFX_SX/2 - position * direction;
			drop.y = GFX_SY/2 + random(y1, y2);
			drop.vx = speed * direction;
			drop.strength = strength;
			drop.direction = direction;
			drop.applyRadius = radius;
			drop.applyTime = time;
			drop.applyTimeRcp = 1.f / time;
			drop.fadeInTime = fadeInTime;
			drop.fadeInTimeRcp = 1.f / fadeInTime;
			
			m_waterDrops.push_back(drop);
		}
	}
	
	if (action == "fade")
	{
		fade.falloff = d.getFloat("falloff", 0.f);
		fade.falloffD = d.getFloat("falloffD", 1.f);
	}
}

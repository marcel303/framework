#include "effect.h"
#include "tinyxml2.h"
#include "xml.h"

//

using namespace tinyxml2;

//

EffectInfosByName g_effectInfosByName;

bool EffectInfosByName::load(const char * filename)
{
	clear();

	bool result = true;

	tinyxml2::XMLDocument xmlDoc;

	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load %s", filename);

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

					// todo : check not already set

					(*this)[name] = effectInfo;
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

std::map<std::string, Effect*> g_effectsByName;

void registerEffect(const char * name, Effect * effect)
{
	Assert(g_effectsByName.count(name) == 0);

	g_effectsByName[name] = effect;
}

void unregisterEffect(Effect * effect)
{
	for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i)
	{
		if (i->second == effect)
		{
			g_effectsByName.erase(i);
			break;
		}
	}
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
	if (!m_effect->enabled)
		return;

	gxPushMatrix();
	{
		if (m_effect->is3D)
			gxMultMatrixf(m_effect->transform.m_v);
		else if (m_effect->is2D)
		{
			gxTranslatef(virtualToScreenX(m_effect->screenX), virtualToScreenY(m_effect->screenY), 0.f);
			gxScalef(m_effect->scaleX * m_effect->scale, m_effect->scaleY * m_effect->scale, 1.f);
		}

		switch (m_effect->blendMode)
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

		m_effect->draw();

		setBlend(BLEND_ADD);
	}
	gxPopMatrix();
}

//

Effect_Fsfx::Effect_Fsfx(const char * name, const char * shader, const std::vector<std::string> & images)
	: Effect(name)
	, m_alpha(1.f)
	, m_param1(0.f)
	, m_param2(0.f)
	, m_param3(0.f)
	, m_param4(0.f)
	, m_images(images)
	, m_textureArray(0)
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
	TweenFloatCollection::tick(dt);
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

	Shader shader(m_shader.c_str(), "fsfx.vs", m_shader.c_str());
	setShader(shader);
	setTextures(shader);
	ShaderBuffer buffer;
	FsfxData data;
	data.alpha = m_alpha;
	data.time = g_currentScene->m_time;
	data.param1 = m_param1;
	data.param2 = m_param2;
	data.param3 = m_param3;
	data.param4 = m_param4;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("FsfxBlock", buffer);
	shader.setTextureArray("textures", 3, m_textureArray, true, false);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

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
	data._time = g_currentScene->m_time;
	data._param1 = m_param1;
	data._param2 = m_param2;
	data._param3 = m_param3;
	data._param4 = m_param4;
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("FsfxBlock", buffer);
	shader.setTextureArray("textures", 3, m_textureArray, true, false);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
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
	TweenFloatCollection::tick(dt);

	for (size_t i = 0; i < m_blocks.size(); ++i)
	{
		Block & b = m_blocks[i];

		b.value += b.speed * dt;
	}

	const int numBlocks = (int)m_numBlocks;

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

	gxBegin(GL_QUADS);
	{
		for (size_t i = 0; i < m_blocks.size(); ++i)
		{
			const Block & b = m_blocks[i];

			const float t = (std::sin(b.value * 2.f * M_PI) + 1.f) / 2.f;
			const float scale = Calc::Lerp(1.f, 1.5f, t);
			const float alpha = Calc::Lerp(.2f, .5f, t) * m_alpha;

			const float sx = scale * b.size / 2.f;
			const float sy = scale * b.size / 2.f;

			gxColor4f(0.f, 0.f, 1.f, alpha); gxVertex2f(b.x - sx, b.y - sy);
			gxColor4f(1.f, 0.f, 1.f, alpha); gxVertex2f(b.x + sx, b.y - sy);
			gxColor4f(1.f, 1.f, 1.f, alpha); gxVertex2f(b.x + sx, b.y + sy);
			gxColor4f(0.f, 1.f, 1.f, alpha); gxVertex2f(b.x - sx, b.y + sy);
		}
	}
	gxEnd();
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
	TweenFloatCollection::tick(dt);

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

			if (m_lines[i].x + m_lines[i].sx < 0.f && m_lines[i].speedX < 0.f ||
				m_lines[i].y + m_lines[i].sy < 0.f && m_lines[i].speedY < 0.f ||
				m_lines[i].x > GFX_SX && m_lines[i].speedX > 0.f ||
				m_lines[i].y > GFX_SY && m_lines[i].speedY > 0.f)
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
	addVar("min_size", m_minSize);
	addVar("max_size", m_maxSize);
	addVar("size_pow", m_sizePow);
	addVar("top_alpha", m_topAlpha);
	addVar("bottom_alpha", m_bottomAlpha);
}

void Effect_Bars::initializeBars()
{
	float x = 0.f;

	while (x < GFX_SX)
	{
		const float t = std::powf(random(0.f, 1.f), m_sizePow);

		Bar bar;
		bar.size = Calc::Lerp(m_minSize, m_maxSize, t);
		bar.color = rand() % 4;

		m_bars.push_back(bar);

		x += bar.size;
	}
}

void Effect_Bars::shuffleBar()
{
	if (m_bars.size() >= 2)
	{
		for (;;)
		{
			const int index1 = rand() % m_bars.size();
			const int index2 = rand() % m_bars.size();

			if (index1 != index2)
			{
				std::swap(m_bars[index1], m_bars[index2]);

				break;
			}
		}
	}
}

void Effect_Bars::tick(const float dt)
{
	if (m_bars.empty())
		initializeBars();

	TweenFloatCollection::tick(dt);
	
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

		shuffleBar();
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

	float x = 0.f;

	gxBegin(GL_QUADS);
	{
		for (size_t i = 0; i < m_bars.size(); ++i)
		{
			const static Color colors[4] =
			{
				Color(0,0,0),
				Color(255,255,255),
				Color(255, 255,0),
				Color(0, 0, 255)
			};

			const Color & color = m_bars[i].color < 4 ? colors[m_bars[i].color] : colorWhite;

			gxColor4f(color.r, color.g, color.b, m_topAlpha * m_alpha);
			gxVertex2f(x, 0.f);
			gxVertex2f(x + m_bars[i].size, 0.f);
			gxColor4f(color.r, color.g, color.b, m_bottomAlpha * m_alpha);
			gxVertex2f(x + m_bars[i].size, GFX_SY);
			gxVertex2f(x, GFX_SY);

			x += m_bars[i].size;
		}
	}
	gxEnd();
}

#include "effect.h"
#include "Parse.h"
#include "tinyxml2.h"
#include "xml.h"

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
		}

		m_effect->applyBlendMode();

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
	buffer.setData(&data, sizeof(data));
	shader.setBuffer("FsfxBlock", buffer);
	shader.setTextureArray("textures", 4, m_textureArray, true, false);
	g_currentSurface->postprocess(shader);

	setBlend(BLEND_ADD);
}

//

Effect_Picture::Effect_Picture(const char * name, const char * filename, bool centered)
	: Effect(name)
	, m_alpha(1.f)
	, m_angle(0.f)
	, m_centered(true)
{
	is2D = true;

	addVar("alpha", m_alpha);
	addVar("angle", m_angle);

	m_filename = filename;
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

	gxPushMatrix();
	{
		Sprite sprite(m_filename.c_str());

		const int sx = sprite.getWidth();
		const int sy = sprite.getHeight();
		const float scaleX = SCREEN_SX / float(sx);
		const float scaleY = SCREEN_SY / float(sy);
		const float scale = Calc::Min(scaleX, scaleY);

		gxRotatef(m_angle, 0.f, 0.f, 1.f);
		gxScalef(scale, scale, 1.f);
		if (m_centered)
			gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

		setColorf(1.f, 1.f, 1.f, m_alpha);
		sprite.drawEx(0.f, 0.f, 0.f, 1.f, 1.f, false, FILTER_LINEAR);
	}
	gxPopMatrix();
}

//

Effect_Video::Effect_Video(const char * name, const char * filename, const char * shader, const bool centered, const bool play)
	: Effect(name)
	, m_alpha(1.f)
	, m_angle(0.f)
	, m_centered(true)
	, m_speed(1.f)
	, m_hideWhenDone(0.f)
{
	is2D = true;

	addVar("alpha", m_alpha);
	addVar("angle", m_angle);
	addVar("speed", m_speed);
	addVar("hide_when_done", m_hideWhenDone);

	m_filename = filename;
	m_shader = shader;
	m_centered = centered;

	if (play)
	{
		handleSignal("start");
	}
}

void Effect_Video::tick(const float dt)
{
	if (m_mediaPlayer.isActive(m_mediaPlayer.context))
	{
		m_mediaPlayer.speed = m_speed;

		if (m_hideWhenDone && m_mediaPlayer.presentedLastFrame(m_mediaPlayer.context))
		{
			m_mediaPlayer.close();
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

	if (m_mediaPlayer.getTexture())
	{
		gxPushMatrix();
		{
			const int sx = m_mediaPlayer.sx;
			const int sy = m_mediaPlayer.sy;
			const float scaleX = SCREEN_SX / float(sx);
			const float scaleY = SCREEN_SY / float(sy);
			const float scale = Calc::Min(scaleX, scaleY);

			gxRotatef(m_angle, 0.f, 0.f, 1.f);
			gxScalef(scale, scale, 1.f);
			if (m_centered)
				gxTranslatef(-sx / 2.f, -sy / 2.f, 0.f);

			if (!m_shader.empty())
			{
				Shader shader(m_shader.c_str());
				setShader(shader);
				shader.setTexture("colormap", 0, m_mediaPlayer.getTexture(), true, true);

				{
					setColorf(1.f, 1.f, 1.f, m_alpha);
					drawRect(0, m_mediaPlayer.sy, m_mediaPlayer.sx, 0);
				}
				clearShader();
			}
			else
			{
				setColorf(1.f, 1.f, 1.f, m_alpha);
				m_mediaPlayer.draw();
			}
		}
		gxPopMatrix();
	}
}

void Effect_Video::handleSignal(const std::string & name)
{
	if (name == "start")
	{
		if (m_mediaPlayer.isActive(m_mediaPlayer.context))
		{
			m_mediaPlayer.close();
		}

		if (!m_mediaPlayer.open(m_filename.c_str()))
		{
			logWarning("failed to open %s", m_filename.c_str());
		}
		else
		{
			m_startTime = g_currentScene->m_time;
		}
	}
}

void Effect_Video::syncTime(const float time)
{
	if (m_mediaPlayer.isActive(m_mediaPlayer.context))
	{
		const float videoTime = time - m_startTime;

		if (videoTime >= 0.f)
		{
			m_mediaPlayer.seek(videoTime);
		}
	}
}

//

Effect_Blit::Effect_Blit(const char * name, const char * layer)
	: Effect(name)
	, m_alpha(1.f)
	, m_angle(0.f)
	, m_centered(0.f)
	, m_absolute(1.f)
	, m_srcX(-1.f)
	, m_srcY(-1.f)
	, m_srcSx(0.f)
	, m_srcSy(0.f)
	, m_layer(layer)
{
	//is2D = true;

	addVar("alpha", m_alpha);
	addVar("angle", m_angle);
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

	out_u =       srcX / SCREEN_SX;
	out_v = 1.f - srcY / SCREEN_SY;

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

		gxRotatef(m_angle, 0.f, 0.f, 1.f);
		if (m_centered)
			gxTranslatef(-m_srcSx / 2.f, -m_srcSy / 2.f, 0.f);

		gxColor4f(1.f, 1.f, 1.f, m_alpha);
		gxSetTexture(layer->m_surface->getTexture());
		{
			gxBegin(GL_QUADS);
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

		const int oldValue = int(b.value);

		b.value += b.speed * dt;

		const int newValue = int(b.value);

		if (oldValue != newValue)
			b.picture = (newValue & 1) ? 0 : Sprite("hue.png").getTexture();
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

	for (size_t i = 0; i < m_blocks.size(); ++i)
	{
		const Block & b = m_blocks[i];

		gxSetTexture(b.picture);

		gxBegin(GL_QUADS);
		{
			const float t = (std::sin(b.value * 2.f * M_PI) + 1.f) / 2.f;
			const float scale = Calc::Lerp(1.f, 1.5f, t);
			const float alpha = Calc::Lerp(.2f, .5f, t) * m_alpha;

			const float sx = scale * b.size / 2.f;
			const float sy = scale * b.size / 2.f;

		#if 0
			const static Color colors[4] =
			{
				Color::fromHex("ffffff"),
				Color::fromHex("ffffff"),
				Color::fromHex("ffffff"),
				Color::fromHex("ffffff")
			};
		#else
			const static Color colors[4] =
			{
				Color::fromHex("000000"),
				//Color::fromHex("ffffff"),
				Color::fromHex("9c9c9c"),
				Color::fromHex("1e1e1e"),
				Color::fromHex("575556")
			};
		#endif

			gxTexCoord2f(0.f, 0.f); gxColor4f(colors[0].r, colors[0].g, colors[0].b, alpha); gxVertex2f(b.x - sx, b.y - sy);
			gxTexCoord2f(1.f, 0.f); gxColor4f(colors[1].r, colors[1].g, colors[1].b, alpha); gxVertex2f(b.x + sx, b.y - sy);
			gxTexCoord2f(1.f, 1.f); gxColor4f(colors[2].r, colors[2].g, colors[2].b, alpha); gxVertex2f(b.x + sx, b.y + sy);
			gxTexCoord2f(0.f, 1.f); gxColor4f(colors[3].r, colors[3].g, colors[3].b, alpha); gxVertex2f(b.x - sx, b.y + sy);
		}
		gxEnd();

		gxSetTexture(0);
	}
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

#if 1
const static Color s_colorBarColors[] =
{
	Color::fromHex("000000"),
	Color::fromHex("ffffff"),
	//Color::fromHex("000000"),
	//Color::fromHex("808080"),
	Color::fromHex("9c9c9c"),
	Color::fromHex("1e1e1e"),
	Color::fromHex("575556"),
};
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
const static int s_numColorBarColors = sizeof(s_colorBarColors) / sizeof(s_colorBarColors[0]);

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

			bar.color = rand() % s_numColorBarColors;

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

	gxBegin(GL_QUADS);
	{
		for (int layer = 0; layer < kNumLayers; ++layer)
		{
			float x = 0.f;

			for (size_t i = 0; i < m_bars[layer].size(); ++i)
			{
				const Bar & b = m_bars[layer][i];

				const Color & color = s_colorBarColors[b.color];

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
{
	is2D = true;

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
			for (auto & n : s.nodes)
			{
				n.bezierNode.m_Position += n.positionSpeed * dt;
				n.bezierNode.m_Tangent[0] += n.tangentSpeed * dt;
			}

			++si;
		}
	}
}

void Effect_Bezier::draw(DrawableList & list)
{
	new (list) EffectDrawable(this);
}

void Effect_Bezier::draw()
{
	for (auto & s : segments)
	{
		const float a = s.time * s.timeRcp;
		const float t = 1.f - a;

		Color baseColor;
		colorCurve.sample(t, baseColor);

		std::vector<BezierNode> nodes;
		nodes.resize(s.nodes.size());

		const int numLoops = s.numGhosts * 2 + 1;

		for (int g = 0; g < numLoops; ++g)
		{
			const float jt = numLoops >= 2 ? (g / float(numLoops - 1)) : .5f;
			const float ja = lerp(-1.f, +1.f, jt);
			const float jb = ja * t * s.ghostSize;

			for (size_t i = 0; i < s.nodes.size(); ++i)
			{
				nodes[i] = s.nodes[i].bezierNode;

				nodes[i].m_Position += s.nodes[i].positionSpeed * jb;
				nodes[i].m_Tangent[0] += s.nodes[i].tangentSpeed * jb;

				nodes[i].m_Tangent[1] = - nodes[i].m_Tangent[0];
			}

			BezierPath path;
			path.ConstructFromNodes(&nodes.front(), nodes.size());

			glEnable(GL_LINE_SMOOTH);
			glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

			gxBegin(GL_LINE_STRIP);
			{
				const Color color = baseColor.mulRGBA(s.color);

				gxColor4f(color.r, color.g, color.b, color.a * a);

				for (float i = 0.f; i <= nodes.size(); i += 0.01f)
				{
					const Vec2F p = path.Interpolate(i);

					gxVertex2f(p[0], p[1]);
				}
			}
			gxEnd();

			glDisable(GL_LINE_SMOOTH);
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

			const float posSpeed = d.getFloat("pos_speed", 0.f);
			const float tanSpeed = d.getFloat("tan_speed", 0.f);
			const float posVary = d.getFloat("pos_vary", 0.f);
			const float tanVary = d.getFloat("tan_vary", 0.f);

			generateSegment(Vec2F(x1, y1), Vec2F(x2, y2), posSpeed, tanSpeed, posVary, tanVary, numSegments, numGhosts, ghostSize, color, lifeTime);
		}
	}
	else if (name == "part1")
	{
		for (auto & s : segments)
			s.time = 1.f / s.timeRcp;
	}
	else if (name == "part2")
	{
	}
	else if (name == "part3")
	{
	}
}

void Effect_Bezier::generateSegment(const Vec2F & p1, const Vec2F & p2, const float posSpeed, const float tanSpeed, const float posVary, const float tanVary, const int numNodes, const int numGhosts, const float ghostSize, const Color & color, const float duration)
{
	const float mS = GFX_SY / 10.f;

	Segment segment;

	segment.color = color;
	segment.time = duration;
	segment.timeRcp = 1.f / duration;
	segment.numGhosts = numGhosts;
	segment.ghostSize = ghostSize;

	for (int i = 0; i < numNodes; ++i)
	{
		const float v = i / float(numNodes - 1.f);
		
		Vec2F p = lerp(p1, p2, v);

		if (i != 0 && i != numNodes - 1)
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

		segment.nodes.push_back(node);
	}

	segments.push_back(segment);
}

//

Effect_Smoke::Effect_Smoke(const char * name)
	: Effect(name)
	, surface(nullptr)
	, capture(false)
{
	surface = new Surface(GFX_SX, GFX_SY);
	surface->clear();
}

Effect_Smoke::~Effect_Smoke()
{
	delete surface;
	surface = nullptr;
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
	if (capture)
	{
		capture = false;
		captureSurface();
	}

	// draw the current surface

	setColor(colorWhite);
	gxSetTexture(surface->getTexture());
	drawRect(0, 0, g_currentSurface->getWidth(), g_currentSurface->getHeight());
	gxSetTexture(0);

	// apply flow map to current surface contents

	pushSurface(surface);
	{
		ScopedSurfaceBlock surfaceScope(surface);

		setBlend(BLEND_OPAQUE);
		setColor(colorWhite);

		// todo : effect parameters

		Shader shader("flowmap");
		setShader(shader);
		shader.setTexture("colormap", 0, g_currentSurface->getTexture(), true, false);
		shader.setTexture("flowmap", 1, 0, true, false);
		shader.setImmediate("flow_time", g_currentScene->m_time);
		ShaderBuffer buffer;
		FlowmapData data;
		data.alpha = .9f;
		data.strength = 2.f;
		data.darken = .01f;
		buffer.setData(&data, sizeof(data));
		shader.setBuffer("FlowmapBlock", buffer);
		g_currentSurface->postprocess(shader);

		setBlend(BLEND_ADD);
	}
	popSurface();
}

void Effect_Smoke::handleSignal(const std::string & name)
{
	if (name == "capture")
	{
		capture = true;
	}
}

void Effect_Smoke::captureSurface()
{
	setBlend(BLEND_OPAQUE);
	setColor(colorWhite);

	pushSurface(surface);
	{
		gxSetTexture(g_currentSceneLayer->m_surface->getTexture());
		drawRect(0, 0, surface->getWidth(), surface->getHeight());
		gxSetTexture(0);
	}
	popSurface();

	applyBlendMode();
}

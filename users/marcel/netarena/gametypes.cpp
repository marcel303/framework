#include "arena.h"
#include "Calc.h"
#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "gametypes.h"
#include "tinyxml2.h"
#include "StreamReader.h"
#include "StreamWriter.h"

//#pragma optimize("", off)

using namespace tinyxml2;

Vec2 getTransitionOffset(TransitionType type, int sxPixels, int syPixels, float transitionAmount)
{
	switch (type)
	{
	case kTransitionType_None:
		return Vec2(0.f, 0.f);
	case kTransitionType_SlideFromLeft:
		return Vec2(-sxPixels * transitionAmount, 0.f);
	case kTransitionType_SlideFromRight:
		return Vec2(+sxPixels * transitionAmount, 0.f);
	case kTransitionType_SlideFromTop:
		return Vec2(0.f, -syPixels * transitionAmount);
	case kTransitionType_SlideFromBottom:
		return Vec2(0.f, +syPixels * transitionAmount);
	default:
		Assert(false);
		break;
	}

	return Vec2(0.f, 0.f);
}

void TransitionInfo::setup(TransitionType type, int sxPixels, int syPixels, float time, float curve)
{
	m_type = type;
	m_sxPixels = sxPixels;
	m_syPixels = syPixels;
	m_time = time;
	m_curve = curve;
}

void TransitionInfo::parse(const class Dictionary & d, int sxPixels, int syPixels)
{
	m_sxPixels = sxPixels;
	m_syPixels = syPixels;

	const std::string type = d.getString("transition_type", "");

	if (type == "none")
		m_type = kTransitionType_None;
	else if (type == "left")
		m_type = kTransitionType_SlideFromLeft;
	else if (type == "right")
		m_type = kTransitionType_SlideFromRight;
	else if (type == "top")
		m_type = kTransitionType_SlideFromTop;
	else if (type == "bottom")
		m_type = kTransitionType_SlideFromBottom;
	else
	{
		logError("unknown transition type: %s", type.c_str());
		m_type = kTransitionType_None;
	}

	m_time = d.getFloat("transition_time", 1.f);
	m_curve = d.getFloat("transition_curve", 1.f);
}

Vec2 TransitionInfo::eval(float transitionTime) const
{
	if (isActiveAtTime(transitionTime))
	{
		const float transitionProgress = std::powf(1.f - transitionTime / m_time, m_curve);

		return getTransitionOffset(m_type, m_sxPixels, m_syPixels, transitionProgress);
	}
	else
	{
		return Vec2(0.f, 0.f);
	}
}

//

const char * g_gameModeNames[kGameMode_COUNT] =
{
	"Lobby",
	"Death Match",
	"Token Hunt",
	"Coin Collector"
};

const char * g_gameStateNames[kGameState_COUNT] =
{
	"mainMenus",
	"connecting",
	"onlineMenus",
	"newGame",
	"roundComplete"
};

const char * g_playerSpecialNames[kPlayerSpecial_COUNT] =
{
	"none",
	"RocketPunch",
	"DoubleSidedMelee",
	"StompAttack",
	"Shield",
	"Invisibility",
	"Jetpack",
	"Zweihander",
	"Axe",
	"Pipebomb",
	"Grapple"
};

Curve defaultCurve(0.f, 1.f);
Curve pipebombBlastCurve;
Curve grenadeBlastCurve;
Curve jetpackAnalogCurveX;
Curve jetpackAnalogCurveY;
Curve gravityWellFalloffCurve;

//

void PlayerInput::gather(bool useKeyboard, int gamepadIndex, bool monkeyMode)
{
	*this = PlayerInput();

	if (useKeyboard)
	{
		if (keyboard.isDown(SDLK_LEFT))
		{
			buttons |= INPUT_BUTTON_LEFT;
			analogX -= PlayerInput::AnalogRange;
		}
		if (keyboard.isDown(SDLK_RIGHT))
		{
			buttons |= INPUT_BUTTON_RIGHT;
			analogX += PlayerInput::AnalogRange;
		}
		if (keyboard.isDown(SDLK_UP))
		{
			buttons |= INPUT_BUTTON_UP;
			analogY -= PlayerInput::AnalogRange;
		}
		if (keyboard.isDown(SDLK_DOWN))
		{
			buttons |= INPUT_BUTTON_DOWN;
			analogY += PlayerInput::AnalogRange;
		}

		if (keyboard.isDown(SDLK_a) || keyboard.isDown(SDLK_SPACE) || keyboard.isDown(SDLK_RETURN))
			buttons |= INPUT_BUTTON_A;
		if (keyboard.isDown(SDLK_s))
			buttons |= INPUT_BUTTON_B;
		if (keyboard.isDown(SDLK_z))
			buttons |= INPUT_BUTTON_X;
		if (keyboard.isDown(SDLK_x))
			buttons |= INPUT_BUTTON_Y;

		if (keyboard.isDown(SDLK_d))
			buttons |= INPUT_BUTTON_START;
		if (keyboard.isDown(SDLK_c))
			buttons |= INPUT_BUTTON_L1;
		if (keyboard.isDown(SDLK_v))
			buttons |= INPUT_BUTTON_R1;

		if (keyboard.isDown(SDLK_h))
			buttons |= INPUT_BUTTON_DPAD_LEFT;
		if (keyboard.isDown(SDLK_k))
			buttons |= INPUT_BUTTON_DPAD_RIGHT;
		if (keyboard.isDown(SDLK_u))
			buttons |= INPUT_BUTTON_DPAD_UP;
		if (keyboard.isDown(SDLK_j))
			buttons |= INPUT_BUTTON_DPAD_DOWN;
	}

	if (gamepadIndex >= 0 && gamepadIndex < MAX_GAMEPAD && gamepad[gamepadIndex].isConnected)
	{
		const Gamepad & g = gamepad[gamepadIndex];

		if (g.isDown(GAMEPAD_START))
			gamepad[gamepadIndex].vibrate(.5f, .5f);

		if (g.isDown(DPAD_LEFT) || g.getAnalog(0, ANALOG_X) < -0.4f)
			buttons |= INPUT_BUTTON_LEFT;
		if (g.isDown(DPAD_RIGHT) || g.getAnalog(0, ANALOG_X) > +0.4f)
			buttons |= INPUT_BUTTON_RIGHT;
		if (g.isDown(DPAD_UP) || g.getAnalog(0, ANALOG_Y) < -0.4f)
			buttons |= INPUT_BUTTON_UP;
		if (g.isDown(DPAD_DOWN) || g.getAnalog(0, ANALOG_Y) > +0.4f)
			buttons |= INPUT_BUTTON_DOWN;

		if (g.isDown(GAMEPAD_A))
			buttons |= INPUT_BUTTON_A;
		if (g.isDown(GAMEPAD_B))
			buttons |= INPUT_BUTTON_B;
		if (g.isDown(GAMEPAD_X))
			buttons |= INPUT_BUTTON_X;
		if (g.isDown(GAMEPAD_Y))
			buttons |= INPUT_BUTTON_Y;

		if (g.isDown(GAMEPAD_START))
			buttons |= INPUT_BUTTON_START;
		if (g.isDown(GAMEPAD_L1))
			buttons |= INPUT_BUTTON_L1;
		if (g.isDown(GAMEPAD_R1))
			buttons |= INPUT_BUTTON_R1;

		if (g.isDown(DPAD_LEFT))
			buttons |= INPUT_BUTTON_DPAD_LEFT;
		if (g.isDown(DPAD_RIGHT))
			buttons |= INPUT_BUTTON_DPAD_RIGHT;
		if (g.isDown(DPAD_UP))
			buttons |= INPUT_BUTTON_DPAD_UP;
		if (g.isDown(DPAD_DOWN))
			buttons |= INPUT_BUTTON_DPAD_DOWN;

		analogX = Calc::Mid((int)(analogX + g.getAnalog(0, ANALOG_X) * PlayerInput::AnalogRange), -PlayerInput::AnalogRange, +PlayerInput::AnalogRange);
		analogY = Calc::Mid((int)(analogY + g.getAnalog(0, ANALOG_Y) * PlayerInput::AnalogRange), -PlayerInput::AnalogRange, +PlayerInput::AnalogRange);
	}

	if (monkeyMode)
	{
		if ((rand() % 2) == 0)
		{
			buttons |= INPUT_BUTTON_LEFT;
			analogX -= PlayerInput::AnalogRange;
		}
		if ((rand() % 10) == 0)
		{
			buttons |= INPUT_BUTTON_RIGHT;
			analogX += PlayerInput::AnalogRange;
		}
		if ((rand() % 10) == 0)
		{
			buttons |= INPUT_BUTTON_UP;
			analogY -= PlayerInput::AnalogRange;
		}
		if ((rand() % 10) == 0)
		{
			buttons |= INPUT_BUTTON_DOWN;
			analogY += PlayerInput::AnalogRange;
		}

		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_A;
		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_B;
		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_X;
		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_Y;

		if ((rand() % 60) == 0)
			buttons |= INPUT_BUTTON_START;
		if ((rand() % 120) == 0)
			buttons |= INPUT_BUTTON_L1;
		if ((rand() % 120) == 0)
			buttons |= INPUT_BUTTON_R1;

		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_LEFT;
		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_RIGHT;
		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_UP;
		if ((rand() % 600) == 0)
			buttons |= INPUT_BUTTON_DPAD_DOWN;
	}
}

//

bool parsePickupType(char c, PickupType & type)
{
	switch (c)
	{
	case 'w': type = kPickupType_Gun; break;
	case 'g': type = kPickupType_Nade; break;
	case 's': type = kPickupType_Shield; break;
	case 'i': type = kPickupType_Ice; break;
	case 'b': type = kPickupType_Bubble; break;
	case 't': type = kPickupType_TimeDilation; break;
	default:
		return false;
	}

	return true;
}

bool parsePickupType(const char * s, PickupType & type)
{
	if (strlen(s) != 1)
		return false;
	else
		return parsePickupType(s[0], type);
}

//

const CollisionShape & CollisionShape::operator=(const CollisionBox & box)
{
	Vec2 p1(box.min[0], box.min[1]);
	Vec2 p2(box.max[0], box.min[1]);
	Vec2 p3(box.max[0], box.max[1]);
	Vec2 p4(box.min[0], box.max[1]);

	set(p1, p2, p3, p4);

	return *this;
}

void CollisionShape::fixup()
{
	Assert(numPoints > 0);
	Vec2 center;
	for (int i = 0; i < numPoints; ++i)
		center += points[i];
	center /= numPoints;
	const Vec2 n = getSegmentNormal(0);
	const float d1 = n * points[0];
	const float d2 = n * center - d1;
	if (d2 > 0.f)
	{
		for (int i = 0; i < numPoints/2; ++i)
			std::swap(points[i], points[numPoints - 1 - i]);
	}
}

void CollisionShape::translate(float x, float y)
{
	for (int i = 0; i < numPoints; ++i)
	{
		points[i][0] += x;
		points[i][1] += y;
	}
}

void CollisionShape::getMinMax(Vec2 & min, Vec2 & max) const
{
	switch (type)
	{
	case kType_Poly:
		Assert(numPoints != 0);

		min = points[0];
		max = points[0];

		for (int i = 1; i < numPoints; ++i)
		{
			min = min.Min(points[i]);
			max = max.Max(points[i]);
		}
		break;

	case kType_Circle:
		min = points[0] - Vec2(radius, radius);
		max = points[0] + Vec2(radius, radius);
		break;

	default:
		Assert(false);
		break;
	}
}

float CollisionShape::projectedMax(Vec2Arg n) const
{
	float result = 0.f;

	switch (type)
	{
	case kType_Poly:
		Assert(numPoints != 0);

		result = n * points[0];

		for (int i = 1; i < numPoints; ++i)
		{
			const float dot = n * points[i];

			if (dot < result)
				result = dot;
		}
		break;

	case kType_Circle:
		result = n * points[0] - radius * n.CalcSize();
		break;
	}

	return result;
}

Vec2 CollisionShape::getSegmentNormal(int idx) const
{
	// todo : these could be precomputed

	const Vec2 & p1 = points[(idx + 0) % numPoints];
	const Vec2 & p2 = points[(idx + 1) % numPoints];

	const Vec2 d = (p2 - p1).CalcNormalized();
	const Vec2 n(+d[1], -d[0]);

	return n;
}

bool CollisionShape::intersects(const CollisionShape & other) const
{
	if (type == kType_Circle)
	{
		if (other.type == kType_Circle)
		{
			const Vec2 delta = points[0] - other.points[0];
			const float distanceSq = delta.CalcSizeSq();
			const float radiusSum = radius + other.radius;
			const float radiusSumSq = radiusSum * radiusSum;
			return distanceSq <= radiusSumSq;
		}
		else if (other.type == kType_Poly)
		{
			// todo!
			return false;
		}
		else
		{
			Assert(false);
			return false;
		}
	}
	else if (type == kType_Poly)
	{
		if (other.type == kType_Circle)
		{
			return other.intersects(*this);
		}
		else if (other.type == kType_Poly)
		{
			// perform SAT test

			// evaluate edges from the first shape

			for (int i = 0; i < numPoints; ++i)
			{
				const Vec2 & p = points[i];
				const Vec2 pn = getSegmentNormal(i);
				const float pd = pn * p;
				const float d = other.projectedMax(pn) - pd;

				if (d >= 0.f)
					return false;
			}

			// evaluate edges from the second shape

			for (int i = 0; i < other.numPoints; ++i)
			{
				const Vec2 & p = other.points[i];
				const Vec2 pn = other.getSegmentNormal(i);
				const float pd = pn * p;
				const float d = projectedMax(pn) - pd;

				if (d >= 0.f)
					return false;
			}

			return true;
		}
		else
		{
			Assert(false);
			return false;
		}
	}
	else
	{
		Assert(false);
		return false;
	}
}

bool CollisionShape::checkCollision(const CollisionShape & other, Vec2Arg delta, float & contactDistance, Vec2 & contactNormal) const
{
	float minDistance = -FLT_MAX;
	Vec2 minNormal;
	bool flip = false;
	bool any = false;

	// perform SAT test

	// evaluate edges from the first shape

	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 & p = points[i];
		const Vec2 pn = getSegmentNormal(i);
		const float pd = pn * p;
		const float d = other.projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
		else
		{
			// is this a normal in the direction we're checking for?

			if (pn * delta == 0.f)
				continue;

			any = true;

			if (d > minDistance)
			{
				minDistance = d;
				minNormal = pn;
			}
		}
	}

	// evaluate edges from the second shape

	for (int i = 0; i < other.numPoints; ++i)
	{
		const Vec2 & p = other.points[i];
		const Vec2 pn = other.getSegmentNormal(i);
		const float pd = pn * p;
		const float d = projectedMax(pn) - pd;

		if (d >= 0.f)
			return false;
		else
		{
			// is this a normal in the direction we're checking for?

			if (pn * delta == 0.f)
				continue;

			any = true;

			if (d > minDistance)
			{
				minDistance = d;
				minNormal = -pn;

				flip = true;
			}
		}
	}

	if (flip && false)
	{
		minDistance = -minDistance;
		minNormal = -minNormal;
	}

	contactDistance = minDistance;
	contactNormal = minNormal;

	return any;
}

void CollisionShape::debugDraw(bool drawNormals) const
{
	for (int i = 0; i < numPoints; ++i)
	{
		const Vec2 p1 = points[(i + 0) % numPoints];
		const Vec2 p2 = points[(i + 1) % numPoints];
		drawLine(p1[0], p1[1], p2[0], p2[1]);

		if (drawNormals)
		{
			const Vec2 m = (p1 + p2) / 2.f;
			const Vec2 n = getSegmentNormal(i);
			drawLine(m[0], m[1], m[0] + n[0] * BLOCK_SX/2.f, m[1] + n[1] * BLOCK_SX/2.f);
		}
	}
}

//

Curve::Curve()
{
	memset(this, 0, sizeof(Curve));
}

Curve::Curve(float min, float max)
{
	makeLinear(min, max);
}

void Curve::makeLinear(float v1, float v2)
{
	for (int i = 0; i < 32; ++i)
	{
		const float u = i / 31.f;
		const float v = 1.f - u;
		value[i] = v1 * v + v2 * u;
	}
}

float Curve::eval(float t) const
{
	t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
	const float s = t * 31.f;
	const int index1 = (int)s;
	const int index2 = (index1 + 1) & 31;
	const float u = s - index1;
	const float v = 1.f - u;
	const float v1 = value[index1];
	const float v2 = value[index2];
	return v1 * v + v2 * u;
}

//

void UserSettings::save(class StreamWriter & writer)
{
	XMLPrinter p;

	p.OpenElement("settings");
	{
		p.OpenElement("audio");
		{
			p.PushAttribute("musicEnabled", audio.musicEnabled);
			p.PushAttribute("musicVolume", audio.musicVolume);
			p.PushAttribute("soundEnabled", audio.soundEnabled);
			p.PushAttribute("soundVolume", audio.soundVolume);
			p.PushAttribute("announcerEnabled", audio.announcerEnabled);
			p.PushAttribute("announcerVolume", audio.announcerVolume);
		}
		p.CloseElement();

		p.OpenElement("display");
		{
			p.PushAttribute("fullscreen", display.fullscreen);
			p.PushAttribute("exclusiveFullscreen", display.exclusiveFullscreen);
			p.PushAttribute("exclusiveFullscreenVsync", display.exclusiveFullscreenVsync);
			p.PushAttribute("exclusiveFullscreenHz", display.exclusiveFullscreenHz);
			p.PushAttribute("fullscreenSx", display.fullscreenSx);
			p.PushAttribute("fullscreenSy", display.fullscreenSy);
			p.PushAttribute("windowedSx", display.windowedSx);
			p.PushAttribute("windowedSy", display.windowedSy);
		}
		p.CloseElement();

		p.OpenElement("graphics");
		{
			p.PushAttribute("brightness", graphics.brightness);
		}
		p.CloseElement();

		p.OpenElement("language");
		{
			p.PushAttribute("locale", language.locale.c_str());
		}
		p.CloseElement();

		p.OpenElement("effects");
		{
			p.PushAttribute("screenShakeStrength", effects.screenShakeStrength);
		}
		p.CloseElement();

		p.OpenElement("characters");
		{
			for (int i = 0; i < MAX_CHARACTERS; ++i)
			{
				p.OpenElement("character");
				{
					p.PushAttribute("index", i);
					p.PushAttribute("emblem", chars[i].emblem);
					p.PushAttribute("skin", chars[i].skin);
				}
				p.CloseElement();
			}
		}
		p.CloseElement();
	}
	p.CloseElement();

	writer.WriteText_Binary(p.CStr());
	//printf("XML:\n%s\n", p.CStr());
}

static const char * stringAttrib(const XMLElement * elem, const char * name, const char * defaultValue)
{
	if (elem->Attribute(name))
		return elem->Attribute(name);
	else
		return defaultValue;
}

static bool boolAttrib(const XMLElement * elem, const char * name, bool defaultValue)
{
	if (elem->Attribute(name))
		return elem->BoolAttribute(name);
	else
		return defaultValue;
}

static int intAttrib(const XMLElement * elem, const char * name, int defaultValue)
{
	if (elem->Attribute(name))
		return elem->IntAttribute(name);
	else
		return defaultValue;
}

static float floatAttrib(const XMLElement * elem, const char * name, float defaultValue)
{
	if (elem->Attribute(name))
		return elem->FloatAttribute(name);
	else
		return defaultValue;
}

void UserSettings::load(class StreamReader & reader)
{
	*this = UserSettings();

	const std::string text = reader.ReadText_Binary();

	XMLDocument doc;
	doc.Parse(text.c_str(), text.length());

	auto settingsXml = doc.FirstChildElement("settings");
	if (settingsXml)
	{
		auto audioXml = settingsXml->FirstChildElement("audio");
		if (audioXml)
		{
			audio.musicEnabled = boolAttrib(audioXml, "musicEnabled", audio.musicEnabled);
			audio.musicVolume = floatAttrib(audioXml, "musicVolume", audio.musicVolume);
			audio.soundEnabled = boolAttrib(audioXml, "soundEnabled", audio.soundEnabled);
			audio.soundVolume = floatAttrib(audioXml, "soundVolume", audio.soundVolume);
			audio.announcerEnabled = boolAttrib(audioXml, "announcerEnabled", audio.announcerEnabled);
			audio.announcerVolume = floatAttrib(audioXml, "announcerVolume", audio.announcerVolume);
		}

		auto displayXml = settingsXml->FirstChildElement("display");
		if (displayXml)
		{
			display.fullscreen = boolAttrib(displayXml, "fullscreen", display.fullscreen);
			display.exclusiveFullscreen = boolAttrib(displayXml, "exclusiveFullscreen", display.exclusiveFullscreen);
			display.exclusiveFullscreenVsync = boolAttrib(displayXml, "exclusiveFullscreenVsync", display.exclusiveFullscreenVsync);
			display.exclusiveFullscreenHz = intAttrib(displayXml, "exclusiveFullscreenHz", display.exclusiveFullscreenHz);
			display.fullscreenSx = intAttrib(displayXml, "fullscreenSx", display.fullscreenSx);
			display.fullscreenSy = intAttrib(displayXml, "fullscreenSy", display.fullscreenSy);
			display.windowedSx = intAttrib(displayXml, "windowedSx", display.windowedSx);
			display.windowedSy = intAttrib(displayXml, "windowedSy", display.windowedSy);
		}

		auto graphicsXml = settingsXml->FirstChildElement("graphics");
		if (graphicsXml)
		{
			graphics.brightness = floatAttrib(graphicsXml, "brightness", graphics.brightness);
		}

		auto languageXml = settingsXml->FirstChildElement("language");
		if (languageXml)
		{
			language.locale = stringAttrib(languageXml, "locale", language.locale.c_str());
		}

		auto effectsXml = settingsXml->FirstChildElement("effects");
		if (effectsXml)
		{
			effects.screenShakeStrength = floatAttrib(effectsXml, "screenShakeStrength", effects.screenShakeStrength);
		}

		auto charactersXml = settingsXml->FirstChildElement("characters");
		if (charactersXml)
		{
			for (auto characterXml = charactersXml->FirstChildElement("character"); characterXml; characterXml = characterXml->NextSiblingElement())
			{
				const int index = intAttrib(characterXml, "index", -1);
				if (index >= 0 && index < MAX_CHARACTERS)
				{
					auto & character = chars[index];

					character.emblem = intAttrib(characterXml, "emblem", character.emblem);
					character.skin = intAttrib(characterXml, "skin", character.skin);
				}
			}
		}
	}
}

#include "audiofft.h"
#include "drawable.h"
#include "effect.h"
#include "framework.h"
#include "Parse.h"
#include "scene.h" 
#include "tinyxml2.h"
#include "types.h"
#include "xml.h"
#include <stdarg.h>

using namespace tinyxml2;

// from internal.h
void splitString(const std::string & str, std::vector<std::string> & result, char c);

// from main.cpp
bool getSceneFileContents(const std::string & filename, Array<uint8_t> *& out_bytes);

//

extern int GFX_SX;
extern int GFX_SY;
extern int GFX_SX_SCALED;
extern int GFX_SY_SCALED;

extern Config config;

#if ENABLE_LEAPMOTION
extern LeapState g_leapState;
#endif

//

Scene * g_currentScene = nullptr;
SceneLayer * g_currentSceneLayer = nullptr;
Surface * g_currentSurface = nullptr;

SceneSurfacePool * g_sceneSurfacePool = nullptr;

//

SceneEffect::SceneEffect()
	: m_effect(nullptr)
	, m_strength(1.f)
{
}

SceneEffect::~SceneEffect()
{
	m_name.clear();

	delete m_effect;
	m_effect = nullptr;

	m_strength = 0.f;
}

bool SceneEffect::load(const XMLElement * xmlEffect)
{
	const auto t1_attributes = loadtimer();

	static int nonameCount = 0;

	m_name = stringAttrib(xmlEffect, "name", "");

	if (m_name.empty())
	{
		nonameCount++;

		m_name = String::FormatC("noname_%d", nonameCount);
	}

	const std::string type = stringAttrib(xmlEffect, "type", "");

	const auto t2_type = loadtimer();

	std::string typeName = type;

	Effect * effect = nullptr;

	if (type == "boxes")
	{
		const bool screenSpace = boolAttrib(xmlEffect, "screen_space", true);
		const std::string shader = stringAttrib(xmlEffect, "shader", "");

		effect = new Effect_Boxes(m_name.c_str(), screenSpace, shader.c_str());
	}
	else if (type == "fsfx")
	{
		const std::string shader = stringAttrib(xmlEffect, "shader", "");
		const std::string image = stringAttrib(xmlEffect, "image", "");
		const std::string imageFiles = stringAttrib(xmlEffect, "images", "");
		std::vector<std::string> images;
		splitString(imageFiles, images, ',');
		const std::string colorsStr = stringAttrib(xmlEffect, "colors", "");
		std::vector<std::string> colorsHex;
		splitString(colorsStr, colorsHex, ',');
		std::vector<Color> colors;
		for (auto & hex : colorsHex)
			colors.push_back(Color::fromHex(hex.c_str()));

		if (shader.empty())
		{
			logWarning("shader not set. skipping effect");
		}
		else
		{
			effect = new Effect_Fsfx(m_name.c_str(), shader.c_str(), image.c_str(), images, colors);

			typeName = shader;
		}
	}
	else if (type == "rain")
	{
		const int numRaindrops = intAttrib(xmlEffect, "num_raindrops", 0);

		if (numRaindrops == 0)
		{
			logWarning("num_raindrops is 0. skipping effect");
		}
		else
		{
			effect = new Effect_Rain(m_name.c_str(), numRaindrops);
		}
	}
	else if (type == "stars")
	{
		const int numStars = intAttrib(xmlEffect, "num_stars", 0);

		if (numStars == 0)
		{
			logWarning("num_stars is 0. skipping effect");
		}
		else
		{
			effect = new Effect_StarCluster(m_name.c_str(), numStars);
		}
	}
	else if (type == "flowmap")
	{
		const std::string map = stringAttrib(xmlEffect, "map", "");

		if (map.empty())
		{
			logWarning("map not set. skipping effect");
		}
		else
		{
			effect = new Effect_Flowmap(m_name.c_str(), map.c_str());
		}
	}
	else if (type == "luminance")
	{
		effect = new Effect_Luminance(m_name.c_str());
	}
	else if (type == "colorlut2d")
	{
		const std::string lut = stringAttrib(xmlEffect, "file", "");

		if (lut.empty())
		{
			logWarning("lut not set. skipping effect");
		}
		else
		{
			effect = new Effect_ColorLut2D(m_name.c_str(), lut.c_str());
		}
	}
	else if (type == "vignette")
	{
		effect = new Effect_Vignette(m_name.c_str());
	}
#if ENABLE_VIDEO
	else if (type == "video")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");
		const std::string shader = stringAttrib(xmlEffect, "shader", "");
		const bool yuv = boolAttrib(xmlEffect, "yuv", false);
		const bool centered = boolAttrib(xmlEffect, "centered", true);
		const bool play = boolAttrib(xmlEffect, "play", false);

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_Video(m_name.c_str(), file.c_str(), shader.c_str(), yuv, centered, play);
		}
	}
	else if (type == "videoloop")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");
		const std::string shader = stringAttrib(xmlEffect, "shader", "");
		const bool yuv = boolAttrib(xmlEffect, "yuv", false);
		const bool centered = boolAttrib(xmlEffect, "centered", true);
		const bool play = boolAttrib(xmlEffect, "play", false);

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_VideoLoop(m_name.c_str(), file.c_str(), shader.c_str(), yuv, centered, play);
		}
	}
#endif
	else if (type == "picture")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");
		const std::string file2 = stringAttrib(xmlEffect, "file2", "");
		const std::string shader = stringAttrib(xmlEffect, "shader", "");
		const bool centered = boolAttrib(xmlEffect, "centered", true);

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_Picture(m_name.c_str(), file.c_str(), file2.c_str(), shader.c_str(), centered);
		}
	}
	else if (type == "circles")
	{
		effect= new Effect_Clockwork(m_name.c_str());
	}
	else if (type == "draw_picture")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_DrawPicture(m_name.c_str(), file.c_str());
		}
	}
	else if (type == "blit")
	{
		const std::string layer = stringAttrib(xmlEffect, "layer", "");

		effect = new Effect_Blit(m_name.c_str(), layer.c_str());
	}
	else if (type == "blocks")
	{
		effect = new Effect_Blocks(m_name.c_str());
	}
	else if (type == "lines")
	{
		const int numLines = intAttrib(xmlEffect, "num_lines", 1);

		effect = new Effect_Lines(m_name.c_str(), numLines);
	}
	else if (type == "bars")
	{
		effect = new Effect_Bars(m_name.c_str());
	}
	else if (type == "text")
	{
		const std::string colorText = stringAttrib(xmlEffect, "color", "000000");
		const std::string font = stringAttrib(xmlEffect, "font", "calibri.ttf");
		const int fontSize = intAttrib(xmlEffect, "font_size", 12);
		const std::string text = stringAttrib(xmlEffect, "text", "???");

		effect = new Effect_Text(m_name.c_str(), Color::fromHex(colorText.c_str()), font.c_str(), fontSize, text.c_str());
	}
	else if (type == "bezier")
	{
		const std::string colors = stringAttrib(xmlEffect, "colors", "");

		effect = new Effect_Bezier(m_name.c_str(), colors.c_str());
	}
	else if (type == "smoke")
	{
		const char * layer = stringAttrib(xmlEffect, "layer", "");

		effect = new Effect_Smoke(m_name.c_str(), layer);
	}
	else if (type == "beams")
	{
		effect = new Effect_Beams(m_name.c_str());
	}
	else if (type == "fxaa")
	{
		effect = new Effect_FXAA(m_name.c_str());
	}
	else if (type == "fireworks")
	{
		effect = new Effect_Fireworks(m_name.c_str());
	}
	else if (type == "sparklies")
	{
		effect = new Effect_Sparklies(m_name.c_str());
	}
	else if (type == "wobbly")
	{
		const char * shader = stringAttrib(xmlEffect, "shader", "");
		
		effect = new Effect_Wobbly(m_name.c_str(), shader);
	}
	else
	{
		logError("unknown effect type: %s", type.c_str());
	}

	const auto t3_typeAttributes = loadtimer();

	if (effect != nullptr)
	{
		effect->typeName = typeName;

		for (const XMLAttribute * xmlAttrib = xmlEffect->FirstAttribute(); xmlAttrib; xmlAttrib = xmlAttrib->Next())
		{
			TweenFloat * var = effect->getVar(xmlAttrib->Name());

			if (var != nullptr)
			{
				float value;

				if (xmlAttrib->QueryFloatValue(&value) == XML_SUCCESS)
				{
					*var = value;
				}
			}
		}

		effect->visible = boolAttrib(xmlEffect, "visible", true);

		effect->blendMode = parseBlendMode(stringAttrib(xmlEffect, "blend", "add"));

		m_effect = effect;

		const auto t4_end = loadtimer();

		logLoadtime(t2_type           - t1_attributes,     "SceneEffect::load::attributes");
		logLoadtime(t3_typeAttributes - t2_type,           "SceneEffect::load::type(%s)", typeName.c_str());
		logLoadtime(t4_end            - t3_typeAttributes, "SceneEffect::load::typeAttributes(%s)", typeName.c_str());

		return true;
	}
	else
	{
		return false;
	}
}

//

SceneLayer::SceneLayer(Scene * scene)
	: m_scene(scene)
	, m_blendMode(kBlendMode_Add)
	, m_autoClear(true)
	, m_clearColor(0.f, 0.f, 0.f, 0.f)
	, m_copyPreviousLayer(false)
	, m_copyPreviousLayerAlpha(1.f)
	, m_visible(1.f)
	, m_opacity(1.f)
	, m_surface(nullptr)
	, m_debugEnabled(true)
{
	addVar("copy_alpha", m_copyPreviousLayerAlpha);
	addVar("visible", m_visible);
	addVar("opacity", m_opacity);

	const auto t1 = loadtimer();
	m_surface = g_sceneSurfacePool->alloc();
	const auto t2 = loadtimer();
	logLoadtime(t2 - t1, "SceneLayer::surface");
}

SceneLayer::~SceneLayer()
{
	const auto t1_effects = loadtimer();

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		delete effect;
		effect = nullptr;
	}

	m_effects.clear();

	const auto t2_surface = loadtimer();

	g_sceneSurfacePool->free(m_surface);
	m_surface = nullptr;

	const auto t3_end = loadtimer();

	logLoadtime(t2_surface - t1_effects, "~SceneLayer::effects");
	logLoadtime(t3_end     - t2_surface, "~SceneLayer::surface");
}

bool SceneLayer::load(const XMLElement * xmlLayer)
{
	const auto t1_attributes = loadtimer();

	bool enabled = boolAttrib(xmlLayer, "enabled", true);

	m_name = stringAttrib(xmlLayer, "name", "");

	std::string blend = stringAttrib(xmlLayer, "blend", "add");

	m_blendMode = parseBlendMode(blend);
	m_autoClear = boolAttrib(xmlLayer, "auto_clear", true);
	m_copyPreviousLayer = boolAttrib(xmlLayer, "copy", false);
	m_copyPreviousLayerAlpha = floatAttrib(xmlLayer, "copy_alpha", 1.f);
	m_opacity = floatAttrib(xmlLayer, "opacity", 1.f);
	m_visible = floatAttrib(xmlLayer, "visible", 1.f);

	const std::string clearColor = stringAttrib(xmlLayer, "clear_color", "");

	if (!clearColor.empty())
		m_clearColor = Color::fromHex(clearColor.c_str());

	const auto t2_surfaceClear = loadtimer();

	m_surface->clearf(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);

	//

	const auto t3_effects = loadtimer();

	for (const XMLElement * xmlEffect = xmlLayer->FirstChildElement("effect"); xmlEffect; xmlEffect = xmlEffect->NextSiblingElement("effect"))
	{
		const bool enabled = boolAttrib(xmlEffect, "enabled", true);

		if (!enabled)
			continue;

		SceneEffect * effect = new SceneEffect();

		if (!effect->load(xmlEffect))
		{
			delete effect;
			effect = nullptr;
		}
		else
		{
			m_effects.push_back(effect);
		}
	}

	const auto t4_end = loadtimer();

	logLoadtime(t2_surfaceClear - t1_attributes, "SceneLayer::load::attributes");
	logLoadtime(t3_effects - t2_surfaceClear, "SceneLayer::load::surfaceClear");
	logLoadtime(t4_end - t3_effects, "SceneLayer::load::effects");

	return enabled;
}

void SceneLayer::tick(const float dt)
{
	ScopedSceneLayerBlock block(this);

	TweenFloatCollection::tick(dt);

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		effect->m_effect->tickBase(dt);
	}
}

void SceneLayer::draw(DrawableList & list)
{
	struct SceneLayerDrawable : Drawable
	{
		SceneLayer * m_layer;

		SceneLayerDrawable(float z, SceneLayer * layer)
			: Drawable(z)
			, m_layer(layer)
		{
		}

		virtual void draw() override
		{
			m_layer->draw();
		}
	};

	if (m_debugEnabled)
	{
		new (list) SceneLayerDrawable(0.f, this);
	}
}

void SceneLayer::draw()
{
	ScopedSceneBlock sceneBlock(m_scene);
	ScopedSceneLayerBlock sceneLayerBlock(this);

	DrawableList drawableList;

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		effect->m_effect->draw(drawableList);
	}

	drawableList.sort();

	// render the effects

	if (m_copyPreviousLayer)
	{
		if (g_currentSurface)
		{
			if (m_copyPreviousLayerAlpha > 0.f)
			{
				pushSurface(m_surface);
				{
					if (m_copyPreviousLayerAlpha == 1.f)
						setBlend(BLEND_OPAQUE);
					else
						setBlend(BLEND_ALPHA);
					gxSetTexture(g_currentSurface->getTexture());
					gxColor4f(1.f, 1.f, 1.f, m_copyPreviousLayerAlpha);
					drawRect(0, 0, m_surface->getWidth(), m_surface->getHeight());
					gxSetTexture(0);
				}
				popSurface();
			}

			//g_currentSurface->blitTo(m_surface);
		}
		else
		{
			m_surface->clearf(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
		}
	}
	else if (m_autoClear)
	{
		m_surface->clearf(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
	}

	pushSurface(m_surface);
	{
		ScopedSurfaceBlock surfaceBlock(m_surface);

		setColor(colorWhite);
		setBlend(BLEND_ADD);

		drawableList.draw();
	}
	popSurface();

	if (m_visible > 0.f)
	{
		// compose

		gxSetTexture(m_surface->getTexture());
		{
			Shader alphaTestShader("layer_compose_alphatest");

			switch (m_blendMode)
			{
			case kBlendMode_Add:
				setBlend(BLEND_ADD_OPAQUE);
				setColorf(1.f, 1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_Subtract:
				setBlend(BLEND_SUBTRACT);
				setColorf(1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_Alpha:
				setBlend(BLEND_ALPHA);
				setColorf(1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_PremultipliedAlpha:
				setBlend(BLEND_PREMULTIPLIED_ALPHA);
				setColorf(m_opacity, m_opacity, m_opacity, m_opacity);
				break;
			case kBlendMode_Opaque:
				setBlend(BLEND_OPAQUE);
				setColorf(1.f, 1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_AlphaTest:
				setBlend(BLEND_OPAQUE);
				alphaTestShader.setImmediate("alphaRef", 0.f);
				alphaTestShader.setTexture("srcColormap", 0, m_surface->getTexture(), false, true);
				alphaTestShader.setTexture("dstColormap", 0, g_currentSurface->getTexture(), false, true);
				setShader(alphaTestShader);
				checkErrorGL();
				break;
			case kBlendMode_Multiply:
				setBlend(BLEND_MUL);
				break;
			default:
				Assert(false);
				break;
			}

			drawRect(0, 0, GFX_SX, GFX_SY);

			setBlend(BLEND_ADD);

			clearShader();
		}
		gxSetTexture(0);
	}
}

//

static EaseType parseEaseType(const std::string & easeType)
{
	EaseType result = kEaseType_Linear;

	if (easeType == "linear")
		result = kEaseType_Linear;
	else if (easeType == "pow_in")
		result = kEaseType_PowIn;
	else if (easeType == "pow_out")
		result = kEaseType_PowOut;
	else if (easeType == "sine_in")
		result = kEaseType_SineIn;
	else if (easeType == "sine_out")
		result = kEaseType_SineOut;
	else if (easeType == "sine_inout")
		result = kEaseType_SineInOut;
	else if (easeType == "back_in")
		result = kEaseType_BackIn;
	else if (easeType == "back_out")
		result = kEaseType_BackOut;
	else if (easeType == "bounce_in")
		result = kEaseType_BounceIn;
	else if (easeType == "bounce_out")
		result = kEaseType_BounceOut;
	else if (easeType == "bounce_inout")
		result = kEaseType_BounceInOut;
	else
		logError("unknown ease type: %s", easeType.c_str());

	return result;
}

SceneAction::SceneAction()
	: m_type(kActionType_None)
{
}

bool SceneAction::load(const XMLElement * xmlAction)
{
	const bool enabled = boolAttrib(xmlAction, "enabled", true);

	if (!enabled)
		return false;

	const std::string type = xmlAction->Name();

	if (type == "tween")
	{
		m_type = kActionType_Tween;

		const std::string layer = stringAttrib(xmlAction, "layer", "");
		const std::string effect = stringAttrib(xmlAction, "effect", "");

		if (!layer.empty() + !effect.empty() > 1)
		{
			logError("more than one target type set on effect action!");
			return false;
		}

		if (!layer.empty())
		{
			m_tween.m_targetType = kTweenTargetType_Layer;
			m_tween.m_targetName = layer;
		}
		else if (!effect.empty())
		{
			m_tween.m_targetType = kTweenTargetType_Effect;
			m_tween.m_targetName = effect;
		}

		m_tween.m_replaceTweens = boolAttrib(xmlAction, "replace", false);
		m_tween.m_addTweens = boolAttrib(xmlAction, "add", false);

		//

		Tween::TweenSet tweenSet;

		tweenSet.m_tweenTime = 0.f;
		tweenSet.m_easeType = kEaseType_Linear;
		tweenSet.m_easeParam = 0.f;

		tweenSet.m_preDelay = floatAttrib(xmlAction, "pre_delay", 0.f);
		tweenSet.m_postDelay = floatAttrib(xmlAction, "post_delay", 0.f);

		for (const XMLAttribute * xmlAttrib = xmlAction->FirstAttribute(); xmlAttrib; xmlAttrib = xmlAttrib->Next())
		{
			if (!strcmp(xmlAttrib->Name(), "enabled") ||
				!strcmp(xmlAttrib->Name(), "replace") ||
				!strcmp(xmlAttrib->Name(), "add") ||
				!strcmp(xmlAttrib->Name(), "pre_delay") ||
				!strcmp(xmlAttrib->Name(), "post_delay"))
			{
			}
			else if (!strcmp(xmlAttrib->Name(), "effect") || !strcmp(xmlAttrib->Name(), "layer"))
			{
			}
			else if (!strcmp(xmlAttrib->Name(), "time"))
			{
				const float time = xmlAttrib->FloatValue();

				tweenSet.m_tweenTime = time;

				m_tween.m_tweens.push_back(tweenSet);

				tweenSet = Tween::TweenSet();
			}
			else if (!strcmp(xmlAttrib->Name(), "ease"))
			{
				tweenSet.m_easeType = parseEaseType(xmlAttrib->Value());
			}
			else if (!strcmp(xmlAttrib->Name(), "ease_param"))
			{
				tweenSet.m_easeParam = xmlAttrib->FloatValue();
			}
			else
			{
				// must be a variable

				Tween::TweenSet::TweenVar var;

				var.m_varName = xmlAttrib->Name();
				var.m_tweenTo = xmlAttrib->FloatValue();
				var.m_preDelay = 0.f;

				for (const Tween::TweenSet & tweenSet : m_tween.m_tweens)
				{
					bool found = false;

					for (const Tween::TweenSet::TweenVar & tweenVar : tweenSet.m_vars)
						if (tweenVar.m_varName == var.m_varName)
							found = true;

					if (found)
						var.m_preDelay = 0.f;
					else
						var.m_preDelay += tweenSet.m_tweenTime;
				}

				tweenSet.m_vars.push_back(var);
			}
		}

		return true;
	}
	else if (type == "signal")
	{
		m_type = kActionType_Signal;

		const std::string effect = stringAttrib(xmlAction, "effect", "");
		const std::string event = stringAttrib(xmlAction, "event", "");
		const std::string message = stringAttrib(xmlAction, "message", "");

		if ((effect.empty() && event.empty()) || message.empty())
		{
			logError("effect or message not set!");
			return false;
		}

		m_signal.m_targetName = effect;
		m_signal.m_eventName = event;
		m_signal.m_message = message;

		return true;
	}
	else
	{
		logError("unknown event action: %s", type.c_str());
		return false;
	}
}

//

SceneEvent::SceneEvent()
	: m_enabled(true)
	, m_oscId(-1)
{
}

SceneEvent::~SceneEvent()
{
	m_name.clear();
	m_oscId = -1;

	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		delete action;
		action = nullptr;
	}

	m_actions.clear();
}

void SceneEvent::execute(Scene & scene)
{
	if (!m_enabled)
		return;

	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		switch (action->m_type)
		{
		case SceneAction::kActionType_Tween:
		{
			TweenFloatCollection * varCollection = nullptr;

			switch (action->m_tween.m_targetType)
			{
			case SceneAction::kTweenTargetType_Layer:
			{
				for (auto l = scene.m_layers.begin(); l != scene.m_layers.end(); ++l)
				{
					SceneLayer * layer = *l;

					if (layer->m_name == action->m_tween.m_targetName)
					{
						varCollection = layer;
					}
				}
			}
			break;

			case SceneAction::kTweenTargetType_Effect:
			{
				SceneEffect * effect = scene.findEffectByName(action->m_tween.m_targetName.c_str(), nullptr);

				if (effect)
				{
					varCollection = effect->m_effect;
				}
			}
			break;

			default:
				Assert(false);
				break;
			}

			if (varCollection == 0)
			{
				logWarning("couldn't find tween target by name. name=%s", action->m_tween.m_targetName.c_str());
			}
			else
			{
				const SceneAction::Tween & tween = action->m_tween;

				for (const SceneAction::Tween::TweenSet & tweenSet : tween.m_tweens)
				{
					for (const SceneAction::Tween::TweenSet::TweenVar & tweenVar : tweenSet.m_vars)
					{
						TweenFloat * var = varCollection->getVar(tweenVar.m_varName.c_str());

						if (var == 0)
						{
							logWarning("couldn't find tween value by name. name=%s", tweenVar.m_varName.c_str());
						}
						else
						{
							if (tween.m_replaceTweens)
							{
								var->clear();
							}

							const float preDelay = tweenSet.m_preDelay + tweenVar.m_preDelay;

							if (preDelay > 0.f)
							{
								var->to(var->getFinalValue(), preDelay, kEaseType_Linear, 0.f);
							}

							float to =  tweenVar.m_tweenTo;

							if (tween.m_addTweens)
							{
								to += var->getFinalValue();
							}

							var->to(
								to,
								tweenSet.m_tweenTime,
								tweenSet.m_easeType,
								tweenSet.m_easeParam);

							if (tweenSet.m_postDelay > 0.f)
							{
								var->to(var->getFinalValue(), tweenSet.m_postDelay, kEaseType_Linear, 0.f);
							}
						}
					}
				}
			}
		}
		break;

		case SceneAction::kActionType_Signal:
		{
			SceneLayer * effectLayer;

			if (!action->m_signal.m_targetName.empty())
			{
				SceneEffect * effect = scene.findEffectByName(action->m_signal.m_targetName.c_str(), &effectLayer);

				if (effect)
				{
					ScopedSceneBlock sceneBlock(&scene);
					ScopedSceneLayerBlock layerBlock(effectLayer);

					effect->m_effect->handleSignal(action->m_signal.m_message);
				}
			}

			if (!action->m_signal.m_eventName.empty())
			{
				SceneEvent * event = scene.findEventByName(action->m_signal.m_eventName.c_str());

				if (event)
				{
					event->handleSignal(action->m_signal.m_message);
				}
			}
		}
		break;

		default:
			Assert(false);
			break;
		}
	}
}

void SceneEvent::handleSignal(const std::string & message)
{
	if (message == "enable")
		m_enabled = true;
	if (message == "disable")
		m_enabled = false;
}

void SceneEvent::load(const XMLElement * xmlEvent)
{
	m_enabled = boolAttrib(xmlEvent, "enabled", true);
	m_name = stringAttrib(xmlEvent, "name", "");
	m_oscId = intAttrib(xmlEvent, "osc_id", -1);

	for (const XMLElement * xmlAction = xmlEvent->FirstChildElement(); xmlAction; xmlAction = xmlAction->NextSiblingElement())
	{
		SceneAction * action = new SceneAction();

		if (!action->load(xmlAction))
		{
			delete action;
			action = nullptr;
		}
		else
		{
			m_actions.push_back(action);
		}
	}
}

//

Scene::Scene()
	: m_fftFade(.5f)
	, m_time(0.f)
	, m_varTime(0.f)
	, m_varTimeStep(0.f)
	, m_varPalmX(0.f)
	, m_varPalmY(0.f)
	, m_varPalmZ(0.f)
	, m_varPcmVolume(0.f)
{
	addVar("fft_fade", m_fftFade);
	addVar("time", m_varTime);
	addVar("time_step", m_varTimeStep);
	addVar("palm_x", m_varPalmX);
	addVar("palm_y", m_varPalmY);
	addVar("palm_z", m_varPalmZ);
	addVar("pcm_volume", m_varPcmVolume);
}

Scene::~Scene()
{
	clear();
}

void Scene::tick(const float dt)
{
	ScopedSceneBlock block(this);

	// process global variables

	m_varTime = m_time;
	m_varTimeStep = m_varTimeStep + dt;

#if ENABLE_LEAPMOTION
	m_varPalmX = g_leapState.palmX;
	m_varPalmY = g_leapState.palmY;
	m_varPalmZ = g_leapState.palmZ;
#endif

	m_varPcmVolume = g_pcmVolume;

	// process tween variables

	TweenFloatCollection::tick(dt);

	// process layers

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->tick(dt);
	}

	// process FFT input

	for (int i = 0; i < kMaxFftBuckets; ++i)
	{
		FftBucket & b = m_fftBuckets[i];

		if (b.isActive)
		{
			const int begin = std::max(b.rangeBegin, 0);
			const int end = std::min(b.rangeEnd, kFFTComplexSize-1);

			double power = 0.0;

			if (end >= begin)
			{
				for (int j = begin; j <= end; ++j)
					power += fftPowerValue(j);

				power /= (end - begin + 1);
			}

			const bool wasBelow = b.lastValue < b.treshold;
			b.lastValue = power;
			const bool isBelow = b.lastValue < b.treshold;

			if (wasBelow && !isBelow)
			{
				if (!b.onUp.empty())
					triggerEvent(b.onUp.c_str());
			}

			if (!wasBelow && isBelow)
			{
				if (!b.onDown.empty())
					triggerEvent(b.onDown.c_str());
			}
		}
	}

	// process MIDI input

	for (auto & map : m_midiMaps)
	{
		const bool g_live = false; // todo : add a global live variable

		if (g_live && !map.liveEnabled)
			continue;

		if (map.type == SceneMidiMap::kMapType_Event)
		{
			if (config.midiWentDown(map.id))
			{
				bool found = false;

				for (auto i = m_events.begin(); i != m_events.end(); ++i)
				{
					SceneEvent * event = *i;

					if (event->m_name == map.event)
					{
						event->execute(*this);

						found = true;
					}
				}

				if (!found)
				{
					logWarning("unable to find event %s", map.event.c_str());
				}
			}
		}
		else if (map.type == SceneMidiMap::kMapType_EffectVar)
		{
			if (config.midiIsDown(map.id))
			{
				SceneEffect * effect = findEffectByName(map.effect.c_str(), nullptr);

				if (effect != nullptr)
				{
					TweenFloat * var = effect->m_effect->getVar(map.var.c_str());

					if (var != nullptr)
					{
						if (!var->isActive())
						{
							const float value = config.midiGetValue(map.id, 0.f);

							*var = map.min + (map.max - map.min) * value;
						}
					}
					else
					{
						logWarning("unable to find effect variable %s", map.var.c_str());
					}
				}
				else
				{
					logWarning("unable to find effect %s", map.effect.c_str());
				}
			}
		}
	}

	//

	m_time += dt;

#if ENABLE_DEBUG_TEXT
	// process debug text

	for (auto i = m_debugTexts.begin(); i != m_debugTexts.end(); )
	{
		DebugText & t = *i;

		if (m_time >= t.endTime)
			i = m_debugTexts.erase(i);
		else
			++i;
	}
#endif
}

void Scene::draw(DrawableList & list)
{
	ScopedSceneBlock block(this);

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->draw(list);
	}

	//

	struct SceneDebugDrawable : Drawable
	{
		Scene * m_scene;

		SceneDebugDrawable(float z, Scene * scene)
			: Drawable(z)
			, m_scene(scene)
		{
		}

		virtual void draw() override
		{
			m_scene->debugDraw();

			m_scene->m_varTimeStep = 0.f;
		}
	};

	new (list) SceneDebugDrawable(0.f, this);
}

void Scene::debugDraw()
{
#if ENABLE_DEBUG_TEXT
	if (!config.debug.showMessages)
		return;

	// draw debug text

	setFont("calibri.ttf");
	setColor(colorWhite);
	setBlend(BLEND_ALPHA);

	float x = GFX_SX - 10;
	float y = 10;
	const int fontSize = 24;

	for (auto i = m_debugTexts.rbegin(); i != m_debugTexts.rend(); ++i)
	{
		drawText(x, y, fontSize, -1, 0.f, i->text.c_str());

		y += fontSize + 4;
	}
#endif
}

SceneLayer * Scene::findLayerByName(const char * name)
{
	for (auto l = m_layers.begin(); l != m_layers.end(); ++l)
	{
		SceneLayer * layer = *l;

		if (layer->m_name == name)
			return layer;
	}

	return nullptr;
}

SceneEffect * Scene::findEffectByName(const char * name, SceneLayer ** out_layer)
{
	if (out_layer)
		*out_layer = nullptr;

	for (auto l = m_layers.begin(); l != m_layers.end(); ++l)
	{
		SceneLayer * layer = *l;

		for (auto e = layer->m_effects.begin(); e != layer->m_effects.end(); ++e)
		{
			SceneEffect * effect = *e;

			if (effect->m_name == name)
			{
				if (out_layer)
					*out_layer = layer;

				return effect;
			}
		}
	}

	return nullptr;
}

SceneEvent * Scene::findEventByName(const char * name)
{
	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		if (event->m_name == name)
			return event;
	}

	return nullptr;
}

void Scene::triggerEvent(const char * name)
{
	ScopedSceneBlock sceneBlock(this);

	logDebug("triggerEvent: %s", name);

	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		if (event->m_name == name)
		{
			event->execute(*this);

			addDebugText(event->m_name.c_str());
		}
	}
}

void Scene::triggerEventByOscId(int oscId)
{
	ScopedSceneBlock sceneBlock(this);

	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		if (event->m_oscId == oscId)
		{
			event->execute(*this);

			addDebugText("%s [id=%02d, time=%06.2f]", event->m_name.c_str(), oscId, m_time);
		}
	}
}

void Scene::addDebugText(const char * format, ...)
{
#if ENABLE_DEBUG_TEXT
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	DebugText t;
	t.text = text;
	t.endTime = m_time + 5.5f;

	m_debugTexts.push_back(t);
#endif
}

bool Scene::load(const char * filename)
{
	ScopedSceneBlock sceneScope(this);

	bool result = true;

	m_filename = filename;

	tinyxml2::XMLDocument xmlDoc;

	const auto t1_loadDocument = loadtimer();
	
	Array<uint8_t> * fileContents = nullptr;
	if (getSceneFileContents(filename, fileContents))
	{
		if (xmlDoc.Parse((const char *)fileContents->data, fileContents->size) != XML_NO_ERROR)
		{
			logError("failed to parse scene: %s", filename);

			result = false;
		}
	}
	else if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load scene: %s", filename);

		result = false;
	}

	if (result)
	{
		const XMLElement * xmlScene = xmlDoc.FirstChildElement("scene");

		if (xmlScene == 0)
		{
			logError("missing <scene> element");

			result = false;
		}
		else
		{
			m_fftFade = floatAttrib(xmlScene, "fft_fade", .5f);

			//

			const auto t2_layers = loadtimer();

			const XMLElement * xmlLayers = xmlScene->FirstChildElement("layers");

			if (xmlLayers == 0)
			{
				logWarning("no layers found in scene!");
			}
			else
			{
				for (const XMLElement * xmlLayer = xmlLayers->FirstChildElement("layer"); xmlLayer; xmlLayer = xmlLayer->NextSiblingElement("layer"))
				{
					SceneLayer * layer = new SceneLayer(this);

					if (!layer->load(xmlLayer))
					{
						delete layer;
					}
					else
					{
						m_layers.push_back(layer);
					}
				}
			}

			//

			const auto t3_events = loadtimer();

			const XMLElement * xmlEvents = xmlScene->FirstChildElement("events");

			if (xmlEvents == 0)
			{
				logDebug("scene doesn't have any events");
			}
			else
			{
				for (const XMLElement * xmlEvent = xmlEvents->FirstChildElement("event"); xmlEvent; xmlEvent = xmlEvent->NextSiblingElement("event"))
				{
					SceneEvent * event = new SceneEvent();

					event->load(xmlEvent);

					m_events.push_back(event);
				}
			}

			//

			const auto t4_fft = loadtimer();

			const XMLElement * xmlFft = xmlScene->FirstChildElement("fft");

			if (xmlFft == 0)
			{
				logDebug("scene doesn't have any FFT settings");
			}
			else
			{
				for (const XMLElement * xmlBucket = xmlFft->FirstChildElement("bucket"); xmlBucket; xmlBucket = xmlBucket->NextSiblingElement("bucket"))
				{
					const bool enabled = boolAttrib(xmlBucket, "enabled", true);
					const int id = intAttrib(xmlBucket, "id", -1);
					const std::string range = stringAttrib(xmlBucket, "range", "");
					const float treshold = floatAttrib(xmlBucket, "treshold", .5f);
					const std::string onUp = stringAttrib(xmlBucket, "on_up", "");
					const std::string onDown = stringAttrib(xmlBucket, "on_down", "");
					
					if (id >= kMaxFftBuckets)
					{
						logError("fft bucket id is out of range. id=%d, max=%d", id, kMaxFftBuckets-1);
					}
					else
					{
						std::vector<std::string> rangeElems;
						splitString(range, rangeElems, ',');

						if (rangeElems.size() != 2)
						{
							logError("range doesn't have two elements");
						}
						else
						{
							if (enabled)
							{
								FftBucket & b = m_fftBuckets[id];

								b.isActive = true;
								b.rangeBegin = Parse::Int32(rangeElems[0]);
								b.rangeEnd = Parse::Int32(rangeElems[1]);
								b.treshold = treshold;
								b.onUp = onUp;
								b.onDown = onDown;
							}
						}
					}
				}
			}

			//

			const auto t5_modifiers = loadtimer();

			const XMLElement * xmlModifiers = xmlScene->FirstChildElement("modifiers");

			if (xmlModifiers == 0)
			{
				logDebug("scene doesn't have any modifiers");
			}
			else
			{
				for (const XMLElement * xmlModifier = xmlModifiers->FirstChildElement("modifier"); xmlModifier; xmlModifier = xmlModifier->NextSiblingElement("modifier"))
				{
					const bool enabled = boolAttrib(xmlModifier, "enabled", true);

					const std::string layerName  = stringAttrib(xmlModifier, "layer",  "");
					const std::string effectName = stringAttrib(xmlModifier, "effect", "");
					const std::string varName    = stringAttrib(xmlModifier, "var",    "");
					const std::string modName    = stringAttrib(xmlModifier, "mod",    "");
					const std::string op         = stringAttrib(xmlModifier, "op",     "");
					const std::string range      = stringAttrib(xmlModifier, "range",  "");

					TweenFloat * var = nullptr;
					
					if (!layerName.empty())
					{
						SceneLayer * layer = findLayerByName(layerName.c_str());
						if (layer)
							var = layer->getVar(varName.c_str());
						if (var == nullptr)
							logError("could not find var value %s:%s", layerName.c_str(), varName.c_str());
					}
					else if (!effectName.empty())
					{
						SceneEffect * effect = findEffectByName(effectName.c_str(), nullptr);
						if (effect)
							var = effect->m_effect->getVar(varName.c_str());
						if (var == nullptr)
							logError("could not find var value %s:%s", effectName.c_str(), varName.c_str());
					}

					if (var == nullptr)
					{
						//
					}
					else
					{
						TweenFloat * mod = getVar(modName.c_str());

						if (mod == nullptr)
						{
							logError("could not find global value %s", modName.c_str());
						}
						else
						{
							Modifier modifier;
							modifier.var = var;
							modifier.mod = mod;
							modifier.str = floatAttrib(xmlModifier, "strength", 1.f);
							modifier.op = Modifier::parseOp(op);

							if (!range.empty())
							{
								std::vector<std::string> values;
								splitString(range, values, ',');

								if (values.size() != 4)
								{
									logError("invalid ranges");
								}
								else
								{
									modifier.hasRange = true;
									modifier.range[0] = Parse::Float(values[0]);
									modifier.range[1] = Parse::Float(values[1]);
									modifier.range[2] = Parse::Float(values[2]);
									modifier.range[3] = Parse::Float(values[3]);
								}
							}

							if (enabled)
							{
								var->addModifier(this);

								m_modifiers.push_back(modifier);
							}
						}
					}
				}
			}

			//

			const auto t6_midi = loadtimer();

			const XMLElement * xmlMidi = xmlScene->FirstChildElement("midi");

			if (xmlMidi == 0)
			{
				logDebug("scene doesn't have any MIDI mappings");
			}
			else
			{
				for (const XMLElement * xmlMidiMap = xmlMidi->FirstChildElement("map"); xmlMidiMap; xmlMidiMap = xmlMidiMap->NextSiblingElement("map"))
				{
					SceneMidiMap map;

					map.id = intAttrib(xmlMidiMap, "id", -1);

					if (map.id < 0)
						continue;

					map.liveEnabled = boolAttrib(xmlMidiMap, "live", false);

					map.event = stringAttrib(xmlMidiMap, "event", "");
					map.effect = stringAttrib(xmlMidiMap, "effect", "");
					map.var = stringAttrib(xmlMidiMap, "var", "");
					map.min = floatAttrib(xmlMidiMap, "min", 0.f);
					map.max = floatAttrib(xmlMidiMap, "max", 1.f);

					if (!map.effect.empty() && !map.var.empty())
					{
						map.type = SceneMidiMap::kMapType_EffectVar;
					}
					else if (!map.event.empty())
					{
						map.type = SceneMidiMap::kMapType_Event;
					}
					else
					{
						continue;
					}

					m_midiMaps.push_back(map);
				}
			}

			const auto t7_end = loadtimer();

			logLoadtime(t2_layers    - t1_loadDocument, "Scene::load::document");
			logLoadtime(t3_events    - t2_layers,       "Scene::load::layers");
			logLoadtime(t4_fft       - t3_events,       "Scene::load::events");
			logLoadtime(t5_modifiers - t4_fft,          "Scene::load::fft");
			logLoadtime(t6_midi      - t5_modifiers,    "Scene::load::midi");
			logLoadtime(t7_end       - t6_midi,         "Scene::load::modifiers");
		}
	}

	return result;
}

void Scene::clear()
{
	m_filename.clear();

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		delete layer;
		layer = nullptr;
	}

	m_layers.clear();

	//

	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		delete event;
		event = nullptr;
	}

	m_events.clear();

	//

	for (int i = 0; i < kMaxFftBuckets; ++i)
		m_fftBuckets[i] = FftBucket();

	//

	m_modifiers.clear();

	//

	m_midiMaps.clear();

	//

	m_fftFade = .5f;

	m_time = 0.f;

	m_varPcmVolume = 0.f;
	m_varTime = 0.f;
	m_varTimeStep = 0.f;

	//

#if ENABLE_DEBUG_TEXT
	m_debugTexts.clear();
#endif
}

void Scene::advanceTo(const float time)
{
	double dt = time - m_time;

	while (dt > 0.f)
	{
		const double step = std::min(dt, 1.0 / 15.0);
		
		tick(step);

		dt -= step;
	}
}

void Scene::syncTime(const float time)
{
	ScopedSceneBlock sceneBlock(this);

	for (SceneLayer * layer : m_layers)
	{
		for (SceneEffect * effect : layer->m_effects)
		{
			if (effect->m_effect)
			{
				effect->m_effect->syncTime(time);
			}
		}
	}
}

float Scene::applyModifier(TweenFloat * tweenFloat, float value)
{
	for (Modifier & modifier : m_modifiers)
	{
		if (modifier.var == tweenFloat)
		{
			value = modifier.apply(value);
		}
	}

	return value;
}

//

SceneSurfacePool::SceneSurfacePool(const int capacity)
{
	for (int i = 0; i < capacity; ++i)
		m_surfaces.push_back(new Surface(GFX_SX, GFX_SY, true));
}

Surface * SceneSurfacePool::alloc()
{
	Surface * result = nullptr;

	if (m_surfaces.empty())
	{
		logWarning("surface pool depleted. allocating new surface!");

		result = new Surface(GFX_SX, GFX_SY, true);
	}
	else
	{
		result = m_surfaces.back();

		m_surfaces.pop_back();
	}

	return result;
}

void SceneSurfacePool::free(Surface * surface)
{
	m_surfaces.push_back(surface);
}

//

#if ENABLE_LOADTIME_PROFILING
#include "Timer.h"
void logLoadtime(const uint64_t time, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	printf("loadtime: %03.2fms :: %s\n", time / 1000.f, text);
}
uint64_t loadtimer()
{
	return g_TimerRT.TimeUS_get();
}
#endif

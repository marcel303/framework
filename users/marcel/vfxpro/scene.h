#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace tinyxml2
{
	class XMLElement;
}

struct DrawableList;
struct Effect;

struct SceneEffect;
struct SceneAction;
struct SceneEvent;
struct SceneLayer;
struct Scene;

struct SceneEffect
{
	std::string m_name;
	Effect * m_effect;

	SceneEffect();
	~SceneEffect();

	bool load(const tinyxml2::XMLElement * xmlEffect);
};

struct SceneLayer : TweenFloatCollection
{
	enum BlendMode
	{
		kBlendMode_Add,
		kBlendMode_Subtract,
		kBlendMode_Alpha,
		kBlendMode_Opaque
	};

	std::string m_name;
	BlendMode m_blendMode;
	bool m_autoClear;

	TweenFloat m_opacity;

	std::vector<SceneEffect*> m_effects;

	SceneLayer();
	~SceneLayer();

	void load(const tinyxml2::XMLElement * xmlLayer);

	void tick(const float dt);
	void draw(DrawableList & drawableList);
	void draw();
};

struct SceneAction
{
	enum ActionType
	{
		kActionType_None,
		kActionType_Tween
	};

	ActionType m_type;

	struct Tween
	{
		enum TargetType
		{
			kTargetType_Layer,
			kTargetType_Effect
		};

		TargetType m_targetType;
		std::string m_targetName;
		std::string m_varName;
		float m_tweenTo;
		float m_tweenTime;
		EaseType m_easeType;
		float m_easeParam;
		bool m_replaceTween;
	} m_tween;

	SceneAction();

	bool load(const tinyxml2::XMLElement * xmlAction);
};

struct SceneEvent
{
	std::string m_name;
	std::vector<SceneAction*> m_actions;

	SceneEvent();
	~SceneEvent();

	void execute(Scene & scene);

	void load(const tinyxml2::XMLElement * xmlEvent);
};

struct Scene
{
	std::string m_name;
	std::vector<SceneLayer*> m_layers;
	std::vector<SceneEvent*> m_events;

	Scene();
	~Scene();

	void tick(const float dt);
	void draw(DrawableList & drawableList);

	void triggerEvent(const char * name);

	bool load(const char * filename);
};

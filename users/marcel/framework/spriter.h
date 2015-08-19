#pragma once

#include <vector> // fixme : remove dep

namespace spriter
{
	class Animation;
	struct Drawable;
	class Entity;
	class FileCache;
	class Scene;

	struct Drawable
	{
		const char * filename;

		float x;
		float y;
		float angle;
		float scaleX;
		float scaleY;
		float pivotX;
		float pivotY;
		float a;
	};

	struct Hitbox
	{
		float sx;
		float sy;

		float x;
		float y;
		float angle;
		float scaleX;
		float scaleY;
		float pivotX;
		float pivotY;
	};

	enum ObjectType
	{
		kObjectType_Sprite,
		kObjectType_Bone,
		kObjectType_Box,
		kObjectType_Point,
		kObjectType_Sound,
		kObjectType_Entity,
		kObjectType_Variable
	};

	class Object
	{
	public:
		std::string name;
		std::string shortName;
		ObjectType type;
		float sx;
		float sy;
		float pivotX;
		float pivotY;
	};

	class Entity
	{
	public:
		Entity(Scene * scene);
		~Entity();

		int getAnimIndexByName(const char * name) const;
		int getAnimLength(int index) const;
		bool isAnimLooped(int index) const;
		void getDrawableListAtTime(int animIndex, int characterMap, float time, Drawable * drawables, int & numDrawables) const;
		bool getHitboxAtTime(int animIndex, const char * name, float time, Hitbox & hitbox) const;

		Scene * m_scene;
		std::string m_name;
		std::vector<Object> m_objects;
		std::vector<Animation*> m_animations;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		bool load(const char * filename);

		int getEntityIndexByName(const char * name) const;
		int getCharacterMapIndexByName(const char * name) const;
		bool hasCharacterMap(int index) const;

		std::vector<FileCache*> m_fileCaches;
		std::vector<Entity*> m_entities;
	};
}

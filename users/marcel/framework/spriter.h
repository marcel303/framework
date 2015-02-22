#pragma once

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

	class Entity
	{
	public:
		Entity(Scene * scene);
		~Entity();

		int getAnimIndexByName(const char * name) const;
		int getAnimLength(int index) const;
		bool isAnimLooped(int index) const;
		void getDrawableListAtTime(int animIndex, float time, Drawable * drawables, int & numDrawables) const;

		Scene * m_scene;
		std::string m_name;
		std::vector<Animation*> m_animations;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		bool load(const char * filename);

		int getEntityIndexByName(const char * name) const;

		FileCache * m_fileCache;
		std::vector<Entity*> m_entities;
	};
}

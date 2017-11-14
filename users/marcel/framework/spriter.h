/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <vector>

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

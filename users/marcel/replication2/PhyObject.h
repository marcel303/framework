#ifndef PHYOBJECT_H
#define PHYOBJECT_H
#pragma once

#include <vector>
#include "CDObject.h"

namespace Phy
{
	class Object
	{
	public:
		Object();
		~Object();

		void NextFrame();

		void Initialize(int group, const Vec3& position, const Vec3& velocity, bool dynamic, bool gravity, void* up);

		void AddGeometry(CD::ShObject object);

		void SetPosition(Vec3 position);

		void Debug_Render();

		int m_group;
		Vec3 m_position;
		Vec3 m_velocity;
		Vec3 m_velocityDamp;
		Vec3 m_force;

		bool m_dynamic;
		bool m_gravity;

		void* m_up;

		inline std::vector<CD::ShObject>& GetGeometry()
		{
			return m_geometry;
		}

	private:
		std::vector<CD::ShObject> m_geometry;
	};
}

#endif

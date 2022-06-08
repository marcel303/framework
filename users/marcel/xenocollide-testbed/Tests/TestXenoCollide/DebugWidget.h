/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#pragma once

#include "DrawUtil.h"
#include "RenderPolytope.h"

#include "framework.h"

#include <list>

//////////////////////////////////////////////////////////////////////////////

namespace XenoCollide
{
	class DebugWidget
	{

	public:

		DebugWidget(int32 lifetime)
		{
			m_lifetime = lifetime;
		}

		int32 GetLifetime()
		{
			return m_lifetime;
		}

		virtual void Draw() = 0;

	protected:

		int m_lifetime;
	};

	//////////////////////////////////////////////////////////////////////////////

	class DebugPoint : public DebugWidget
	{
	public:
		DebugPoint(const Vector& p, int32 lifetime = -1) : DebugWidget(lifetime)
		{
			m_point = p;
		}

		void Draw()
		{
			//		DrawGeoSphere(m_point, Quat(0, 0, 0, 1), 1.0f, 4);
			DrawSphere(m_point, Quat(0, 0, 0, 1), 1.0f, Vector(1, 1, 1));
		}

		Vector m_point;
	};

	//////////////////////////////////////////////////////////////////////////////

	class DebugVector : public DebugWidget
	{

	public:

		DebugVector(const Vector& p1, const Vector& p2, int32 lifetime = -1) : DebugWidget(lifetime)
		{
			m_v1 = p1;
			m_v2 = p2;
		}

		void Draw()
		{
			gxBegin(GX_LINES);
			gxColor3f(1, 0, 0);
			gxVertex3f(m_v1.X(), m_v1.Y(), m_v1.Z());
			gxVertex3f(m_v2.X(), m_v2.Y(), m_v2.Z());
			gxEnd();
		}

		Vector m_v1;
		Vector m_v2;
	};

	//////////////////////////////////////////////////////////////////////////////

	class DebugTri : public DebugWidget
	{

	public:

		DebugTri(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& c, int32 lifetime = -1) : DebugWidget(lifetime)
		{
			m_v1 = v1;
			m_v2 = v2;
			m_v3 = v3;
			m_color = c;
		}

		void Draw()
		{
			Vector n = (m_v3 - m_v2) % (m_v1 - m_v2);

			if (n.CanNormalize3())
			{
				n.Normalize3();

				gxBegin(GX_TRIANGLES);
				gxColor3f(m_color.X(), m_color.Y(), m_color.Z());
				gxNormal3f(n.X(), n.Y(), n.Z());
				gxVertex3f(m_v1.X(), m_v1.Y(), m_v1.Z());
				gxVertex3f(m_v2.X(), m_v2.Y(), m_v2.Z());
				gxVertex3f(m_v3.X(), m_v3.Y(), m_v3.Z());
				gxEnd();
			}
		}

		Vector m_v1;
		Vector m_v2;
		Vector m_v3;
		Vector m_color;

	};

	//////////////////////////////////////////////////////////////////////////////

	class DebugPolytope : public DebugWidget
	{
	public:

		DebugPolytope(CollideGeometry& g, const Quat& q, const Vector& t, const Vector& c, int32 lifetime = -1) : DebugWidget(lifetime)
		{
			m_renderModel.Init(g, 4);
			m_renderModel.SetColor(c);
			m_q = q;
			m_t = t;
		}

		void Draw()
		{
			m_renderModel.Draw(m_q, m_t);
		}

		RenderPolytope m_renderModel;
		Quat m_q;
		Vector m_t;
	};

	//////////////////////////////////////////////////////////////////////////////

	typedef std::list<DebugWidget*> DebugWidgetList;
	DebugWidgetList g_debugQueue;

	bool gTrackingOn = false;

#define ENABLE_TRACKING 1

	//////////////////////////////////////////////////////////////////////////////

	inline void DebugPushPolytope(CollideGeometry& g, const Quat& q, const Vector& t, int32 lifetime = -1)
	{
#if ENABLE_TRACKING
		if (gTrackingOn)
			g_debugQueue.push_back(new DebugPolytope(g, q, t, Vector(0.5f, 0.5f, 0.5f), lifetime));
#endif
	}

	//////////////////////////////////////////////////////////////////////////////

	inline void DebugPushPoint(const Vector& p, int32 lifetime = -1)
	{
#if ENABLE_TRACKING
		if (gTrackingOn)
			g_debugQueue.push_back(new DebugPoint(p, lifetime));
#endif
	}

	//////////////////////////////////////////////////////////////////////////////

	inline void DebugPushVector(const Vector& p1, const Vector& p2, int32 lifetime = -1)
	{
#if ENABLE_TRACKING
		if (gTrackingOn)
			g_debugQueue.push_back(new DebugVector(p1, p2, lifetime));
#endif
	}

	//////////////////////////////////////////////////////////////////////////////

	inline void DebugPushTri(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& c, int32 lifetime = -1)
	{
#if ENABLE_TRACKING
		if (gTrackingOn)
			g_debugQueue.push_back(new DebugTri(v1, v2, v3, c, lifetime));
#endif
	}
}

//////////////////////////////////////////////////////////////////////////////

#include "Camera.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Graphics.h"
#include "Mat4x4.h"
#if defined(IPHONEOS) || defined(BBOS)
#include "OpenGLCompat.h"
#endif

Camera::Camera()
{
}

//void Camera::Setup(int viewSx, int viewSy, int worldSx, int worldSy)
void Camera::Setup(int viewSx, int viewSy, Vec2F min, Vec2F max)
{
	m_ViewSize[0] = (float)viewSx;
	m_ViewSize[1] = (float)viewSy;
	
	m_WorldSize[0] = max[0] - min[0];
	m_WorldSize[1] = max[1] - min[1];
	
	m_WorldPos = min;
	
	const Vec2F diff = m_WorldSize - m_ViewSize;
	
	m_Area = RectF(diff / 2.0f, m_ViewSize);
}

void Camera::Move(const Vec2F& delta)
{
	m_Area.m_Position = m_Area.m_Position.Add(delta);
}

void Camera::Focus(const Vec2F& location)
{
	m_Area.m_Position = location.Subtract(m_Area.m_Size.Scale(0.5f));
}

void Camera::AdjustSize(const Vec2F& newSize)
{
	const Vec2F delta = m_Area.m_Size.Subtract(newSize);
	
	m_Area.m_Position = m_Area.m_Position.Add(delta.Scale(0.5f));
	m_Area.m_Size = m_Area.m_Size.Subtract(delta);
}

void Camera::Shrink(float amount)
{
	const Vec2F newSize = m_Area.m_Size.Scale(1.0f - amount);

	AdjustSize(newSize);
}

void Camera::Zoom(float zoom)
{
	const float newSizeX = m_ViewSize[0] / zoom;
	const float newSizeY = m_ViewSize[1] / zoom;

	AdjustSize(Vec2F(newSizeX, newSizeY));
}

void Camera::Clip()
{
	// normalize area height, based on ratio view size
	
	const float ratio = m_ViewSize[1] / m_ViewSize[0];
	
	m_Area.m_Size[1] = m_Area.m_Size[0] * ratio;
	
	// make sure rect is within game area
	
	for (int i = 0; i < 2; ++i)
	{
		float sizeDiff = m_WorldSize[i] - (m_Area.m_Position[i] + m_Area.m_Size[i]);

		if (sizeDiff < 0.0f)
		{
			m_Area.m_Position[i] += sizeDiff;
		}
		
		if (m_Area.m_Position[i] < m_WorldPos[i])
		{
			m_Area.m_Position[i] = m_WorldPos[i];
		}
	}
}

void Camera::ApplyGL(bool tilt3D)
{
	/*LOG_DBG("camera: (%g, %g) x (%g, %g)",
		m_Area.m_Position[0],
		m_Area.m_Position[1],
		m_Area.m_Size[0],
		m_Area.m_Size[1]);*/
	
	const float focusX = m_Area.m_Position[0] + m_Area.m_Size[0] * 0.5f;
	const float focusY = m_Area.m_Position[1] + m_Area.m_Size[1] * 0.5f;

	const float zoom = Zoom_get();

#if 0
	glTranslatef(m_ViewSize[0] / 2.0f, m_ViewSize[1] / 2.0f, 0.0f);
	glScalef(zoom, zoom, 1.0f);
	glTranslatef(-focusX, -focusY, 0.0f);
#else
	Mat4x4 mat;
	
#if defined(IPAD)
	mat.MakePerspectiveLH(Calc::mPI2, VIEW_SY / (float)VIEW_SX, 0.1f, 1000.0f);
#else
	mat.MakePerspectiveLH(Calc::mPI2, VIEW_SY / (float)VIEW_SX, 0.1f, 1000.0f);

#if defined(IPHONEOS)
	//glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	//mat.MakePerspectiveLH(Calc::mPI2, VIEW_SX / (float)VIEW_SY, 0.1f, 1000.0f);
	Mat4x4 matRot;
	matRot.MakeRotationZ(g_GameState->m_GameSettings->m_ScreenFlip ? -Calc::mPI2 : +Calc::mPI2);
	mat = matRot * mat;
#endif
#endif
	
#if 0
	// rotate view and adjust Y direction
	
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glTranslatef(0.0f, -VIEW_SY, 0.0f);
#endif
	
	Mat4x4 perspective = mat;

	Mat4x4 scale;
	scale.MakeScaling(1.0f, -1.0f, 1.0f);
	float t = 0.04f;
	Vec3 mid(m_WorldSize[0] * 0.5f, m_WorldSize[1] * 0.5f, 0.0f);
	Vec3 target(focusX, focusY, 0.0f);
	Vec3 pos = target * (1.0f - t) + mid * t;

	if (tilt3D)
	{
		mat.MakeLookat(
			Vec3(pos[0], pos[1], -VIEW_SY/2.0f / zoom),
			target,
			Vec3(0.0f, 1.0f, 0.0f));
	}
	else
	{
		mat.MakeLookat(
			target + Vec3(0.0f, 0.0f, -VIEW_SY/2.0f / zoom),
			target,
			Vec3(0.0f, 1.0f, 0.0f));
	}
	
	Mat4x4 final = perspective * scale * mat;

#if defined(BBOS_ALLTOUCH) && 0
	Mat4x4 rot;
	rot.MakeRotationZ(-M_PI/2.f);
	final = rot * final;
#endif

	gGraphics.MatrixSet(MatrixType_Projection, final);
#endif
	
//	NSLog([NSString stringWithFormat:@"cam: %f, %f", m_Area.m_Position[0], m_Area.m_Position[1]]);
//	NSLog([NSString stringWithFormat:@"focus: %f, %f", focusX, focusY]);
}

Vec2F Camera::ViewToWorld(const Vec2F& point) const
{
	const float tx = point[0] / m_ViewSize[0];
	const float ty = point[1] / m_ViewSize[1];
	
	return Vec2F(
		m_Area.m_Position[0] + m_Area.m_Size[0] * tx,
		m_Area.m_Position[1] + m_Area.m_Size[1] * ty);
}

Vec2F Camera::WorldToView(const Vec2F& point) const
{
	const float tx = (point[0] - m_Area.m_Position[0]) / m_Area.m_Size[0] * m_ViewSize[0];
	const float ty = (point[1] - m_Area.m_Position[1]) / m_Area.m_Size[1] * m_ViewSize[1];
	
	return Vec2F(tx, ty);
}

float Camera::Zoom_get() const
{
	return m_ViewSize[0] / m_Area.m_Size[0];
}

Vec2F Camera::Position_get() const
{
	return Vec2F(m_Area.m_Position.Add(m_Area.m_Size.Scale(0.5f)));
}

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

#include "XenoTestbed.h"
#include "XenoTestbedWindow.h"
#include "XenoTestbedWindow.h"

#include "framework.h"

#define XENO_TODO 0
#define XENO_TODO_MATERIAL 0

XenoTestbedWindow* XenoTestbedWindow::s_this = NULL;

XenoTestbedWindow::XenoTestbedWindow()
: m_leftButtonDown(false)
, m_middleButtonDown(false)
, m_rightButtonDown(false)
, m_lastMousePoint(0,0)
, m_module(NULL)
, m_moduleIndex(0)
, m_captureCount(0)
{
	m_rotation = new TrackBall;
	m_rotation->SetMagnitudeScale(0.01f);

	m_translation = new Vector(0, 0, -200.0f);
}

XenoTestbedWindow::~XenoTestbedWindow()
{
	delete m_rotation;
	m_rotation = NULL;

	delete m_translation;
	m_translation = NULL;

	delete m_module;
	m_module = NULL;
}


bool XenoTestbedWindow::Init(void)
{
	s_this = this;

	framework.enableDepthBuffer = true;
	
	framework.init(800, 600);

	// Initiaize GL to reasonable defaults

#if XENO_TODO_MATERIAL == 1
	GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	GLfloat lightAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat lightDiffuse[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat lightSpecular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
//	GLfloat lightSpecular[] = { 1, 1, 1, 1.0f };
	GLfloat materialColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat lightpos[] = { -8.0f, 12.0f, 10.0f, 0.0f };
//	GLfloat lightpos[] = { -8.0f, 12.0f, -10.0f, 0.0f };

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);	
	glEnable(GL_COLOR_MATERIAL);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	glLightfv(GL_LIGHT1, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, black);
//	glLightfv(GL_LIGHT1, GL_POSITION, lightpos);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glDisable(GL_LIGHT3);

	glMaterialfv(GL_FRONT, GL_SPECULAR, lightSpecular);
	glMaterialfv(GL_BACK, GL_SPECULAR, black);

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialColor);
	glMaterialfv(GL_BACK, GL_AMBIENT_AND_DIFFUSE, materialColor);

//	glMateriali(GL_FRONT, GL_SHININESS, 128);
	glMateriali(GL_FRONT, GL_SHININESS, 64);
	glMateriali(GL_BACK, GL_SHININESS, 128);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPointSize(5);
    glPolygonOffset(1.0, 2);

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#endif

	RegisterTestModule::FactoryMethod* factoryMethod = RegisterTestModule::GetFactoryMethod( m_moduleIndex );
	m_module = factoryMethod();

	// Initialize the test module
	m_module->Init();

	LoadView();
	return true;
}

void XenoTestbedWindow::SetSize(int width, int height)
{
	// Set up the projection matrix and viewport to use the new size
	gxMatrixMode(GX_PROJECTION);
	gxLoadIdentity();
	
	projectPerspective3d(30.0f, 1.0f, 1000.0f);
}

void XenoTestbedWindow::OnSize(int nType, int cx, int cy)
{
	SetSize(cx, cy);
}

void XenoTestbedWindow::OnPaint()
{
	if (!m_module) return;

// todo : xeno : beginDraw, enndDraw

	// Draw the scene
	DrawScene();
}

void XenoTestbedWindow::DrawScene(void)
{
#if XENO_TODO_MATERIAL == 1
	// Enable lighting calculations
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);

	// Enable depth calculations
	glEnable(GL_DEPTH_TEST);

	// Clear the color and depth buffers
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set the material color to follow the current color
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
#endif

	// Change to model view matrix stack.
	gxMatrixMode(GX_MODELVIEW);

	// Start with an identity matrix
	gxLoadIdentity();

	// Translate the view back
	gxTranslatef( m_translation->X(), m_translation->Y(), m_translation->Z() );

	// We want to start off looking down the Y axis
	gxRotatef(-90, 1, 0, 0);
	gxRotatef(90, 0, 0, 1);

	// Obtain the trackball rotation and convert to axis / angle
	Quat q = m_rotation->GetRotation();
	Vector vec;
	float angle;
	(~q).ConvertToAxisAngle(&vec, &angle);

	// Convert the angle from radians to degrees
	angle = angle * 180.0f / PI;

	// Rotate the view
	gxRotatef(angle, vec.X(), vec.Y(), vec.Z());

	// Draw the scene
	m_module->DrawScene();

#if XENO_TODO_MATERIAL == 1
	// Flush the drawing pipeline
	glFlush();
#endif
}

void XenoTestbedWindow::WindowPointToWorldRay(Vector* rayOrigin, Vector* rayDirection, const Point& p)
{
	float modelMatrix[16];
	float projectionMatrix[16];

	gxGetMatrixf(GX_MODELVIEW, modelMatrix);
	gxGetMatrixf(GX_PROJECTION, projectionMatrix);

#if XENO_TODO_MATERIAL == 1
// todo : use framework function to un-project ?
	GLenum err = glGetError();

	GLdouble x0, y0, z0;
	GLdouble x1, y1, z1;

	gluUnProject(p.x, viewport[3] - p.y, 0.0f, modelMatrix, projectionMatrix, viewport, &x0, &y0, &z0);
	gluUnProject(p.x, viewport[3] - p.y, 1.0f, modelMatrix, projectionMatrix, viewport, &x1, &y1, &z1);

	Vector diff = Vector( (float32) x1, (float32) y1, (float32) z1 ) - Vector( (float32) x0, (float32) y0, (float32) z0 );
	diff.Normalize3();

	*rayDirection = diff;
	*rayOrigin = Vector((float32)x0, (float32)y0, (float32)z0); //*m_translation;
#endif
}

void XenoTestbedWindow::OnLButtonDown(int nFlags, Point point)
{
	m_lastMousePoint = point;
#if XENO_TODO == 1
	if (GetKeyState(VK_MENU) & 0x80000000)
	{
		m_leftButtonDown = true;
		SetCapture(m_hWnd);
		m_captureCount++;
	}
	else
#endif
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnLeftButtonDown(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnLButtonUp(int nFlags, Point point)
{
#if XENO_TODO == 1
	if (m_leftButtonDown)
	{
		m_leftButtonDown = false;
		if (--m_captureCount == 0)
			ReleaseCapture();
	}
	else
#endif
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnLeftButtonUp(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnMButtonDown(int nFlags, Point point)
{
	m_lastMousePoint = point;
#if XENO_TODO == 1
	if (GetKeyState(VK_MENU) & 0x80000000)
	{
		m_middleButtonDown = true;
		SetCapture(m_hWnd);
		m_captureCount++;
	}
	else
#endif
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnMiddleButtonDown(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnMButtonUp(int nFlags, Point point)
{
#if XENO_TODO == 1
	if (m_middleButtonDown)
	{
		m_middleButtonDown = false;
		if (--m_captureCount == 0)
			ReleaseCapture();
	}
	else
#endif
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnMiddleButtonUp(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnRButtonDown(int nFlags, Point point)
{
	m_lastMousePoint = point;
#if XENO_TODO == 1
	if (GetKeyState(VK_MENU) & 0x80000000)
	{
		m_rightButtonDown = true;
		SetCapture(m_hWnd);
		m_captureCount++;
	}
	else
#endif
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnRightButtonDown(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnRButtonUp(int nFlags, Point point)
{
#if XENO_TODO == 1
	if (m_rightButtonDown)
	{
		m_rightButtonDown = false;
		if (--m_captureCount == 0) ReleaseCapture();
	}
	else
#endif
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnRightButtonUp(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnMouseMove(int nFlags, Point point)
{
	Vector rayOrigin;
	Vector rayDirection;
	WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
	m_module->OnMouseMove(rayOrigin, rayDirection, Vector(point.x, point.y, 0));

	Point delta(point.x - m_lastMousePoint.x, point.y - m_lastMousePoint.y);
	m_lastMousePoint = point;

	if (m_middleButtonDown || (m_leftButtonDown && m_rightButtonDown))
	{
		*m_translation += Vector(delta.x * 0.2f, -delta.y * 0.2f, 0);
	}
	else if (m_leftButtonDown)
	{
		float32 yaw;
		float32 pitch;
		m_rotation->GetYawPitchRoll(&yaw, &pitch, NULL);
		yaw -= delta.x * 0.01f;
		pitch += delta.y * 0.01f;

		pitch = Min(pitch, PI_2 - 0.001f);
		pitch = Max(pitch, -(PI_2 - 0.001f));

		m_rotation->SetYawPitchRoll(yaw, pitch, 0);
	}
	else if (m_rightButtonDown)
	{
		*m_translation += Vector(0, 0, delta.y * 0.2f);
	}
}

void XenoTestbedWindow::Simulate(float32 dt)
{
	// Simulate the module
	m_module->Simulate(dt);

}

void XenoTestbedWindow::OnChar(int nChar, int nRepCnt, int nFlags)
{
	// Forward the keystroke to the module
	m_module->OnChar(nChar);
}

void XenoTestbedWindow::OnKeyDown(int nChar, int nRepCnt, int nFlags)
{
	// Handle pg-up and pg-down
	if (nChar == 33)
	{
		m_moduleIndex--;

		if (m_moduleIndex < 0)
			m_moduleIndex = RegisterTestModule::ModuleCount() - 1;

		delete m_module;

		m_module = RegisterTestModule::GetFactoryMethod(m_moduleIndex)();
		m_module->Init();
	}
	else if (nChar == 34)
	{
		m_moduleIndex++;

		if (m_moduleIndex >= RegisterTestModule::ModuleCount())
			m_moduleIndex = 0;

		delete m_module;
		m_module = RegisterTestModule::GetFactoryMethod(m_moduleIndex)();

		m_module->Init();
	}
	else if (nChar == 'K')
	{
	#if XENO_TODO == 1
		// Control must be held
		if ( !(GetKeyState(VK_LCONTROL) & 0x8000) && !(GetKeyState(VK_RCONTROL) & 0x8000) )
		{
			return;
		}
	#endif
	
		SaveView();
	}
	else if (nChar == 'L')
	{
	#if XENO_TODO == 1
		// Control must be held
		if ( !(GetKeyState(VK_LCONTROL) & 0x8000) && !(GetKeyState(VK_RCONTROL) & 0x8000) )
		{
			return;
		}
	#endif
	
		LoadView();
	}
	else
	{
		// Forward the keystroke to the module
		m_module->OnKeyDown(nChar);
	}
}

#if XENO_TODO == 1
static Point MakePoint(LPARAM lParam)
{
	return Point( (int16)LOWORD(lParam), (int16)HIWORD(lParam) );
}
#endif

#if XENO_TODO == 1
LRESULT CALLBACK XenoTestbedWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//	int wmId, wmEvent;
	XenoTestbedWindow* wnd = s_this;

	switch (message)
	{
		case WM_PAINT:
			wnd->OnPaint();
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_ERASEBKGND:
			return TRUE;
			break;
		case WM_SIZE:
			wnd->OnSize(wParam, (lParam & 0xffff), (lParam >> 16) & 0xfff);
			break;
		case WM_LBUTTONDOWN:
			wnd->OnLButtonDown(wParam, MakePoint(lParam));
			break;
		case WM_LBUTTONUP:
			wnd->OnLButtonUp(wParam, MakePoint(lParam));
			break;
		case WM_MBUTTONDOWN:
			wnd->OnMButtonDown(wParam, MakePoint(lParam));
			break;
		case WM_MBUTTONUP:
			wnd->OnMButtonUp(wParam, MakePoint(lParam));
			break;
		case WM_RBUTTONDOWN:
			wnd->OnRButtonDown(wParam, MakePoint(lParam));
			break;
		case WM_RBUTTONUP:
			wnd->OnRButtonUp(wParam, MakePoint(lParam));
			break;
		case WM_MOUSEMOVE:
			wnd->OnMouseMove(wParam, MakePoint(lParam));
			break;
		case WM_KEYDOWN:
			wnd->OnKeyDown(wParam, lParam & 0xffff, lParam);
			break;
		case WM_CHAR:
			wnd->OnChar(wParam, lParam & 0xffff, lParam);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#endif

void XenoTestbedWindow::SaveView()
{
	Quat q = m_rotation->GetRotation();
	Vector t = *m_translation;
	FILE* fp = fopen("save.dat", "wt");
	if (fp)
	{
		fprintf(fp, "%f %f %f %f\n", q.X(), q.Y(), q.Z(), q.W());
		fprintf(fp, "%f %f %f %f\n", t.X(), t.Y(), t.Z());
		fclose(fp);
	}
}

void XenoTestbedWindow::LoadView()
{
	FILE* fp = fopen("save.dat", "rt");
	if (fp)
	{
		float32 x, y, z, w;
		x = y = z = w = 0.0f;
		fscanf(fp, " %f %f %f %f \n", &x, &y, &z, &w);
		m_rotation->SetRotation( Quat(x, y, z, w) );
		fscanf(fp, " %f %f %f \n", &x, &y, &z);
		*m_translation = Vector(x, y, z);
		fclose(fp);
	}
}

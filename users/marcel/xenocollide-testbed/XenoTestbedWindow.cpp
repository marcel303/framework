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


#include "stdafx.h"

#include "XenoTestbed.h"
#include "XenoTestbedWindow.h"
#include ".\XenoTestbedwindow.h"

#include "gl/gl.h"
#include "gl/glu.h"

XenoTestbedWindow* XenoTestbedWindow::s_this = NULL;

XenoTestbedWindow::XenoTestbedWindow(HINSTANCE hInstance)
: m_leftButtonDown(false)
, m_middleButtonDown(false)
, m_rightButtonDown(false)
, m_lastMousePoint(0,0)
, m_module(NULL)
, m_moduleIndex(0)
, m_hInstance(hInstance)
, m_captureCount(0)
{
	m_glContext = NULL;

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

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= XenoTestbedWindow::WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= m_hInstance;
	wcex.hIcon			= LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_TEST);
	wcex.lpszClassName	= "XenoTestbedWindow";
	wcex.hIconSm		= NULL;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	ATOM windowClass = RegisterClassEx(&wcex);

	m_hWnd = CreateWindowEx
	(
		WS_EX_APPWINDOW,
		_T("XenoTestbedWindow"),
		_T("Xeno Physics"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		0,
		NULL,
		0
	);

	if ( !m_hWnd )
	{
		return false;
	}

	// Obtain a DC for this window
	HDC dc = GetDC(m_hWnd);

	// Create a pixel format descriptor that is compatible with OpenGL (WGL)
	PIXELFORMATDESCRIPTOR pfd ;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	// Find a compatible pixel format
	int nPixelFormat = ChoosePixelFormat(dc, &pfd);
	if (nPixelFormat == 0)
	{
//		TRACE( "ChoosePixelFormat Failed %d\r\n", GetLastError() );
		ReleaseDC(m_hWnd, dc);
		return false;
	}

	// Select this pixel format into the DC
	BOOL success = SetPixelFormat (dc, nPixelFormat, &pfd);
	if ( !success )
	{
//		TRACE( "SetPixelFormat Failed %d\r\n",GetLastError() );
		ReleaseDC(m_hWnd, dc);
		return false;
	}

	// Create an OpenGL (WGL) rendering context
	m_glContext = wglCreateContext(dc);
	if ( !m_glContext )
	{
//		TRACE("wglCreateContext Failed %x\r\n", GetLastError()) ;
		ReleaseDC(m_hWnd, dc);
		return false;
	}

	///
	// Initiaize GL to reasonable defaults

	wglMakeCurrent (dc, m_glContext);

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

	wglMakeCurrent(NULL, NULL);


	// Set the initial window size
	RECT r;
	GetClientRect(m_hWnd, &r);
	SetSize( r.right - r.left, r.bottom - r.top );

	SetFocus(m_hWnd);

	RegisterTestModule::FactoryMethod* factoryMethod = RegisterTestModule::GetFactoryMethod( m_moduleIndex );
	m_module = factoryMethod();

	// Initialize the test module
	success = wglMakeCurrent (dc, m_glContext);
	m_module->Init();
	wglMakeCurrent(NULL, NULL);
	ReleaseDC(m_hWnd, dc);

	LoadView();
	return true;
}

void XenoTestbedWindow::SetSize(int width, int height)
{
	HDC dc = GetDC(m_hWnd);
	BOOL success = wglMakeCurrent (dc, m_glContext);
	if ( !success )
	{
//		TRACE( "wglMakeCurrent Failed %x\r\n", GetLastError() ) ;
		ReleaseDC(m_hWnd, dc);
		return;
	}

	// Set up the projection matrix and viewport to use the new size
	GLdouble gldAspect = (GLdouble) width / (GLdouble) height;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0, gldAspect, 1.0, 1000.0);
	glViewport(0, 0, width, height);

	// Invalidate this window so it will refresh
	RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);

	ReleaseDC(m_hWnd, dc);
}

void XenoTestbedWindow::OnSize(UINT nType, int cx, int cy)
{
	if ( m_glContext )
	{
		SetSize(cx, cy);
	}
}

void XenoTestbedWindow::OnPaint()
{
	if (!m_module) return;

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hWnd, &ps);

	// Select the OpenGL (WGL) context of our window
	BOOL success = wglMakeCurrent (hdc, m_glContext);
	if ( !success )
	{
//		TRACE("wglMakeCurrent Failed %x\r\n", GetLastError() );
		return;
	}

	// Draw the scene
	DrawScene();

	// Swap buffers
	SwapBuffers(hdc);

	// Clear the OpenGL (WGL) context [makes debugging easier with multiple OpenGL windows]
	wglMakeCurrent(NULL, NULL);

	EndPaint(m_hWnd, &ps);
}

void XenoTestbedWindow::DrawScene(void)
{
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

	// Change to model view matrix stack.
	glMatrixMode(GL_MODELVIEW); 

	// Start with an identity matrix
	glLoadIdentity();

	// Translate the view back
	glTranslated( m_translation->X(), m_translation->Y(), m_translation->Z() );

	// We want to start off looking down the Y axis
	glRotatef(-90, 1, 0, 0);
	glRotatef(90, 0, 0, 1);

	// Obtain the trackball rotation and convert to axis / angle
	Quat q = m_rotation->GetRotation();
	Vector vec;
	float angle;
	(~q).ConvertToAxisAngle(&vec, &angle);

	// Convert the angle from radians to degrees
	angle = angle * 180.0f / PI;

	// Rotate the view
	glRotatef(angle, vec.X(), vec.Y(), vec.Z());

	// Draw the scene
	m_module->DrawScene();

	// Flush the drawing pipeline
	glFlush();
}

void XenoTestbedWindow::WindowPointToWorldRay(Vector* rayOrigin, Vector* rayDirection, const Point& p)
{
	HDC dc = GetDC(m_hWnd);

	BOOL success = wglMakeCurrent (dc, m_glContext);

	GLdouble modelMatrix[16];
	GLdouble projectionMatrix[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	GLenum err = glGetError();

	GLdouble x0, y0, z0;
	GLdouble x1, y1, z1;

	gluUnProject(p.x, viewport[3] - p.y, 0.0f, modelMatrix, projectionMatrix, viewport, &x0, &y0, &z0);
	gluUnProject(p.x, viewport[3] - p.y, 1.0f, modelMatrix, projectionMatrix, viewport, &x1, &y1, &z1);

	Vector diff = Vector( (float32) x1, (float32) y1, (float32) z1 ) - Vector( (float32) x0, (float32) y0, (float32) z0 );
	diff.Normalize3();

	*rayDirection = diff;
	*rayOrigin = Vector((float32)x0, (float32)y0, (float32)z0); //*m_translation;

	// Clear the OpenGL (WGL) context [makes debugging easier with multiple OpenGL windows]
	wglMakeCurrent(NULL, NULL);
	ReleaseDC(m_hWnd, dc);
}

void XenoTestbedWindow::OnLButtonDown(UINT nFlags, Point point)
{
	m_lastMousePoint = point;
	if (GetKeyState(VK_MENU) & 0x80000000)
	{
		m_leftButtonDown = true;
		SetCapture(m_hWnd);
		m_captureCount++;
	}
	else
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnLeftButtonDown(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnLButtonUp(UINT nFlags, Point point)
{
	if (m_leftButtonDown)
	{
		m_leftButtonDown = false;
		if (--m_captureCount == 0) ReleaseCapture();
	}
	else
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnLeftButtonUp(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnMButtonDown(UINT nFlags, Point point)
{
	m_lastMousePoint = point;
	if (GetKeyState(VK_MENU) & 0x80000000)
	{
		m_middleButtonDown = true;
		SetCapture(m_hWnd);
		m_captureCount++;
	}
	else
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnMiddleButtonDown(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnMButtonUp(UINT nFlags, Point point)
{
	if (m_middleButtonDown)
	{
		m_middleButtonDown = false;
		if (--m_captureCount == 0) ReleaseCapture();
	}
	else
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnMiddleButtonUp(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnRButtonDown(UINT nFlags, Point point)
{
	m_lastMousePoint = point;
	if (GetKeyState(VK_MENU) & 0x80000000)
	{
		m_rightButtonDown = true;
		SetCapture(m_hWnd);
		m_captureCount++;
	}
	else
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnRightButtonDown(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnRButtonUp(UINT nFlags, Point point)
{
	if (m_rightButtonDown)
	{
		m_rightButtonDown = false;
		if (--m_captureCount == 0) ReleaseCapture();
	}
	else
	{
		Vector rayOrigin;
		Vector rayDirection;
		WindowPointToWorldRay(&rayOrigin, &rayDirection, point);
		m_module->OnRightButtonUp(rayOrigin, rayDirection, Vector(point.x, point.y, 0));
	}
}

void XenoTestbedWindow::OnMouseMove(UINT nFlags, Point point)
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

	// Force a redraw
	RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);

}

void XenoTestbedWindow::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Forward the keystroke to the module
	m_module->OnChar(nChar);
}

void XenoTestbedWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Handle pg-up and pg-down
	if (nChar == 33)
	{
		m_moduleIndex--;

		if (m_moduleIndex < 0)
			m_moduleIndex = RegisterTestModule::ModuleCount() - 1;

		HDC dc = GetDC(m_hWnd);
		BOOL success = wglMakeCurrent (dc, m_glContext);

		delete m_module;

		m_module = RegisterTestModule::GetFactoryMethod(m_moduleIndex)();
		m_module->Init();

		wglMakeCurrent(NULL, NULL);
		ReleaseDC(m_hWnd, dc);
	}
	else if (nChar == 34)
	{
		m_moduleIndex++;

		if (m_moduleIndex >= RegisterTestModule::ModuleCount())
			m_moduleIndex = 0;

		HDC dc = GetDC(m_hWnd);
		BOOL success = wglMakeCurrent (dc, m_glContext);

		delete m_module;
		m_module = RegisterTestModule::GetFactoryMethod(m_moduleIndex)();

		m_module->Init();

		wglMakeCurrent(NULL, NULL);
		ReleaseDC(m_hWnd, dc);
	}
	else if (nChar == 'K')
	{
		// Control must be held
		if ( !(GetKeyState(VK_LCONTROL) & 0x8000) && !(GetKeyState(VK_RCONTROL) & 0x8000) )
		{
			return;
		}

		SaveView();
	}
	else if (nChar == 'L')
	{
		// Control must be held
		if ( !(GetKeyState(VK_LCONTROL) & 0x8000) && !(GetKeyState(VK_RCONTROL) & 0x8000) )
		{
			return;
		}

		LoadView();
	}
	else
	{
		// Forward the keystroke to the module
		m_module->OnKeyDown(nChar);
	}
}

static Point MakePoint(LPARAM lParam)
{
	return Point( (int16)LOWORD(lParam), (int16)HIWORD(lParam) );
}

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
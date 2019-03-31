/*
* Copyright (c) 2006-2016 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "imgui-framework.h"
#include "Test.h"
#include <stdio.h>

DebugDraw g_debugDraw;

DebugCamera g_camera;

void DebugCamera::Apply() const
{
	gxTranslatef(1024/2, 640/2, 0);
	gxScalef(1, -1, 1);
	gxScalef(640/50.f, 640/50.f, 1);
	gxScalef(g_camera.m_zoom, g_camera.m_zoom, 1);
	gxTranslatef(-g_camera.m_center.x, -g_camera.m_center.y, 0);
}

b2Vec2 DebugCamera::ConvertScreenToWorld(b2Vec2 v) const
{
	v -= b2Vec2(1024/2, 640/2);
	
	v.y = -v.y;
	
	v.x /= 640/50.f;
	v.y /= 640/50.f;
	
	v.x /= g_camera.m_zoom;
	v.y /= g_camera.m_zoom;
	
	v += g_camera.m_center;
	
	return v;
}

//
struct UIState
{
	bool showMenu;
};

//
namespace
{
	UIState ui;

	int32 testIndex = 0;
	int32 testSelection = 0;
	int32 testCount = 0;
	TestEntry* entry;
	Test* test;
	Settings settings;
	bool rightMouseDown;
	b2Vec2 lastp;
}

//
static void sCreateUI()
{
	ui.showMenu = true;

	// Init UI
	const char* fontPath = "Data/DroidSans.ttf";
	ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath, 15.f);
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = style.GrabRounding = style.ScrollbarRounding = 2.0f;
	style.FramePadding = ImVec2(4, 2);
	style.DisplayWindowPadding = ImVec2(0, 0);
	style.DisplaySafeAreaPadding = ImVec2(0, 0);
}

//
static void sTickKeyboard()
{
	bool keys_for_ui = ImGui::GetIO().WantCaptureKeyboard;
	if (keys_for_ui)
		return;

	if (keyboard.wentDown(SDLK_ESCAPE))
	{
		// Quit
		framework.quitRequested = true;
	}

	if (keyboard.wentDown(SDLK_LEFT))
	{
		// Pan left
		if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL))
		{
			b2Vec2 newOrigin(2.0f, 0.0f);
			test->ShiftOrigin(newOrigin);
		}
		else
		{
			g_camera.m_center.x -= 0.5f;
		}
	}

	if (keyboard.wentDown(SDLK_RIGHT))
	{
		// Pan right
		if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL))
		{
			b2Vec2 newOrigin(-2.0f, 0.0f);
			test->ShiftOrigin(newOrigin);
		}
		else
		{
			g_camera.m_center.x += 0.5f;
		}
	}

	if (keyboard.wentDown(SDLK_DOWN))
	{
		// Pan down
		if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL))
		{
			b2Vec2 newOrigin(0.0f, 2.0f);
			test->ShiftOrigin(newOrigin);
		}
		else
		{
			g_camera.m_center.y -= 0.5f;
		}
	}

	if (keyboard.wentDown(SDLK_UP))
	{
		// Pan up
		if (keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL))
		{
			b2Vec2 newOrigin(0.0f, -2.0f);
			test->ShiftOrigin(newOrigin);
		}
		else
		{
			g_camera.m_center.y += 0.5f;
		}
	}

	if (keyboard.wentDown(SDLK_HOME))
	{
		// Reset view
		g_camera.m_zoom = 1.0f;
		g_camera.m_center.Set(0.0f, 20.0f);
	}

	if (keyboard.wentDown(SDLK_z))
	{
		// Zoom out
		g_camera.m_zoom = b2Min(1.1f * g_camera.m_zoom, 20.0f);
	}

	if (keyboard.wentDown(SDLK_x))
	{
		// Zoom in
		g_camera.m_zoom = b2Max(0.9f * g_camera.m_zoom, 0.02f);
	}

	if (keyboard.wentDown(SDLK_r))
	{
		// Reset test
		delete test;
		test = entry->createFcn();
	}

	if (keyboard.wentDown(SDLK_SPACE))
	{
		// Launch a bomb.
		if (test)
		{
			test->LaunchBomb();
		}
	}

	if (keyboard.wentDown(SDLK_o))
	{
		settings.singleStep = true;
	}

	if (keyboard.wentDown(SDLK_p))
	{
		settings.pause = !settings.pause;
	}

	if (keyboard.wentDown(SDLK_LEFTBRACKET))
	{
		// Switch to previous test
		--testSelection;
		if (testSelection < 0)
		{
			testSelection = testCount - 1;
		}
	}

	if (keyboard.wentDown(SDLK_RIGHTBRACKET))
	{
		// Switch to next test
		++testSelection;
		if (testSelection == testCount)
		{
			testSelection = 0;
		}
	}

	if (keyboard.wentDown(SDLK_TAB))
		ui.showMenu = !ui.showMenu;
	
	auto toGlfwKey = [](const int key) -> GLFW_KEY
	{
		if (key >= SDLK_a && key <= SDLK_z)
			return GLFW_KEY(GLFW_KEY_A + (key - SDLK_a));
		if (key >= SDLK_0 && key <= SDLK_9)
			return GLFW_KEY(GLFW_KEY_0 + (key - SDLK_0));
		
		if (key == SDLK_LEFT)
			return GLFW_KEY_LEFT;
		if (key == SDLK_RIGHT)
			return GLFW_KEY_RIGHT;
		if (key == SDLK_ESCAPE)
			return GLFW_KEY_ESCAPE;
		if (key == SDLK_COMMA)
			return GLFW_KEY_COMMA;
		
		return GLFW_KEY_UNKNOWN;
	};
	
	for (auto & e : keyboard.events)
	{
		const GLFW_KEY key = toGlfwKey(e.key.keysym.sym);
		
		if (key != GLFW_KEY_UNKNOWN)
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (test)
 					test->Keyboard(key);
			}
			else if (e.type == SDL_KEYUP)
			{
				if (test)
					test->KeyboardUp(key);
			}
		}
	}
}

//
static void sTickMouse()
{
	b2Vec2 ps(mouse.x, mouse.y);

	//ps.Set(0, 0);
	b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
	
	// Use the mouse to move things around.
	if (mouse.wentDown(BUTTON_LEFT))
	{
		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
		{
			test->ShiftMouseDown(pw);
		}
		else
		{
			test->MouseDown(pw);
		}
	}
	
	if (mouse.wentUp(BUTTON_LEFT))
	{
		test->MouseUp(pw);
	}
	
	if (mouse.wentDown(BUTTON_RIGHT))
	{
		lastp = g_camera.ConvertScreenToWorld(ps);
		rightMouseDown = true;
	}

	if (mouse.wentUp(BUTTON_RIGHT))
	{
		rightMouseDown = false;
	}
}

//
static void sTickMouseMotion()
{
	b2Vec2 ps(mouse.x, mouse.y);

	b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
	test->MouseMove(pw);
	
	if (rightMouseDown)
	{
		b2Vec2 diff = pw - lastp;
		g_camera.m_center.x -= diff.x;
		g_camera.m_center.y -= diff.y;
		lastp = g_camera.ConvertScreenToWorld(ps);
	}
}

//
static void sTickMouseScroll()
{
	bool mouse_for_ui = ImGui::GetIO().WantCaptureMouse;

	if (!mouse_for_ui)
	{
		g_camera.m_zoom *= powf(1.1f, mouse.scrollY);
	}
}

//
static void sRestart()
{
	delete test;
	entry = g_testEntries + testIndex;
	test = entry->createFcn();
}

//
static void sSimulate()
{
	pushDepthTest(true, DEPTH_LESS);
	test->Step(&settings);

	test->DrawTitle(entry->name);
	popDepthTest();

	if (testSelection != testIndex)
	{
		testIndex = testSelection;
		delete test;
		entry = g_testEntries + testIndex;
		test = entry->createFcn();
		g_camera.m_zoom = 1.0f;
		g_camera.m_center.Set(0.0f, 20.0f);
	}
}

//
static bool sTestEntriesGetName(void*, int idx, const char** out_name)
{
	*out_name = g_testEntries[idx].name;
	return true;
}

//
static void sInterface()
{
	int menuWidth = 200;
	if (ui.showMenu)
	{
		ImGui::SetNextWindowPos(ImVec2((float)g_camera.m_width - menuWidth - 10, 10));
		ImGui::SetNextWindowSize(ImVec2((float)menuWidth, (float)g_camera.m_height - 20));
		ImGui::Begin("Testbed Controls", &ui.showMenu, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
		ImGui::PushAllowKeyboardFocus(false); // Disable TAB

		ImGui::PushItemWidth(-1.0f);

		ImGui::Text("Test");
		if (ImGui::Combo("##Test", &testIndex, sTestEntriesGetName, NULL, testCount, testCount))
		{
			delete test;
			entry = g_testEntries + testIndex;
			test = entry->createFcn();
			testSelection = testIndex;
		}
		ImGui::Separator();

		ImGui::Text("Vel Iters");
		ImGui::SliderInt("##Vel Iters", &settings.velocityIterations, 0, 50);
		ImGui::Text("Pos Iters");
		ImGui::SliderInt("##Pos Iters", &settings.positionIterations, 0, 50);
		ImGui::Text("Hertz");
		ImGui::SliderFloat("##Hertz", &settings.hz, 5.0f, 120.0f, "%.0f hz");
		ImGui::PopItemWidth();

		ImGui::Checkbox("Sleep", &settings.enableSleep);
		ImGui::Checkbox("Warm Starting", &settings.enableWarmStarting);
		ImGui::Checkbox("Time of Impact", &settings.enableContinuous);
		ImGui::Checkbox("Sub-Stepping", &settings.enableSubStepping);

		ImGui::Separator();

		ImGui::Checkbox("Shapes", &settings.drawShapes);
		ImGui::Checkbox("Joints", &settings.drawJoints);
		ImGui::Checkbox("AABBs", &settings.drawAABBs);
		ImGui::Checkbox("Contact Points", &settings.drawContactPoints);
		ImGui::Checkbox("Contact Normals", &settings.drawContactNormals);
		ImGui::Checkbox("Contact Impulses", &settings.drawContactImpulse);
		ImGui::Checkbox("Friction Impulses", &settings.drawFrictionImpulse);
		ImGui::Checkbox("Center of Masses", &settings.drawCOMs);
		ImGui::Checkbox("Statistics", &settings.drawStats);
		ImGui::Checkbox("Profile", &settings.drawProfile);

		ImVec2 button_sz = ImVec2(-1, 0);
		if (ImGui::Button("Pause (P)", button_sz))
			settings.pause = !settings.pause;

		if (ImGui::Button("Single Step (O)", button_sz))
			settings.singleStep = !settings.singleStep;

		if (ImGui::Button("Restart (R)", button_sz))
			sRestart();

		if (ImGui::Button("Quit", button_sz))
			framework.quitRequested = true;

		ImGui::PopAllowKeyboardFocus();
		ImGui::End();
	}

	//ImGui::ShowTestWindow(NULL);
}

//
void glfwErrorCallback(int error, const char *description)
{
	fprintf(stderr, "GLFW error occured. Code: %d. Description: %s\n", error, description);
}

//
int main(int, char**)
{
#if defined(_WIN32)
	// Enable memory-leak reports
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

	changeDirectory(CHIBI_RESOURCE_PATH);

	g_camera.m_width = 1024;
	g_camera.m_height = 640;

	framework.enableDepthBuffer = true;
	
	if (!framework.init(1024, 640))
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	char title[64];
	sprintf(title, "Box2D Testbed Version %d.%d.%d", b2_version.major, b2_version.minor, b2_version.revision);
	
	FrameworkImGuiContext guiContext;
	guiContext.init();

	sCreateUI();

	testCount = 0;
	while (g_testEntries[testCount].createFcn != NULL)
	{
		++testCount;
	}

	testIndex = b2Clamp(testIndex, 0, testCount - 1);
	testSelection = testIndex;

	entry = g_testEntries + testIndex;
	test = entry->createFcn();

	double frameTime = 0.0;
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		sTickKeyboard();
		sTickMouse();
		sTickMouseMotion();
		sTickMouseScroll();
		
		framework.beginDraw(75, 75, 75, 255);
		{
			bool inputIsCaptured = false;
			guiContext.processBegin(framework.timeStep, 1024, 640, inputIsCaptured);
			{
				ImGui::SetNextWindowPos(ImVec2(0,0));
				ImGui::SetNextWindowSize(ImVec2((float)g_camera.m_width, (float)g_camera.m_height));
				ImGui::Begin("Overlay", NULL, ImVec2(0,0), 0.0f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoScrollbar);
				ImGui::SetCursorPos(ImVec2(5, (float)g_camera.m_height - 20));
				ImGui::Text("%.1f ms", 1000.0 * frameTime);
				ImGui::End();

				gxPushMatrix();
				g_camera.Apply();
				
				sSimulate();
				sInterface();
				
				gxPopMatrix();
			}
			guiContext.processEnd();

			// Measure speed
			double alpha = 0.9;
			frameTime = alpha * frameTime + (1.0 - alpha) * framework.timeStep;

			guiContext.draw();
		}
		framework.endDraw();
	}

	if (test)
	{
		delete test;
		test = NULL;
	}

	guiContext.shut();
	
	framework.shutdown();

	return 0;
}

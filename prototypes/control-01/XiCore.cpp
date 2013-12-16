#include "Renderer.h"
#include "XiCore.h"

namespace Xi
{
	Core::Core()
	{
		Position = Vec2T(240.0f, 160.0f);
		//Position = Vec2T(0.0f, 0.0f);
		Root = new Link();
	}

	void Core::Update()
	{
		g_Renderer.PushT(Position.v);

		Root->Update();

		g_Renderer.Pop();
	}

	void Core::RenderSB(SelectionBuffer* sb)
	{
		Root->RenderSB(sb);
	}

	void Core::Render(BITMAP* buffer)
	{
		g_Renderer.PushT(Position.v);

		Root->Render(buffer);

		g_Renderer.Pop();
	}
};
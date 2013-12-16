#include "libgui_precompiled.h"
#include "Gui.h"

#if 0
namespace Gui
{
	Manager& Manager::I()
	{
		static Manager manager;
		return manager;
	}

	GuiResult Manager::Initialize(Engine::ROLE kernelRole)
	{
		GuiResult result = true;

		/*
		if (Network::SubmitManager::I().Initialize(kernelMode) != true)
			result = false;
		if (Network::RequestManager::I().Initialize(kernelMode) != true)
			result = false;
		if (Network::CommandManager::I().Initialize(kernelMode) != true)
			result = false;*/

		return result;
	}

	GuiResult Manager::Shutdown()
	{
		GuiResult result = true;

		/*
		if (Network::SubmitManager::I().Shutdown() != true)
			result = false;
		if (Network::RequestManager::I().Shutdown() != true)
			result = false;
		if (Network::CommandManager::I().Shutdown() != true)
			result = false;*/

		return result;
	}

	GuiResult Manager::Update()
	{
		GuiResult result = true;

		/*
		if (Network::RequestManager::I().Update() != true)
			result = false;*/

		return result;
	}
};
#endif

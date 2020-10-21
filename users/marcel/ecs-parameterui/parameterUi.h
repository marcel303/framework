#pragma once

#include <string>

struct ParameterBase;
struct ParameterMgr;
struct ParameterComponentMgr;

namespace parameterUi
{
	// todo : open folders and scroll to parameter when a single parameter matches the address filter
	// todo : option + click to unfold all underlying folders

	enum Flags
	{
		kFlag_ShowDefaultButton = 1 << 0, // Show a 'Default' button in front of each parameter, which will reset the parameter back to its default value.
		kFlag_ShowDeveloperTooltip = 1 << 1 // Show a tooltip when hovering over parameters with some detailed information about the parameter.
	};

	/**
	 * Callback function invoked when a user right-clicks on a parameter.
	 * @param stack The stack of parameterMgrs traversed to reach the current parameterMgr.
	 * @param stackSize The number of elements inside the stack.
	 * @param parameterMgr The parameterMgr the parameter belongs to or the parameterMgr that was right-clicked.
	 * @param parameterBase The parameter that was right-clicked. When nullptr, the right-click interaction occured on the parameterMgr itself.
	 */
	typedef void (*ContextMenu)(ParameterMgr ** stack, const int stackSize, ParameterMgr & parameterMgr, ParameterBase * parameterBase);

	// functions to customize how the UI is presented

	void pushFlags(const int flags);
	void popFlags();

	void pushContextMenu(ContextMenu contextMenu);
	void popContextMenu();

	// functions for drawing the UI

	void doParameterUi(ParameterBase & parameter);
	void doParameterUi(ParameterMgr & parameterMgr, const char * filter, const bool showCollapsingHeader);
	void doParameterUi_recursive(ParameterMgr & parameterMgr, const char * filter);

	void doParameterTooltipUi(ParameterBase & parameter);

	// helper functions for copy/pasting changed parameters to/from clipboard or text

	void parametersToText_recursive(ParameterMgr * const parameterMgr, const char * filter, std::string & out_text);
	void parametersFromText_recursive(ParameterMgr * const parameterMgr, const char * text, const char * filter);
	
	void parametersToClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter);
	void parametersFromClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter);
}

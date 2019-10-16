#pragma once

struct ParameterBase;
struct parameterMgr;
struct ParameterComponentMgr;

// todo : open folders and scroll to parameter when a single parameter matches the address filter
// todo : show closed folders when a folder has matching parameters
// todo : option + click to unfold all underlying folders

enum ParameterUiFlags
{
	kParameterUiFlag_ShowDefaultButton = 1 << 0, // Shows a 'Default' button in front of each parameter, which will reset the parameter back to its default value.
	kParameterUiFlag_ShowDeveloperTooltip = 1 << 1 // Shows a tooltip when hovering over parameters with some detailed information about the parameter.
};

typedef void (*ParameterMgrContextMenu)(ParameterMgr & rootMgr, ParameterMgr & parameterMgr);

// functions to customize how the UI is presented

void pushParameterUiFlags(const int flags);
void popParameterUiFlags();

void pushParameterUiContextMenu(ParameterMgrContextMenu contextMenu);
void popParameterUiContextMenu();

// functions for drawing the UI

void doParameterUi(ParameterBase & parameter);
void doParameterUi(ParameterMgr & parameterMgr, const char * filter, const bool showCollapsingHeader);
void doParameterUi_recursive(ParameterMgr & parameterMgr, const char * filter);

void doParameterTooltipUi(ParameterBase & parameter);

// helper functions for copy/pasting changed parameters to/from clipboard or text

void copyParametersToText_recursive(ParameterMgr * const parameterMgr, const char * filter, std::string & out_text);
void copyParametersToClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter);

void pasteParametersFromText_recursive(ParameterMgr * const parameterMgr, const char * text, const char * filter);
void pasteParametersFromClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter);

#pragma once

#include "BitmapMem.h"
#include "BrushCache.h"
#include "ColBuf.h"
#include "Dirty.h"
#include "Brush.h"
#include "Traveller.h"
#include "Input.h"
#include "Log.h"

namespace Paint
{
	enum AppScreen
	{
		AppScreen_Canvas,
		AppScreen_ColorPicker,
		AppScreen_ToolSelect,
		AppScreen_StylusCalibrate,
		AppScreen_About
	};
	
	enum ToolType
	{
		ToolType_Brush,
		ToolType_Smudge,
		ToolType_Smoothe
	};
	
	class Application
	{
	public:
		Application(MemBitmap* bitmap);
		void UpdateBrush();

		void Run(); // todo: make Application event based.
		
		static void HandleInput(void* obj, void* arg);
		static void HandleDirty(void* obj, void* arg);
		static void HandleTravel(void* obj, void* arg);
		static void HandlePaint(void* obj, void* arg);

		MemBitmap* m_Bitmap;   // Memory bitmap. Contains the image's representation in device RGB values.
		ColBuf m_ColBuf;       // Colour buffer. Contains HQ representation of the image.
		Dirty m_Dirty;         // Dirty area manager.
		Brush m_Brush;         // Paint brush.
		Traveller m_Traveller; // Our beloved travel agent.
		InputMgr m_InputMgr;   // Handles raw input events and emits our own input events. 
		LogCtx m_Log;
		CallBack m_DirtyCB;
		
		ToolType m_ToolType;
		Col m_Col;
		Fix m_Opacity;
		
		//AppScreen m_AppScreen;
		int m_StatDirty;
	};
};
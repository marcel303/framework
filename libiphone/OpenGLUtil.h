#pragma once

#include "Log.h"
#include "Res.h"
#include "OpenGLCompat.h"

class OpenGLUtil
{
public:
	static void SetTexture(IRes* texture);
	
	static void CreateTexture(IRes* texture);
	static void DestroyTexture(IRes* texture);
	
	//
	
	static void HandleMemoryWarning(void* obj, void* arg);
	
private:
	static LogCtx m_Log;
};

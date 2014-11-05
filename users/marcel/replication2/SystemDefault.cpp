//#include "GraphicsDeviceD3D9.h"
#include "GraphicsDeviceGL.h"
#include "ResLoaderFont.h"
#include "ResLoaderPS.h"
#include "ResLoaderShader.h"
#include "ResLoaderSnd.h"
#include "ResLoaderTex.h"
#include "ResLoaderVS.h"
#include "ResMgr.h" // TODO: Remove?
#include "SoundDevice.h"
#include "SystemDefault.h"

void SystemDefault::RegisterFileSystems()
{
}

void SystemDefault::RegisterResourceLoaders()
{
	ResMgr::I().RegisterLoader(RES_FONT, new ResLoaderFont);
	ResMgr::I().RegisterLoader(RES_TEX, new ResLoaderTex);
	ResMgr::I().RegisterLoader(RES_VS, new ResLoaderVS);
	ResMgr::I().RegisterLoader(RES_PS, new ResLoaderPS);
	ResMgr::I().RegisterLoader(RES_SHADER, new ResLoaderShader);
	ResMgr::I().RegisterLoader(RES_SND, new ResLoaderSnd);
}

GraphicsDevice* SystemDefault::CreateGraphicsDevice()
{
	//return new GraphicsDeviceD3D9();
	return new GraphicsDeviceGL();
}

SoundDevice* SystemDefault::CreateSoundDevice()
{
	return new SoundDevice();
}

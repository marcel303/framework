#include "etherdream.h"
#include "framework.h"
#include <map>

struct DacInfo
{
	int connectTimer = 0;
	
	bool connected = false;
};

std::map<int, DacInfo> s_dacInfos;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	const bool isEtherdreamStarted = etherdream_lib_start() == 0;
	
	while (!framework.quitRequested)
	{
		SDL_Delay(10);
		
		framework.process();
		
		if (isEtherdreamStarted)
		{
			const int numDACs = etherdream_dac_count();
			//logDebug("num DACs: %d", numDACs);
			
			for (int i = 0; i < numDACs; ++i)
			{
				etherdream * e = etherdream_get(i);
				
				DacInfo & dacInfo = s_dacInfos[i];
				
				if (dacInfo.connected == false)
				{
					if (dacInfo.connectTimer == 0)
					{
						dacInfo.connectTimer = 1000;
						
						if (etherdream_connect(e) == 0)
						{
							dacInfo.connected = true;
						}
					}
				}
				
				if (etherdream_is_ready(e))
				{
					//logDebug("DAC is ready. index: %d", i);
					
					const int pps = 30000;
					const int repeatCount = 1;
					
					const int numPoints = 500; // 30000/60 = 500 (60fps)
					
					etherdream_point pts[numPoints];
					
					/*
					struct etherdream_point {
						int16_t x;
						int16_t y;
						uint16_t r;
						uint16_t g;
						uint16_t b;
						uint16_t i;
						uint16_t u1;
						uint16_t u2;
					};
					*/
					
					for (int i = 0; i < numPoints; ++i)
					{
						auto & p = pts[i];
						
						const float t = i / float(numPoints - 1);
						const float a = t * 2.f * float(M_PI);
						
						const float r = lerp(.6f + .2f * sinf(framework.time + a * 3.f), 1.f, (sinf(a * 10.f - framework.time * .6f) + 1.f) / 2.f);
						//const float r = 1.f;
						
						p.x = (int)roundf(cosf(a) * r * 16000.f);
						p.y = (int)roundf(sinf(a) * r * 16000.f);
						
						const float intensity = .15f;
						
						p.r = (sinf(a + 2.f * float(M_PI) * 0.f / 3.f) + 1.f) / 2.f * intensity * ((1 << 16) - 1);
						p.g = (sinf(a + 2.f * float(M_PI) * 1.f / 3.f) + 1.f) / 2.f * intensity * ((1 << 16) - 1);
						p.b = (sinf(a + 2.f * float(M_PI) * 2.f / 3.f) + 1.f) / 2.f * intensity * ((1 << 16) - 1);
						p.i = (1 << 16) - 1;
						p.u1 = 0; // unused
						p.u2 = 0;
					}
					
					const int result = etherdream_write(
						e,
						pts,
                     	500,
                     	pps,
                     	repeatCount);
					
					if (result != 0)
					{
						logDebug("etherdream_write failed with code %d", result);
					}
				}
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
		}
		framework.endDraw();
	}
	
	return 0;
}

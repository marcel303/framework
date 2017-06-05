#include "framework.h"
#include "vfxNodeTest.h"

extern const int GFX_SX;
extern const int GFX_SY;

VfxNodeTest::VfxNodeTest()
	: VfxNodeBase()
	, anyOutput(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addOutput(kOutput_Any, kVfxPlugType_Int, &anyOutput);
}

VfxNodeTest::~VfxNodeTest()
{
}

void VfxNodeTest::tick(const float dt)
{
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	
	if (channels && channels->numChannels >= 2)
	{
		auto & x = channels->channels[0];
		auto & y = channels->channels[1];
		//auto & z = channels->channels[2];
		
		gxPushMatrix();
		{
			gxTranslatef(GFX_SX/2, GFX_SY/2, 0);

			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < channels->size; ++i)
				{
					setColor(colorWhite);
					//hqFillCircle(x.data[i], y.data[i], 100);
					hqFillCircle(x.data[i] * 10, y.data[i] * 10, 3);
				}
			}
			hqEnd();
		}
		gxPopMatrix();
	}
}

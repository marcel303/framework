#include "framework.h"
#include "oscReceiver.h"
#include <math.h>
#include <string.h>

struct Timeline
{
	double length = 0.0;
	double time = 0.0;
	double volume = 0.0;
};


struct MyOscReceiveHandler : OscReceiveHandler
{
	Timeline * timeline = nullptr;
	
	MyOscReceiveHandler(Timeline * in_timeline)
		: timeline(in_timeline)
	{
	}
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override final
	{
		if (strcmp(m.AddressPattern(), "/time") == 0)
		{
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				auto & a = *i;
				
				if (a.IsFloat())
					timeline->time = a.AsFloatUnchecked();
				else if (a.IsDouble())
					timeline->time = a.AsDoubleUnchecked();
			}
			
			timeline->length = fmax(timeline->length, timeline->time);
		}
		else if (strcmp(m.AddressPattern(), "/1/volume") == 0)
		{
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				auto & a = *i;
				
				if (a.IsFloat())
					timeline->volume = a.AsFloatUnchecked();
				else if (a.IsDouble())
					timeline->volume = a.AsDoubleUnchecked();
			}
		}
	}
};

int main(int argc, char * argv[])
{
	if (!framework.init(600, 200))
		return -1;
	
	OscReceiver oscReceiver;
	oscReceiver.init("127.0.0.1", 4002);
	
	Timeline timeline;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		MyOscReceiveHandler oscReceiveHandler(&timeline);
		
		oscReceiver.flushMessages(&oscReceiveHandler);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setAlpha(255);
			
			if (timeline.length > 0.0)
			{
				const double scale = 600 / timeline.length;
				
				gxPushMatrix();
				{
					gxScalef(scale, 200, 1);
					gxTranslatef(0, 1, 0);
					gxScalef(1, -1, 1);
					
					setLumi(100);
					drawRect(0, 0, timeline.length, 1);
					
					setLumi(200);
					drawLine(timeline.time, 0, timeline.time, 1);
					
					setLumi(240);
					drawLine(0, timeline.volume, timeline.length, timeline.volume);
				}
				gxPopMatrix();
			}
		}
		framework.endDraw();
	}
	
	oscReceiver.shut();
	
	framework.shutdown();
	
	return 0;
}

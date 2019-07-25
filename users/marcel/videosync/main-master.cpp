#include "framework.h"
#include "oscReceiver.h"
#include "oscSender.h"
#include "reflection.h"
#include "reflection-bindtofile.h"

struct SlaveInfo
{
	std::string oscSendAddress = "127.0.0.1";
	int oscSendPort = 2002;
	
	static void reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<SlaveInfo>("SlaveInfo")
			.add("oscSendAddress", &SlaveInfo::oscSendAddress)
			.add("oscReceivePort", &SlaveInfo::oscSendPort);
	}
};

struct Settings
{
	int oscReceivePort = 2000;
	
	std::vector<SlaveInfo> slaves;
	
	static void reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<Settings>("Settings")
			.add("oscReceivePort", &Settings::oscReceivePort)
			.add("slaves", &Settings::slaves);
	}
};

struct MyOscReceiveHandler : OscReceiveHandler
{
	bool isStarted = false;
	float time = 0.f;
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		if (strcmp(m.AddressPattern(), "/start") == 0)
		{
			for (auto arg_itr = m.ArgumentsBegin(); arg_itr != m.ArgumentsEnd(); ++arg_itr)
			{
				auto & arg = *arg_itr;
				
				if (arg.IsBool())
					isStarted = arg.AsBoolUnchecked();
				if (arg.IsInt32())
					isStarted = arg.AsInt32Unchecked();
				if (arg.IsFloat())
					isStarted = arg.AsFloatUnchecked();
			}
		}
		else if (strcmp(m.AddressPattern(), "/stop") == 0)
		{
			for (auto arg_itr = m.ArgumentsBegin(); arg_itr != m.ArgumentsEnd(); ++arg_itr)
			{
				auto & arg = *arg_itr;
				
				if (arg.IsBool())
					isStarted = 0 == arg.AsBoolUnchecked();
				if (arg.IsInt32())
					isStarted = 0 == arg.AsInt32Unchecked();
				if (arg.IsFloat())
					isStarted = 0 == arg.AsFloatUnchecked();
			}
		}
		else if (strcmp(m.AddressPattern(), "/time") == 0)
		{
			for (auto arg_itr = m.ArgumentsBegin(); arg_itr != m.ArgumentsEnd(); ++arg_itr)
			{
				auto & arg = *arg_itr;
				
				if (arg.IsInt32())
				{
					time = arg.AsInt32Unchecked();
				}
				else if (arg.IsFloat())
				{
					time = arg.AsFloatUnchecked();
				}
				else if (arg.IsDouble())
				{
					time = arg.AsDoubleUnchecked();
				}
			}
		}
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif
	
	TypeDB typeDB;
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<std::string>("string", kDataType_String);
	SlaveInfo::reflect(typeDB);
	Settings::reflect(typeDB);
	
	Settings settings;
	bindObjectToFile(&typeDB, &settings, "settings.json");
	
	//
	
	framework.windowIsResizable = true;
	
	if (!framework.init(400, 100))
		return -1;
	
	//
	
	OscReceiver oscReceiver;
	if (!oscReceiver.init("255.255.255.255", settings.oscReceivePort))
		logError("failed to initialzie OSC receiver");
	
	std::vector<OscReceiver> oscSenders;
	oscSenders.resize(settings.slaves.size());
	for (size_t i = 0; i < settings.slaves.size(); ++i)
		if (!oscSenders[i].init(settings.slaves[i].oscSendAddress.c_str(), settings.slaves[i].oscSendPort))
			logError("failed to initialzie OSC sender");
	
	MyOscReceiveHandler oscReceiveHandler;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		oscReceiver.flushMessages(&oscReceiveHandler);
		
		framework.beginDraw(0, 0, 0, 0);
		{
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}

#pragma once

#if !defined(DEPLOYMENT) && defined(PSP)
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet/sys/socket.h>
#include <string.h>
#include <utility/utility_module.h>
#define PSPNET_POOLSIZE (512 * 1024)
#define CALLOUT_SPL 30
#define NETINTR_SPL 30
class LogUdp
{
public:
	static int mSocket;
	static struct SceNetInetSockaddrIn mTargetAddr;
	static bool mFailed;

	static void Init()
	{
		if (mFailed || mSocket >= 0)
			return;

		mFailed = true;

		if (sceUtilityLoadModule(SCE_UTILITY_MODULE_NET_COMMON) < 0)
			return;
		if (sceUtilityLoadModule(SCE_UTILITY_MODULE_NET_INET) < 0)
			return;

		if (sceNetInit(PSPNET_POOLSIZE, CALLOUT_SPL, 0, NETINTR_SPL, 0) < 0)
			return;
		if (sceNetInetInit() < 0)
			return;

		mSocket = sceNetInetSocket(SCE_NET_INET_AF_INET, SCE_NET_INET_SOCK_DGRAM, 0);
		if (mSocket < 0)
		{
			int error = sceNetInetGetPspError();
			return;
		}

		memset(&mTargetAddr, 0, sizeof(mTargetAddr));

		mTargetAddr.sin_len = sizeof(mTargetAddr);
		mTargetAddr.sin_family = SCE_NET_INET_AF_INET;
		mTargetAddr.sin_port = sceNetNtohs(16571);
		if (sceNetInetInetAton("192.168.1.110", &mTargetAddr.sin_addr) < 0)
			return;

		mFailed = false;
	}

	static void Send(const char* text)
	{
		if (mSocket < 0)
			return;

		if (sceNetInetSendto(mSocket, text, strlen(text), SCE_NET_INET_MSG_DONTWAIT, (const SceNetInetSockaddr*)&mTargetAddr, sizeof(mTargetAddr)) < 0)
		{
			int error = sceNetInetGetPspError();
			return;
		}
	}
};
int LogUdp::mSocket = -1;
struct SceNetInetSockaddrIn LogUdp::mTargetAddr;
bool LogUdp::mFailed = false;
#endif

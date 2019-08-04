#pragma once

struct ParameterBase;
struct ParameterMgr;

namespace osc
{
	class ReceivedMessage;
}

bool handleOscMessage(ParameterBase * parameterBase, const osc::ReceivedMessage & m);

bool handleOscMessage(ParameterMgr & paramMgr, const osc::ReceivedMessage & m, const char * addressPattern);

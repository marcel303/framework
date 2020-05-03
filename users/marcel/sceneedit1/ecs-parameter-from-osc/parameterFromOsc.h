#pragma once

struct ParameterBase;
struct ParameterMgr;

namespace osc
{
	class ReceivedMessage;
}

/**
 * Decode the parameter value contained inside the OSC message.
 * @return True upon success. False when decoding the message failed.
 */
bool parameterFromOsc(ParameterBase * parameterBase, const osc::ReceivedMessage & m);

/**
 * Recursively descent the parameter tree to find the parameter matching the address pattern, and
 * attempt to decode the parameter value contained inside the OSC message.
 * @return True upon success. False when the parameter was not found or decoding the message failed.
 */
bool parameterFromOsc(ParameterMgr & paramMgr, const osc::ReceivedMessage & m, const char * addressPattern);

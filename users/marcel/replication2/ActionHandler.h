#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H
#pragma once

class ActionHandler
{
public:
	virtual void OnAction(int action, float value) = 0;
};

#endif

#ifndef RESUSER_H
#define RESUSER_H
#pragma once

class Res;

class ResUser
{
public:
	virtual ~ResUser();

	virtual void OnResInvalidate(Res* res) = 0;
	virtual void OnResDestroy(Res* res) = 0;
};

#endif

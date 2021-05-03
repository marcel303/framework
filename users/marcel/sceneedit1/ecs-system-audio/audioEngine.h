#pragma once

class Mat4x4;

struct AudioEngineBase
{
	virtual void init() = 0;
	virtual void shut() = 0;
	
	virtual void start() = 0;
	virtual void stop() = 0;
	
	virtual void setListenerTransform(const Mat4x4 & worldToView) = 0;
};

AudioEngineBase * createAudioEngine();

#pragma once

#include "CallBack.h"
#include "DeltaTimer.h"

namespace Game
{
	typedef struct AnimKey
	{
		float time;
		int event;
	} AnimKey;

	class AnimSeq
	{
	public:
		AnimSeq();
		void Initialize(ITimer* timer, CallBack onEvent);
		void Update();
		
		void Start(const AnimKey* keys, int keyCount);
		void Stop();
		
	private:
		bool mIsActive;
		DeltaTimer mTimer;
		const AnimKey* mKeys;
		int mKeyCount;
		int mLastKey;
		CallBack mOnEvent;
	};
}

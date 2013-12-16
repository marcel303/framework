#include "AnimSeq.h"

namespace Game
{
	AnimSeq::AnimSeq()
	{
		mIsActive = false;
		mKeys = 0;
		mKeyCount = 0;
		mLastKey = 0;
	}
	
	void AnimSeq::Initialize(ITimer* timer, CallBack onEvent)
	{
		mTimer.Initialize(timer);
		mOnEvent = onEvent;
	}
	
	void AnimSeq::Update()
	{
		if (!mIsActive)
			return;
		
		float t = mTimer.Delta_get();
		
		for (int i = mLastKey + 1; i < mKeyCount; ++i)
		{
			if (t >= mKeys[i].time)
			{
				if (mOnEvent.IsSet())
					mOnEvent.Invoke((void*)&mKeys[i]);
				
				mLastKey = i;
			}
			else
			{
				return;
			}
		}
	}
	
	void AnimSeq::Start(const AnimKey* keys, int keyCount)
	{
		mKeys = keys;
		mKeyCount = keyCount;
		mIsActive = true;
		mTimer.Start();
		mLastKey = -1;
	}
	
	void AnimSeq::Stop()
	{
		mIsActive = false;
		mTimer.Stop();
	}
}

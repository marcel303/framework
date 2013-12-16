#pragma once

#include "IView.h"

namespace Game
{
	class View_UpgradeHD : public IView
	{
	public:
		View_UpgradeHD();
		void Initialize();
		
		// --------------------
		// View related
		// --------------------
		
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		// --------------------
		// Touch related
		// --------------------
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
	private:
		int HitItem(const Vec2F& pos);
		float ItemPosW_get(int index);
		float ItemPosV_get(int index);
		float ItemSize_get(int index);
		float TotalItemSize_get(bool includeScroll);
		bool HasOpenItem_get();
		
		enum OpenState
		{
			OpenState_Closed,
			OpenState_ClosedAnim,
			OpenState_Open,
			OpenState_OpenAnim
		};
		
		// scrolling through the item list
		float mScrollPosition;
		float mScrollSpeed;
		bool mScrollActive;
		
		// opening item details
		int mOpenIndex;
		OpenState mOpenState;
		float mOpenAnim;
	};
}

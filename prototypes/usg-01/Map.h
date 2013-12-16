#pragma once

#include <allegro.h>
#include "Allocators.h"
#include "Input.h"
#include "IObject.h"
#include "ObjectList.h"
#include "ObjectPool.h"
#include "SelectionBuffer.h"
#include "SelectionMap.h"
#include "Types.h"

class BlackHole;
class BlackHoleGrid;
class Boss;
class Gun;
class Sun;

// Map == Game class..

enum MapClass
{
	MapClass_Gun,
	MapClass_Whatver,
	MapClass_EmptySpace
};

// ugh
enum MapTouchState
{
	MapTouchState_None,
	MapTouchState_FeedBlackHole,
	MapTouchState_MoveGun
};

class Map
{
public:
	Map(Vec2F size);
	void Initialize(Vec2F size);

	MapClass Classify(Vec2F pos); // note: pos in map coords.

	BOOL HandleInput(const InputEvent& e);

	void Update();
	void Render(BITMAP* buffer);
	void RenderSB(SelectionBuffer* sb);
	void Setup();

	Vec2F m_Size;

	Sun* m_Sun;
	Gun* m_Gun;
	Boss* m_Boss;

	MapTouchState m_TouchState;
	
	List<IObject*, PoolAllocator< ListNode<IObject*> > > m_UpdateList;

	BlackHoleGrid* m_BlackHoleGrid;

	SelectionBuffer* m_SelectionBuffer;
	SelectionMap* m_SelectionMap;
};

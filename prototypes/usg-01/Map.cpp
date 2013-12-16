#include "Map.h"
#include "Objects.h"

Map::Map(Vec2F size)
{
	Initialize(size);
}

void Map::Initialize(Vec2F size)
{
	m_Size = size;

	m_Sun = 0;
	m_Gun = 0;
	m_Boss = 0;

	m_TouchState = MapTouchState_None;

	m_BlackHoleGrid = new BlackHoleGrid();

	m_SelectionBuffer = new SelectionBuffer();
	m_SelectionBuffer->SetSize(size[0], size[1]);

	m_SelectionMap = new SelectionMap;
}

MapClass Map::Classify(Vec2F pos)
{
	RectF gunRect(
		m_Gun->HeliosPos() - Vec2F(3.0f, 3.0f),
		Vec2F(6.0f, 6.0f));

 	if (gunRect.IsInside(pos))
		return MapClass_Gun;

	return MapClass_EmptySpace;
}

BOOL Map::HandleInput(const InputEvent& e)
{
	switch (e.type)
	{
	case InputType_TouchDown:
		{
			MapClass test = Classify(Vec2F(e.x, e.y));

			if (test == MapClass_Gun)
			{
				m_TouchState = MapTouchState_MoveGun;
			}

			if (test == MapClass_EmptySpace)
			{
				// if touch down on empty space, spawn black hole.

				BlackHole* hole = new BlackHole(this, m_BlackHoleGrid, Vec2F(e.x, e.y));

				m_UpdateList.AddTail(hole);

				m_Gun->BlackHoleFeed_Begin(hole);

				m_TouchState = MapTouchState_FeedBlackHole;
			}
		}
	break;

	case InputType_TouchMove:
		{
			if (m_TouchState == MapTouchState_MoveGun)
			{
				m_Gun->Pull(Vec2F(e.x, e.y));
			}
		}
		break;

	case InputType_TouchUp:
		{
			if (m_TouchState == MapTouchState_FeedBlackHole)
			{
				//m_CurrentBlackHole = 0;
				m_Gun->BlackHoleFeed_End();
			}

			m_TouchState = MapTouchState_None;
		}
		break;
	}

	return TRUE;
}

void Map::Update()
{
	for (ListNode<IObject*>* node = m_UpdateList.m_Head; node;)
	{
		node->m_Object->Update(this);

		ListNode<IObject*>* next = node->m_Next;

		if (node->m_Object->m_ObjectFlags & ObjectFlag_Dead)
		{
			delete node->m_Object;

			m_UpdateList.Remove(node);
		}

		node = next;
	}
}

void Map::Render(BITMAP* buffer)
{
#if 0
	m_BlackHoleGrid->Render(buffer);
#endif

	for (ListNode<IObject*>* node = m_UpdateList.m_Head; node; node = node->m_Next)
	{
		node->m_Object->Render(buffer);
	}
}

void Map::RenderSB(SelectionBuffer* sb)
{
	for (ListNode<IObject*>* node = m_UpdateList.m_Head; node; node = node->m_Next)
	{
		node->m_Object->RenderSB(sb);
	}
}

void Map::Setup()
{
	m_Sun = new Sun(this);
	m_Sun->Setup(Vec2F(m_Size[0] / 2.0f, m_Size[1] / 2.0f));

	m_Gun = new Gun(this, m_Sun);
	
	m_Boss = new Boss(this);

	m_UpdateList.AddTail(m_Sun);
	m_UpdateList.AddTail(m_Gun);
	m_UpdateList.AddTail(m_Boss);
}

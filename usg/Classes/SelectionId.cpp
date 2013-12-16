#include "Debugging.h"
#include "Log.h"
#include "SelectionId.h"
#include "SelectionMap.h"

SelectionId::SelectionId()
{
	m_Id = 0;
}

SelectionId::~SelectionId()
{
	if (m_Id)
	{
#if defined(DEBUG) && 0
		LOG(LogLevel_Debug, "freeing selection ID: %d", (int)m_Id);
#endif
		
		m_Map->Free(m_Id);
	}
}

void SelectionId::Set(SelectionMap* map, void* p)
{
	Assert(m_Id == 0);
	
	m_Map = map;

	m_Id = map->Allocate();

	map->Set(m_Id, p);
	
#if defined(DEBUG) && 0
	LOG(LogLevel_Debug, "acquired selection ID: %d", (int)m_Id);
#endif
}

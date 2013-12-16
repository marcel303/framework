#include <assert.h>
#include <string.h>
#include "Log.h"

#include "GameState.h"
#include "World.h"

static LogCtx g_Log("dbg");

void dbg_show(const char* name)
{
	if (!strcmp(name, "wg.buckets"))
	{
		// todo: iterate over world grid cells
		//       show hash map bucket count, list stats, etc
		
		g_Log.WriteLine(LogLevel_Debug, "wg.buckets:");
		
		if (Game::g_World)
			Game::g_World->m_WorldGrid.DBG_Show();
		else
			g_Log.WriteLine(LogLevel_Debug, "world is NULL");
	}
	
	if (!strcmp(name, "w.entities"))
	{
		// todo: show statistics about world entities
		//       show free, allocated, etc
		
		g_Log.WriteLine(LogLevel_Debug, "w.entities:");
	}
	
	if (!strcmp(name, "sb.stats"))
	{
		// show selection buffer statistics
		
		if (g_GameState)
			g_GameState->m_SelectionBuffer.DBG_ShowStats();
		else
			g_Log.WriteLine(LogLevel_Debug, "game state is NULL");
	}
}


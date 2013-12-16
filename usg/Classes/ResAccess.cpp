#include "GameState.h"
#include "ResAccess.h"

const FontMap* GetFont(int id)
{
	return g_GameState->GetFont(id);
}

FontMap* _GetFont(int id)
{
	return g_GameState->_GetFont(id);
}

const VectorShape* GetShape(int id)
{
	return g_GameState->GetShape(id);
}

Res* GetSound(int id)
{
	return g_GameState->GetSound(id);
}

const AtlasImageMap* GetTexture(int id)
{
	return g_GameState->GetTexture(id);
}

#include "ResSnd.h"

ResSnd::ResSnd()
{
	SetType(RES_SND);

	m_data = 0;
}

void ResSnd::operator=(Mix_Chunk* data)
{
	m_data = data;
}

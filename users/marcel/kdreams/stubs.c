#include "ID_HEADS.H"
#include "id_mm.h"
#include "id_sd.h"

// MM_*

mminfotype	mminfo;
memptr		bufferseg;
boolean		bombonerror;

void MM_Startup (void)
{
	mminfo.mainmem = 1024 * 640;

	MM_GetPtr (&bufferseg,BUFFERSIZE);
}

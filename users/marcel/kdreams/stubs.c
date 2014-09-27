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

// SD_*

boolean		LeaveDriveOn;
boolean		SoundSourcePresent,SoundBlasterPresent,AdLibPresent,
					NeedsDigitized,NeedsMusic;	// For Caching Mgr
SDMode		SoundMode;
SMMode		MusicMode;
longword	TimeCount = 0;

boolean		ssIsTandy = false;
word		ssPort = 0;

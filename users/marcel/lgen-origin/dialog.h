#ifndef __dialog_h__
#define __dialog_h__

#if 0 // todo : dialog

//---------------------------------------------------------------------------

#include <allegro.h>

//---------------------------------------------------------------------------

#define DLG_OK		0
#define DLG_YESNO	1

//---------------------------------------------------------------------------

extern void dlg_init();
extern bool dlg_message(char* message, int type);
extern void dlg_config();

//---------------------------------------------------------------------------

#endif

#endif

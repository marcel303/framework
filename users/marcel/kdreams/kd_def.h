/* Keen Dreams Source Code
 * Copyright (C) 2014 Javier M. Chavez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// KD_DEF.H

#include "ID_HEADS.H"
#include "SOFT.H"
#include "SL_FILE.H"

#define FRILLS	0			// Cut out frills for 360K - MIKE MAYNARD


/*
=============================================================================

						GLOBAL CONSTANTS

=============================================================================
*/

#define CREDITS 0

#define	MAXACTORS	MAXSPRITES

#define ACCGRAVITY	3
#define SPDMAXY		80

#define BLOCKSIZE	(8*PIXGLOBAL)		// for positioning starting actors

//
// movement scrolling edges
//
#define SCROLLEAST (TILEGLOBAL*11)
#define SCROLLWEST (TILEGLOBAL*9)
#define SCROLLSOUTH (TILEGLOBAL*8)
#define SCROLLNORTH (TILEGLOBAL*4)

#define CLIPMOVE	24					// speed to push out of a solid wall

#define GAMELEVELS	17

/*
=============================================================================

							 TYPES

=============================================================================
*/

typedef	enum	{notdone,resetgame,levelcomplete,warptolevel,died,victorious}
				exittype;

typedef	enum	{nothing,keenobj,powerobj,doorobj,
	bonusobj,broccoobj,tomatobj,carrotobj,celeryobj,asparobj,grapeobj,
	taterobj,cartobj,frenchyobj,melonobj,turnipobj,cauliobj,brusselobj,
	mushroomobj,squashobj,apelobj,peapodobj,peabrainobj,boobusobj,
	shotobj,inertobj}	classtype;

typedef struct
{
  short int	leftshapenum,rightshapenum;
  enum		{step,slide,think,stepthink,slidethink} progress;
  boolean	skippable;

  boolean	pushtofloor;
  short int tictime;
  short int xmove;
  short int ymove;
  void (*think) ();
  void (*contact) ();
  void (*react) ();
  void *nextstate;
} statetype;


typedef	struct
{
	unsigned short	worldx,worldy;
	boolean	leveldone[GAMELEVELS];
	long	score,nextextra;
	short int		flowerpowers;
	short int		boobusbombs,bombsthislevel;
	short int		keys;
	short int		mapon;
	short int		lives;
	short int		difficulty;
} gametype;


typedef struct	objstruct
{
	classtype	obclass;
	enum		{no,yes,allways,removable} active;
	boolean		needtoreact,needtoclip;
	unsigned short	nothink;
	unsigned short	x,y;

	short int		xdir,ydir;
	short int		xmove,ymove;
	short int		xspeed,yspeed;

	short int		ticcount,ticadjust;
	statetype		*state;

	unsigned short	shapenum;

	unsigned short	left,top,right,bottom;	// hit rectangle
	unsigned short	midx;
	unsigned short	tileleft,tiletop,tileright,tilebottom;	// hit rect in tiles
	unsigned short	tilemidx;

	short int		hitnorth,hiteast,hitsouth,hitwest;	// wall numbers contacted

	intptr_t		temp1,temp2,temp3,temp4;

	void			*sprite;

	struct	objstruct	*next,*prev;
} objtype;

#pragma pack(push) // mstodo : needed?
#pragma pack(2)

struct BitMapHeader {
	unsigned short	w,h,x,y;
	unsigned char	d,trans,comp,pad;
};

struct BitMap {
	unsigned short Width;
	unsigned short Height;
	unsigned short Depth;
	unsigned short BytesPerRow;
	char far *Planes[8];
};

#pragma pack(pop)

struct Shape {
	memptr Data;
	long size;
	unsigned short BPR;
	struct BitMapHeader bmHdr;
};

typedef struct {
	int handle;			// handle of file
	memptr buffer;		// pointer to buffer
	word offset;		// offset into buffer
	word status;		// read/write status
} BufferedIO;


/*
=============================================================================

					 KD_MAIN.C DEFINITIONS

=============================================================================
*/

extern	char	str[80],str2[20];
extern	boolean	singlestep,jumpcheat,godmode,tedlevel;
extern	word	tedlevelnum;

void	DebugMemory (void);
void	TestSprites(void);
short	DebugKeys (void);
void	StartupId (void);
void	ShutdownId (void);
void	Quit (char *error);
void	InitGame (void);


/*
=============================================================================

					  KD_DEMO.C DEFINITIONS

=============================================================================
*/

void	Finale (void);
void	GameOver (void);
void	DemoLoop (void);
void	StatusWindow (void);
void	NewGame (void);
void	TEDDeath (void);

boolean	LoadGame (int file);
boolean	SaveGame (int file);
void	ResetGame (void);

/*
=============================================================================

					  KD_PLAY.C DEFINITIONS

=============================================================================
*/

extern	gametype		gamestate;
extern	exittype		playstate;
extern	boolean			button0held,button1held;
extern	unsigned short	originxtilemax,originytilemax;
extern	objtype			*new,*check,*player,*scoreobj;

extern	ControlInfo	c;

extern	objtype dummyobj;

extern	char		*levelnames[21];

void	CheckKeys (void);
void	CalcInactivate (void);
void 	InitObjArray (void);
void 	GetNewObj (boolean usedummy);
void	RemoveObj (objtype *gone);
void 	ScanInfoPlane (void);
void 	PatchWorldMap (void);
void 	MarkTileGraphics (void);
void 	FadeAndUnhook (void);
void 	SetupGameLevel (boolean loadnow);
void 	ScrollScreen (void);
void 	MoveObjVert (objtype *ob, short ymove);
void 	MoveObjHoriz (objtype *ob, short xmove);
void 	GivePoints (unsigned short points);
void 	ClipToEnds (objtype *ob);
void 	ClipToEastWalls (objtype *ob);
void 	ClipToWestWalls (objtype *ob);
void 	ClipToWalls (objtype *ob);
void	ClipToSprite (objtype *push, objtype *solid, boolean squish);
void	ClipToSpriteSide (objtype *push, objtype *solid);
short 	DoActor (objtype *ob,short tics);
void 	StateMachine (objtype *ob);
void 	NewState (objtype *ob,statetype *state);
void 	PlayLoop (void);
void 	GameLoop (void);

/*
=============================================================================

					  KD_KEEN.C DEFINITIONS

=============================================================================
*/

void	CalcSingleGravity (void);

void	ProjectileThink		(objtype *ob);
void	VelocityThink		(objtype *ob);
void	DrawReact			(objtype *ob);

void	SpawnScore (void);
void	FixScoreBox (void);
void	SpawnWorldKeen (short tilex, short tiley);
void	SpawnKeen (short tilex, short tiley, short dir);

void 	KillKeen (void);

extern	short			singlegravity;
extern	unsigned short	bounceangle[8][8];

extern	statetype s_keendie1;

/*
=============================================================================

					  KD_ACT1.C DEFINITIONS

=============================================================================
*/

void WalkReact (objtype *ob);

void 	DoGravity (objtype *ob);
void	AccelerateX (objtype *ob,short dir,short max);
void 	FrictionX (objtype *ob);

void	ProjectileThink		(objtype *ob);
void	VelocityThink		(objtype *ob);
void	DrawReact			(objtype *ob);
void	DrawReact2			(objtype *ob);
void	DrawReact3			(objtype *ob);
void	ChangeState (objtype *ob, statetype *state);

void	ChangeToFlower (objtype *ob);

void	SpawnBonus (short tilex, short tiley, short type);
void	SpawnDoor (short tilex, short tiley);
void 	SpawnBrocco (short tilex, short tiley);
void 	SpawnTomat (short tilex, short tiley);
void 	SpawnCarrot (short tilex, short tiley);
void 	SpawnAspar (short tilex, short tiley);
void 	SpawnGrape (short tilex, short tiley);

extern	statetype s_doorraise;

extern	statetype s_bonus;
extern	statetype s_bonusrise;

extern	statetype s_broccosmash3;
extern	statetype s_broccosmash4;

extern	statetype s_grapefall;

/*
=============================================================================

					  KD_ACT2.C DEFINITIONS

=============================================================================
*/

void SpawnTater (short tilex, short tiley);
void SpawnCart (short tilex, short tiley);
void SpawnFrenchy (short tilex, short tiley);
void SpawnMelon (short tilex, short tiley,short dir);
void SpawnSquasher (short tilex, short tiley);
void SpawnApel (short tilex, short tiley);
void SpawnPeaPod (short tilex, short tiley);
void SpawnPeaBrain (short tilex, short tiley);
void SpawnBoobus (short tilex, short tiley);

extern	statetype s_taterattack2;
extern	statetype s_squasherjump2;
extern	statetype s_boobusdie;

extern	statetype s_deathwait1;
extern	statetype s_deathwait2;
extern	statetype s_deathwait3;
extern	statetype s_deathboom1;
extern	statetype s_deathboom2;


/////////////////////////////////////////////////////////////////////////////
//
//						GELIB.C DEFINITIONS
//
/////////////////////////////////////////////////////////////////////////////

void FreeShape(struct Shape *shape);
short UnpackEGAShapeToScreen(struct Shape *SHP, short startx, short starty);

long Verify(char *filename);
memptr InitBufferedIO(int handle, BufferedIO *bio);
void FreeBufferedIO(BufferedIO *bio);
byte bio_readch(BufferedIO *bio);
void bio_fillbuffer(BufferedIO *bio);
void SwapLong(long far *Var);
void SwapWord(unsigned short far *Var);
void MoveGfxDst(short x, short y);
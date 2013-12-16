#include <UIKit/UIKit.h>
#include "Atlas.h"
#include "FixedSizeString.h"
#include "GameCenterManager.h"
#include "GameState.h"
#include "LogicTimer.h"
#include "OpenALState.h"
#include "OpenGLState.h"
#include "ParticleEffect.h"
#include "ResIndex.h"
#include "ResMgr.h"
#include "SoundEffectMgr.h"
#include "TouchMgr.h"
#include "VectorShape.h"

@interface GameView : UIView <GameCenterManagerDelegate, UIAlertViewDelegate>
{
	@private
	
	TouchMgr* m_TouchMgr;
	OpenGLState* m_OpenGLState;
	OpenALState* m_OpenALState;
	
	Application* m_GameState;
	LogicTimer* m_LogicTimer;
	
	int m_FPSFrame;
	int m_FPS;
	
	NSTimer* m_Timer_Logic;
	NSTimer* m_Timer_FPS;
	
	UIAlertView* m_rateAlert;
	UIAlertView* m_infoAlert;
	
	bool m_appHack_Speed150;
	bool m_appHack_Speed200;
	
	bool m_HasException;
	FixedSizeString<1024> m_ExceptionText;
	
	bool m_IsInitialized;
	
	LogCtx m_Log;
}

- (id)initWithFrame:(CGRect)frame;
- (void)initForReal;

- (Vec2F)PointToPointF:(CGPoint)point;
- (Vec2F)ScreenToView:(Vec2F)location withOffset:(bool)offset;
- (Vec2F)ScreenToViewWithTouch:(UITouch*)touch withOffset:(bool)offset;
- (Vec2F)ScreenToWorld:(Vec2F)location withOffset:(bool)offset;
- (Vec2F)ScreenToWorldWithTouch:(UITouch*)touch withOffset:(bool)offset;

- (void)startTimers;
- (void)stopTimers;

- (void)pauseApp;
- (void)resumeApp;

- (void)drawView;

- (void)update;
- (void)render;

#if defined(IPAD)
-(NSUInteger)supportedInterfaceOrientations;
-(BOOL)shouldAutorotate;
#endif

#ifndef DEPLOYMENT

- (void)drawSelectionBuffer;
- (void)drawWorldGrid;
- (void)drawAxis;
- (void)drawVectorGraphic3D;
- (void)benchSinCos;
- (void)benchMemory;

#endif

- (void)handleNotifications;

- (void)handleMemoryWarning;
- (void)showException:(std::exception&)e;

- (void)spawnParticlesAt:(Vec2F)position withThisMany:(int)count;

// -------------------------------
// UIAlertViewDelegate
// -------------------------------
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

@end

extern GameView* g_GameView;

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import "GameView.h"

#import "AnimTimer.h"
#import "AudioSession.h"
#import "Calc.h"
#import "CompiledShape.h"
#import "ConfigState.h"
#import "GameCenter.h"
#import "OpenGLUtil.h"
#import "PerfCount.h"
#import "ResIO.h"
#import "ResMgr.h"
//#import "Resources.h"
#import "SelectionBuffer_Test.h"
#import "SpriteGfx.h"
//#import "Stream_File.h"
#import "System.h"
#import "TextureAtlas.h"
#import "TexturePVR.h"
#import "TextureRGBA.h"
#import "Textures.h"
#import "TimeTracker.h"

#import "Types.h"

#import "Benchmark.h"
#import "Camera.h"
#import "WorldGrid.h"
#import "ColHashMap.h"
#import "Path.h"
#import "TempRender.h"

#include "Game/GameSettings.h"

#include "Base64.h"
#include "grs.h"

GameView* g_GameView;

#include "GameState.h"
#include "Menu_Scores.h"
#include "MenuMgr.h"
#include "View_Scores.h"
static void HandleGameCenterLoginComplete(void * obj, void * arg)
{
	Game::View_Scores* view = (Game::View_Scores*)g_GameState->GetView(View_Scores);
	view->Database_set(Game::ScoreDatabase_Global);
	view->Refresh();
	
	GameMenu::Menu_Scores* menu = (GameMenu::Menu_Scores*)g_GameState->m_MenuMgr->Menu_get(GameMenu::MenuType_Scores);
	menu->Handle_GameCenterLogin();
}

#ifdef DEBUG
#include "Graphics.h"
#include "StringBuilder.h"
#include "UsgResources.h"
static Vec2F s_LastTouchPos(0.0f, 0.0f);
static float s_LastTouchTimer = 0.0f;
#endif

@implementation GameView

- (id)initWithFrame:(CGRect)frame
{
	if ((self = [super initWithFrame:frame]))
	{
		LOG_INF("initialized game view", 0);
		
		m_IsInitialized = false;
	}
	else
	{
		LOG_ERR("failed to initialize game view", 0);
	}
	
	return self;
}

- (void)initForReal
{
	try
	{
		if (m_IsInitialized)
			return;
		
		m_IsInitialized = true;
			
		UsingBegin (Benchmark gbm("GameView init"))
		{
		//if ((self = [super initWithFrame:frame]))
		{
#if !defined(DEPLOYMENT) && 0
			if (NSClassFromString(@"iSimulate"))
				[NSClassFromString(@"iSimulate") disableTouchesOverlay];
#endif
			
#if 1
#if !defined(IPAD)
			if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 4.0f)
				self.contentScaleFactor = [UIScreen mainScreen].scale;
#endif
#else
#warning "iPhone 4 retina support disabled!"
#endif
			
			g_GameView = self;
			
			m_Log = LogCtx("GameView");
			m_HasException = false;
			
			[self setMultipleTouchEnabled:TRUE];

			CAEAGLLayer* eaglLayer = (CAEAGLLayer*)self.layer;
			
			UsingBegin (Benchmark bm("AudioSession"))
			{
				// Initialize audio session (NOTE: Do this first to allow user to immediately change audio volume while still initializing)
				
				bool backgroundMusic = AudioSession_DetectbackgroundMusic();
			
				AudioSession_Initialize(backgroundMusic);
			}
			UsingEnd()
			
			// Create touch manager
			
			m_TouchMgr = new TouchMgr();
			
			UsingBegin (Benchmark bm("OpenGL"))
			{
				// Initialize OpenGL
			
				m_OpenGLState = new OpenGLState();
			
#ifdef IPAD
				bool trueColor = true;
#else
				bool trueColor = false;
#endif
				
				if (!m_OpenGLState->Initialize(eaglLayer, VIEW_SX, VIEW_SY, false, trueColor))
				{
					NSLog(@"Failed to initialize OpenGL\n");
				}
				
				m_OpenGLState->CreateBuffers();
			}
			UsingEnd()
			
			// todo: try to redraw screen. does it work? can we use it to implement a simple loading screen?
			
			UsingBegin (Benchmark bm("OpenAL"))
			{
				// Initialize OpenAL
			
				m_OpenALState = new OpenALState();
			
				if (!m_OpenALState->Initialize())
				{
					NSLog(@"Failed to initialize OpenAL\n");
				}
				
				AudioSession_ManageOpenAL(m_OpenALState);
			}
			UsingEnd()
					
			g_gameCenter = new GameCenter();
			
			UsingBegin (Benchmark bm("GameState"))
			{
				// Create game state
			
				float scale = 1.0f;
				
#if !defined(IPAD)
			if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 4.0f)
				scale = [UIScreen mainScreen].scale;
#else
				scale = 2.0f;
#endif

				
				m_GameState = new Application();
				m_GameState->Setup(m_OpenALState, scale);
			}
			UsingEnd()
			
			// Route touch input to game state
			
			m_Log.WriteLine(LogLevel_Debug, "Setting up touch callbacks");
			
			m_TouchMgr->OnTouchBegin = CallBack(m_GameState, Application::HandleTouchBegin);
			m_TouchMgr->OnTouchEnd = CallBack(m_GameState, Application::HandleTouchEnd);
			m_TouchMgr->OnTouchMove = CallBack(m_GameState, Application::HandleTouchMove);
			
			// Initialize logic timer
			
			m_Log.WriteLine(LogLevel_Debug, "Initializing logic timer");
			
			m_LogicTimer = new LogicTimer();
			m_LogicTimer->Setup(new Timer(), true, 60.0f, 3);
			
			// Initialize FPS related stuff
			
			m_Log.WriteLine(LogLevel_Debug, "Initializing FPS counter");
			
			m_FPSFrame = 0;
			m_FPS = 0;
			
			// Display splash screen
			
			#if defined(DEPLOYMENT)
			// loading is so fast on modern day devices.. showing the splash screen seems rushed..
			// aim for showing the splash screen for 3 seconds by adding a sleep
			sleep(2);
			#endif
			
			// Begin main loop
			
			m_Log.WriteLine(LogLevel_Debug, "Starting main loop");
			
			m_LogicTimer->Start();
			
			[self startTimers];
			
			[self render];
			
			g_gameCenter->OnLoginComplete = CallBack(0, HandleGameCenterLoginComplete);
		}
		}
		UsingEnd();
	}
	catch (Exception& e)
	{
		[self showException:e];
		
		throw e;
	}
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (void)dealloc
{
	try
	{
		m_Log.WriteLine(LogLevel_Info, "dealloc");
		
		// Stop timers
		
		[self stopTimers];
		
		// Dealloc alert views
		[m_infoAlert dealloc];
		m_infoAlert = nil;
		
		[m_rateAlert dealloc];
		m_rateAlert = nil;
		
		// Free objects (in reverse order of creation)
		
		delete m_LogicTimer;
		m_LogicTimer = 0;
		
		delete m_GameState;
		m_GameState = 0;
	
		delete g_gameCenter;
		g_gameCenter = 0;
		
		m_OpenALState->Shutdown();
		delete m_OpenALState;
		m_OpenALState = 0;
		
		m_OpenGLState->Shutdown();
		delete m_OpenGLState;
		m_OpenGLState = 0;
		
		AudioSession_Shutdown();
		
		delete m_TouchMgr;
		m_TouchMgr = 0;
		
		[super dealloc];
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)handleNotifications
{
	// set orientation
	// note: this is a HACK! but required to make the alert views work ;)
	UIInterfaceOrientation orientationNew = g_GameState->m_GameSettings->m_ScreenFlip ? UIInterfaceOrientationLandscapeLeft : UIInterfaceOrientationLandscapeRight;
	[[UIApplication sharedApplication] setStatusBarOrientation:orientationNew animated:NO];
	
	int resetNotifications = ConfigGetIntEx("utilResetNotifications", 0);
	
	if (resetNotifications)
	{
		ConfigSetInt("rating_runcount", 0);
		ConfigSetInt("rating_disabled", 0);
		ConfigSetInt("info_shown", 0);
		ConfigSetInt("utilResetNotifications", 0);
	}
	
	// app rating
	
#ifndef IPAD
	m_rateAlert = [[UIAlertView alloc] initWithTitle:@"Like this game? Please leave a rating on iTunes! :)" message:nil delegate:self cancelButtonTitle:@"Remind me later" otherButtonTitles:@"Yes, rate it!", @"Don't ask me again", nil];
#else
	m_rateAlert = [[UIAlertView alloc] initWithTitle:@"Like This Game?" message:@"If you like this game, please give it a 5 star rating on iTunes! :)" delegate:self cancelButtonTitle:@"Remind me later" otherButtonTitles:@"Yes, rate it!", @"Don't ask me again", nil];
#endif
	
	int ratingRuncount = ConfigGetIntEx("rating_runcount", 0) + 1;
	
#if defined(DEPLOYMENT)
	int ratingDisabled = ConfigGetIntEx("rating_disabled", 0);
	int ratingBegin = 4;
	int ratingRepeat = 4;
#else
	int ratingDisabled = 0;
	int ratingBegin = 1;
	int ratingRepeat = 1;
#endif
	
#ifndef DEBUG
	if (ratingDisabled == 0 && ratingRuncount >= ratingBegin && ((ratingRuncount - ratingBegin) % ratingRepeat) == 0)
	{
		[m_rateAlert show];
	}
#endif
	
	ConfigSetInt("rating_runcount", ratingRuncount);
	
	// first start info message
	
	NSString* msg =
		@"We hope you enjoy this game! To those who prefer tilt controls: " \
		@"tilt can be enabled from the advanced options menu. Also, the optional 3D effect can be enable from there.";
	
	m_infoAlert = [[UIAlertView alloc] initWithTitle:@"Welcome!" message:msg delegate:self cancelButtonTitle:@"Dismiss" otherButtonTitles:nil];
	
	int infoShown = ConfigGetIntEx("info_shown", 0);
	
	if (infoShown == 0)
	{
		[m_infoAlert show];
		
		ConfigSetInt("info_shown", 1);
	}
	
	ConfigSave(false);	
}

- (void)handleMemoryWarning
{
	try
	{
		if (m_IsInitialized == false)
			return;
		
		m_Log.WriteLine(LogLevel_Debug, "Received memory warning");
		
		m_GameState->m_ResMgr.HandleMemoryWarning();
		
		//m_GameState->m_TextureAtlas->m_Texture->HandleMemoryWarning();
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)showException:(std::exception&)e;
{
	if (!m_HasException)
	{
		m_HasException = true;
		
		m_Log.WriteLine(LogLevel_Error, "Exception: %s", e.what());

		[[[[UIAlertView alloc] initWithTitle:@"Fatal Error" message:[NSString stringWithFormat:@"The application experienced a fatal exception.\n\n%s",  e.what()] delegate:self cancelButtonTitle:@"EXIT" otherButtonTitles:nil] autorelease] show];
		
		m_ExceptionText = e.what();
	}
}

- (Vec2F)PointToPointF:(CGPoint)point
{
	return Vec2F(point.x, point.y);
}

- (Vec2F)ScreenToView:(Vec2F)location withOffset:(bool)offset
{
	float scaleX = VIEW_SY / self.frame.size.width;
	float scaleY = VIEW_SX / self.frame.size.height;
	
	location[0] *= scaleX;
	location[1] *= scaleY;
	
	Vec2F result;
	
#if defined(IPAD)
	result = Vec2F(location.x, location.y + (offset ? TOUCH_OFFSET : 0.0f));
#else
	if (!g_GameState->m_GameSettings->m_ScreenFlip)
		result = Vec2F(location.y, VIEW_SY - location.x - (offset ? TOUCH_OFFSET : 0.0f));
	else
		result = Vec2F(VIEW_SX - location.y, location.x + (offset ? TOUCH_OFFSET : 0.0f));
#endif
	
	return result;
}

- (Vec2F)ScreenToViewWithTouch:(UITouch*)touch withOffset:(bool)offset
{
	CGPoint location = [touch locationInView:self];
	
	return [self ScreenToView:[self PointToPointF:location] withOffset:offset];
}

- (Vec2F)ScreenToWorld:(Vec2F)location withOffset:(bool)offset
{
	location = [self ScreenToView:location withOffset:offset];
	
	return m_GameState->m_Camera->ViewToWorld(location);
}

- (Vec2F)ScreenToWorldWithTouch:(UITouch*)touch withOffset:(bool)offset
{
	CGPoint location = [touch locationInView:self];
	
	return [self ScreenToWorld:[self PointToPointF:location] withOffset:offset];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		if (m_IsInitialized == false)
			return;
		
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			Vec2F locationV = [self ScreenToViewWithTouch:touch withOffset:false];
			Vec2F locationW = [self ScreenToWorldWithTouch:touch withOffset:false];
			Vec2F locationWOffset = [self ScreenToWorldWithTouch:touch withOffset:true];
			
			m_TouchMgr->TouchBegin(touch, locationV, locationW, locationWOffset);
			
#ifdef DEBUG
			s_LastTouchPos = locationV;
			s_LastTouchTimer = 1.0f;
#endif
		}
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		if (m_IsInitialized == false)
			return;
		
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
	
			Vec2F locationV = [self ScreenToViewWithTouch:touch withOffset:false];
			Vec2F locationW = [self ScreenToWorldWithTouch:touch withOffset:false];
			Vec2F locationWOffset = [self ScreenToWorldWithTouch:touch withOffset:true];
			
			m_TouchMgr->TouchMoved(touch, locationV, locationW, locationWOffset);
			
#ifdef DEBUG
			s_LastTouchPos = locationV;
			s_LastTouchTimer = 1.0f;
#endif
		}
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	try
	{
		if (m_IsInitialized == false)
			return;
		
		for (NSUInteger i = 0; i < [[touches allObjects] count]; ++i)
		{
			UITouch* touch = [[touches allObjects] objectAtIndex:i];
			
			m_TouchMgr->TouchEnd(touch);
		}
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	if (m_IsInitialized == false)
		return;
	
	[self touchesEnded:touches withEvent:event];
}

- (void)fpsUpdate
{
	m_FPS = m_FPSFrame;
	m_FPSFrame = 0;
	
#if 0
	m_Log.WriteLine(LogLevel_Debug, "FPS: %d", m_FPS);
	
	static int fpsUpdateCount = 0;
	
	if (fpsUpdateCount % 10 == 0)
	{
		g_PerfCount.DBG_Show();
		
		m_Log.WriteLine(LogLevel_Info, "[alloc] size: %d", (int)GetAllocState().allocationSize);
	}
	
	fpsUpdateCount++;
#endif
}

- (void)startTimers
{
	[self stopTimers];
	
//	m_Timer_Logic = [NSTimer scheduledTimerWithTimeInterval:m_LogicTimer->TimeStep_Get() target:self selector:@selector(drawView) userInfo:nil repeats:TRUE];
	m_Timer_Logic = [NSTimer scheduledTimerWithTimeInterval:1.0f / 200.0f target:self selector:@selector(drawView) userInfo:nil repeats:TRUE];
//	m_Timer_Logic = [NSTimer scheduledTimerWithTimeInterval:1.0f / 100.0f target:self selector:@selector(drawView) userInfo:nil repeats:TRUE];
#ifndef DEPLOYMENT
	m_Timer_FPS = [NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(fpsUpdate) userInfo:nil repeats:TRUE];
#endif
}

- (void)stopTimers
{
	[m_Timer_Logic invalidate];
	m_Timer_Logic = nil;
	
	[m_Timer_FPS invalidate];
	m_Timer_FPS = nil;
}

//static float gPauseTime;

- (void)pauseApp
{
	try
	{
		m_Log.WriteLine(LogLevel_Debug, "Pause");
		
	//	gPauseTime = g_TimerRT.Time_get();
		
		AudioSession_Pause();
		
		m_LogicTimer->Pause();
		
		[self stopTimers];
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)resumeApp
{
	try
	{
		m_Log.WriteLine(LogLevel_Debug, "Resume");
		
		ConfigLoad();
		
		m_LogicTimer->Resume();
		
		[self startTimers];
		
		AudioSession_Resume();
		
		//	g_TimerRT.Time_set(gPauseTime);
		
		g_GameState->m_TimeTracker_Global->Time_set(g_TimerRT.Time_get());
		
#if defined(IPHONEOS) && 1
		if (!g_gameCenter->IsLoggedIn())
			g_gameCenter->Login();
#endif
		
		m_appHack_Speed150 = ConfigGetIntEx("addonSpeed150", 0);
		m_appHack_Speed200 = ConfigGetIntEx("addonSpeed200", 0);
		
		[self handleNotifications];
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)drawView
{
	try
	{
		UsingBegin(PerfTimer timer(PC_UPDATE))
		{
			[self update];
		}
		UsingEnd()
		
		UsingBegin(PerfTimer timer(PC_RENDER))
		{
			[self render];
		}
		UsingEnd()
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

- (void)update
{
	// update loop w/ time recovery
	
	if (m_LogicTimer->BeginUpdate())
	{
		while (m_LogicTimer->Tick())
		{
			if (m_appHack_Speed200)
			{
				m_GameState->Update(1.0f);
				m_GameState->Update(1.0f);
			}
			else if (m_appHack_Speed150)
			{
				m_GameState->Update(0.75f);
				m_GameState->Update(0.75f);
			}
			else
			{
				m_GameState->Update(1.0f);
			}
		}
		
		m_LogicTimer->EndUpdate();
	}
}

- (void)render
{
	g_PerfCount.Set_Count(PC_RENDER_FLUSH, 0);
	
	UsingBegin(PerfTimer timer(PC_RENDER_MAKECURRENT))
	{	
		// prepare OpenGL context
		
		m_OpenGLState->MakeCurrent();
	}
	UsingEnd()
	
	// clear surface
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//	glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
//	glClear(GL_COLOR_BUFFER_BIT);
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	
	UsingBegin(PerfTimer timer(PC_RENDER_SETUP))
	{
		// prepare OpenGL transformation for landscape mode drawing with X axis right and Y axis down
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

#if defined(IPAD)
		glOrthof(0.f, VIEW_SX, VIEW_SY, 0.f, -1000.0f, 1000.0f);
#else
		if (!g_GameState->m_GameSettings->m_ScreenFlip)
		{
			glOrthof(0.0f, VIEW_SY, VIEW_SX, 0.0f, -1000.0f, 1000.0f);
		}
		else
		{
//			glOrthof(0.0f, VIEW_SY, VIEW_SX, 0.0f, -1000.0f, 1000.0f);
			glOrthof(VIEW_SY, 0.0f, 0.0f, VIEW_SX, -1000.0f, 1000.0f);
		}
		
//		glMatrixMode(GL_MODELVIEW);
//		glLoadIdentity();
		
		// rotate view and adjust Y direction
		
		glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
		glTranslatef(0.0f, -VIEW_SY, 0.0f);
#endif
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	UsingEnd()
	
	m_GameState->Render();
	
#ifdef DEBUG
	s_LastTouchTimer -= 1.0f / 60.0f;
	if (s_LastTouchTimer > 0.0f)
	{
		float opacity = Calc::Min(1.0f, s_LastTouchTimer * 2.0f);
		gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
		TempRenderBegin(g_GameState->m_SpriteGfx);
		StringBuilder<32> sb;
		sb.AppendFormat("[%d, %d]", (int)s_LastTouchPos[0], (int)s_LastTouchPos[1]);
		RenderText(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, opacity), TextAlignment_Left, TextAlignment_Top, true, sb.ToString());
		g_GameState->m_SpriteGfx->Flush();
		g_GameState->m_SpriteGfx->FrameEnd();
	}
#endif
	
//#ifdef DEBUG
#if 0
	// prepare to render debug pass
	
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	
	[self drawSelectionBuffer];

	glDisable(GL_BLEND);
	
//	[self drawWorldGrid];
	
//	[self drawAxis];
#endif
//#endif
	
#if 1
//	[self benchSinCos];
//	[self benchMemory];
#endif
	
#if 0
	[self drawVectorGraphic3D];
#endif
	
	UsingBegin(PerfTimer timer(PC_RENDER_PRESENT))
	{
		// present rendered output to screen
		
		m_OpenGLState->Present();
	}
	UsingEnd()
	
	m_FPSFrame++;
}

#if defined(IPAD)
- (NSUInteger)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
}

- (BOOL)shouldAutorotate
{
	return [[UIDevice currentDevice] orientation] != UIInterfaceOrientationPortrait;
}
#endif

#ifndef DEPLOYMENT

- (void)drawSelectionBuffer
{
	GLuint sbTexture = 0;
	
	glGenTextures(1, &sbTexture);
	glBindTexture(GL_TEXTURE_2D, sbTexture);
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();
	
	RectF rect = g_GameState->m_Camera->m_Area;
	const float focusX = rect.m_Position[0] + rect.m_Size[0] * 0.5f;
	const float focusY = rect.m_Position[1] + rect.m_Size[1] * 0.5f;

	const float zoom = 1.0f;

	glTranslatef(VIEW_SX / 2.0f, VIEW_SY / 2.0f, 0.0f);
	glScalef(zoom, zoom, 1.0f);
	glTranslatef(-focusX, -focusY, 0.0f);
//	glTranslatef(-(rect.m_Position[0] - rect.m_Size[0] * 0.5f), -(rect.m_Position[1] - rect.m_Size[1] * 0.5f), 0.0f);
	
	int textureSx = 512 * 2;
	int textureSy = 512 * 2;
	
	SelectionBuffer_ToTexture(&m_GameState->m_SelectionBuffer, 0, 0, textureSx, textureSy);
	
	m_GameState->m_SpriteGfx->Reserve(4, 6);
	m_GameState->m_SpriteGfx->WriteBegin();
	
	m_GameState->m_SpriteGfx->WriteVertex(0.0f, 0.0f, SpriteColors::White.rgba, 0.0f, 0.0f);
	m_GameState->m_SpriteGfx->WriteVertex(textureSx, 0.0f, SpriteColors::White.rgba, 1.0f, 0.0f);
	m_GameState->m_SpriteGfx->WriteVertex(textureSx, textureSy, SpriteColors::White.rgba, 1.0f, 1.0f);
	m_GameState->m_SpriteGfx->WriteVertex(0.0f, textureSy, SpriteColors::White.rgba, 0.0f, 1.0f);
	m_GameState->m_SpriteGfx->WriteIndex(0);
	m_GameState->m_SpriteGfx->WriteIndex(1);
	m_GameState->m_SpriteGfx->WriteIndex(2);
	m_GameState->m_SpriteGfx->WriteIndex(0);
	m_GameState->m_SpriteGfx->WriteIndex(2);
	m_GameState->m_SpriteGfx->WriteIndex(3);
	
	m_GameState->m_SpriteGfx->WriteEnd();
	m_GameState->m_SpriteGfx->Flush();
	
	glPopMatrix();
	glDeleteTextures(1, &sbTexture);
	glDisable(GL_TEXTURE_2D);
}

- (void)drawWorldGrid
{
	m_GameState->DBG_RenderWorldGrid();
}

- (void)drawAxis
{
//	Vec2F mid(-5, -5);
	Vec2F mid(20, 20);
	
	RenderRect(mid, mid + Vec2F(100.0f, 10.0f), 0.0f, 0.0f, 1.0f, m_GameState->GetTexture(Textures::COLOR_WHITE));
	RenderRect(mid, mid + Vec2F(10.0f, 100.0f), 1.0f, 0.0f, 0.0f, m_GameState->GetTexture(Textures::COLOR_WHITE));
	
	m_GameState->m_SpriteGfx->Flush();
}

- (void)drawVectorGraphic3D
{
#if 0
	OpenGLUtil::SetTexture(m_GameState->m_TextureAtlas->m_Texture);
	glEnable(GL_TEXTURE_2D);
	
	glPushMatrix();
	glTranslatef(240.0f, 160.0f, 0.0f);
	glRotatef(m_GameState->m_TimeTracker_Global.Time_get() * 100.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(sinf(m_GameState->m_TimeTracker_Global.Time_get()) * 20.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(sinf(m_GameState->m_TimeTracker_Global.Time_get()) * 20.0f, 0.0f, 1.0f, 0.0f);
	
	const VectorShape* shape = m_GameState->GetShape(Resources::BOSS_01_MODULE_HEAD);
	
	// todo: use visual triangles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionTriangleCount; ++i)
	{
		VRCS::Collision_Triangle& triangle = shape->m_Shape.m_CollisionTriangles[i];
		
		for (int j = 0; j < 3; ++j)
		{
			int index1 = (j + 0) % 3;
			int index2 = (j + 1) % 3;
			
			float x1 = triangle.m_Coords[index1 * 2 + 0];
			float y1 = triangle.m_Coords[index1 * 2 + 1];
			float x2 = triangle.m_Coords[index2 * 2 + 0];
			float y2 = triangle.m_Coords[index2 * 2 + 1];
			
			RenderBeam(
					   Vec2F(x1, y1) * 5.0f,
					   Vec2F(x2, y2) * 5.0f, 
					   4.0f, 
					   SpriteColors::White,
					   m_GameState->GetTexture(Textures::PARTICLE_SPARK),
					   m_GameState->GetTexture(Textures::PARTICLE_SPARK),
					   m_GameState->GetTexture(Textures::PARTICLE_SPARK));
		}
	}
	
	g_GameState->m_SpriteGfx.Flush();
	
	glPopMatrix();
#endif
}

- (void)benchSinCos
{
	int n = 1000000;
	
	UsingBegin(Benchmark bm("sincos_native"))
	{
		float v = 0.0f;
		float a = 0.0f;
		float s = 0.1f;
		
		for (int i = 0; i < n; ++i)
		{
			v += sinf(a);
			v += cosf(a);
			
			a += s;
		}
		
		LOG(LogLevel_Info, "sincos: result: %f", v);
	}
	UsingEnd()
	
	UsingBegin(Benchmark bm("sincos_lut"))
	{
		float v = 0.0f;
		float a = 0.0f;
		float s = 0.1f;
		
		for (int i = 0; i < n; ++i)
		{
			float vs;
			float vc;
			
			Calc::SinCos_Fast(a, &vc, &vs);
			
			v += vs;
			v += vc;
			
			a += s;
		}
		
		LOG(LogLevel_Info, "sincos: result: %f", v);
	}
	UsingEnd()
}

- (void)benchMemory
{
	int megCount = 10;
	int byteCount = 1024 * 1024 * megCount;
	
	char* mem = new char[byteCount];
	
	DeltaTimer delta;
	delta.Initialize(&g_TimerRT);
	delta.Start();
	
	mem[0] = 0;
	for (int i = 1; i < byteCount; ++i)
		mem[i] = mem[i - 1] + 1;
	
	LOG(LogLevel_Info, "memory: write: result: %.2fMb/sec", megCount / delta.Delta_get());
	
	delete[] mem;
}

#endif

- (void)spawnParticlesAt:(Vec2F)position withThisMany:(int)count
{
	const AtlasImageMap* image = m_GameState->GetTexture(Textures::PARTICLE_SPARK);
	
	for (int i = 0; i < count; ++i)
	{
		Particle& p = m_GameState->m_ParticleEffect.Allocate(image->m_Info, 0, 0);
		
		Particle_Default_Setup(
			&p,
			position[0] + Calc::Random_Scaled(10.0f),
			position[1] + Calc::Random_Scaled(10.0f), 1.0f, 10.0f, 4.0f, Calc::Random(Calc::mPI * 2.0f), 200.0f);
	}
}

// -------------------------------
// UIAlertViewDelegate
// -------------------------------

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	if (alertView == m_infoAlert)
	{
	}
#if 0
	if (alertView == m_shareAlert)
	{
		if (buttonIndex == 1)
		{
			[shareAlert dismissWithClickedButtonIndex:-1 animated:NO];
			
			[self handleShare:nil];
			
			SetInt(@"share_disabled", 1);
		}
	}
	if (alertView == shareInfoAlert)
	{
		[shareInfoAlert dismissWithClickedButtonIndex:-1 animated:NO];
		
		[self handleShare2:nil];
	}
#endif
	if (alertView == m_rateAlert)
	{
		if (buttonIndex == 0)
		{
			// remind me
		}
		if (buttonIndex == 1)
		{
			// rate it!
			
			//NSString* url = @"http://itunes.com/apps/granniesgames/puzzletime";
			//NSString* url = @"http://itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?id=386963293";
			//NSString* url = @"https://userpub.itunes.apple.com/WebObjects/MZUserPublishing.woa/wa/addUserReview?id=386963293&type=Purple+Software";
			//NSString* url = @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?id=386963293";
#if defined(IPAD) && !defined(DEPLOYMENT)
#warning using non-iPad review URI
			NSString* url = @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?type=Purple+Software&id=357008398";
#elif defined(IPAD)
			NSString* url = @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?type=Purple+Software&id=492312401";
#else
			NSString* url = @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?type=Purple+Software&id=357008398";
#endif
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
			
			ConfigSetInt("rating_disabled", 1);
		}
		if (buttonIndex == 2)
		{
			// don't ask again
			
			ConfigSetInt("rating_disabled", 1);
		}
	}
}

// ----------------
// UIView overrides
// ----------------

- (void)drawRect:(CGRect)rect
{
	if (m_IsInitialized == false)
		return;
	
	return;
}

- (void)layoutSubviews
{
	try
	{
		if (m_IsInitialized == false)
			return;
		
		m_OpenGLState->MakeCurrent();
	}
	catch (Exception& e)
	{
		[self showException:e];
	}
}

@end

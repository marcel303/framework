#import "AppDelegate.h"
#import "AppView.h"
#import "Calc.h"
#import "eventmgr.h"
#import "game.h"
#import "HelpView.h"
#import "main.h"
#import "OpenGLState.h"
#import "OpenFeint.h"
#import "OFAchievementService.h"
#import "OFHighScoreService.h"
#import "PauseView.h"
#import "player.h"
#import "render_iphone.h"
#import "world.h"

static void Position(UIView* view, float x, float y)
{
	CGAffineTransform transform = CGAffineTransformIdentity;
	transform = CGAffineTransformTranslate(transform, x, y);
	transform = CGAffineTransformRotate(transform, Calc::mPI2);
	[view setCenter:CGPointMake(0.0f, 0.0f)];
	[view.layer setAnchorPoint:CGPointMake(0.0f, 0.0f)];
	[view setTransform:transform];
}

#define CONFIG [NSUserDefaults standardUserDefaults]

void SetInt(NSString* name, int value)
{
	[CONFIG setInteger:value forKey:name];
}

int GetInt(NSString* name)
{
	return [CONFIG integerForKey:name];
}

@implementation AppView

-(id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) 
	{
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		int sx = 320;
		int sy = 480;
		
		openGlState = new OpenGLState();
		openGlState->Initialize(layer, sx, sy, true, true);
		openGlState->CreateBuffers();
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		main = new Main();
		main->Initialize();
		
		scoreLabel = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 20.0f)] autorelease];
		Position(scoreLabel, 320.0f, 0.0f);
		[scoreLabel setFont:[UIFont systemFontOfSize:14.0f]];
		[scoreLabel setText:@""];
		[scoreLabel setTextColor:[UIColor whiteColor]];
		[scoreLabel setOpaque:FALSE];
		[scoreLabel setBackgroundColor:[UIColor clearColor]];
		[self addSubview:scoreLabel];
		
		comboLabel = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 20.0f)] autorelease];
		Position(comboLabel, 320.0f, 320.0f);
		[comboLabel setFont:[UIFont systemFontOfSize:14.0f]];
		[comboLabel setText:@"test"];
		[comboLabel setTextColor:[UIColor whiteColor]];
		[comboLabel setOpaque:FALSE];
		[comboLabel setBackgroundColor:[UIColor clearColor]];
		[self addSubview:comboLabel];
		
		comboLabel2 = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 20.0f)] autorelease];
		Position(comboLabel2, 305.0f, 380.0f);
		[comboLabel2 setFont:[UIFont systemFontOfSize:24.0f]];
		[comboLabel2 setText:@"test"];
		[comboLabel2 setTextColor:[UIColor whiteColor]];
		[comboLabel2 setOpaque:FALSE];
		[comboLabel2 setBackgroundColor:[UIColor clearColor]];
		[self addSubview:comboLabel2];
		
		gameOverLabel = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 20.0f)] autorelease];
		Position(gameOverLabel, 155.0f, 50.0f);
		[gameOverLabel setFont:[UIFont systemFontOfSize:18.0f]];
		[gameOverLabel setText:@"GAME OVER"];
		[gameOverLabel setTextColor:[UIColor whiteColor]];
		[gameOverLabel setOpaque:FALSE];
		[gameOverLabel setBackgroundColor:[UIColor clearColor]];
		[gameOverLabel setHidden:TRUE];
		[self addSubview:gameOverLabel];
		
		touchToRestart = [[[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 20.0f)] autorelease];
		Position(touchToRestart, 105.0f, 100.0f);
		[touchToRestart setFont:[UIFont systemFontOfSize:12.0f]];
		[touchToRestart setText:@"touch to begin"];
		[touchToRestart setTextColor:[UIColor whiteColor]];
		[touchToRestart setOpaque:FALSE];
		[touchToRestart setBackgroundColor:[UIColor clearColor]];
		[touchToRestart setHidden:TRUE];
		[self addSubview:touchToRestart];
		
		restartButton = [UIButton buttonWithType:UIButtonTypeCustom];
		[restartButton setFrame:CGRectMake(0.0f, 0.0f, 164.0f, 44.0f)];
		Position(restartButton, 140.0f, 40.0f);
		[restartButton addTarget:self action:@selector(handleRestart:) forControlEvents:UIControlEventTouchUpInside];
		[restartButton setOpaque:NO];
		[restartButton setBackgroundImage:[UIImage imageNamed:@"play"] forState:UIControlStateNormal];
		[restartButton setAdjustsImageWhenHighlighted:YES];
		[self addSubview:restartButton];
		
		restartButton2 = [UIButton buttonWithType:UIButtonTypeCustom];
		[restartButton2 setFrame:CGRectMake(0.0f, 0.0f, 164.0f, 44.0f)];
		Position(restartButton2, 140.0f, 40.0f);
		[restartButton2 addTarget:self action:@selector(handleRestart:) forControlEvents:UIControlEventTouchUpInside];
		[restartButton2 setOpaque:NO];
		[restartButton2 setBackgroundImage:[UIImage imageNamed:@"play2"] forState:UIControlStateNormal];
		[restartButton2 setAdjustsImageWhenHighlighted:YES];
		[self addSubview:restartButton2];
		
		feintButton = [UIButton buttonWithType:UIButtonTypeCustom];
		[feintButton setFrame:CGRectMake(0.0f, 0.0f, 164.0f, 44.0f)];
		Position(feintButton, 140.0f, 240.0f);
		[feintButton addTarget:self action:@selector(handleFeint:) forControlEvents:UIControlEventTouchUpInside];
		[feintButton setOpaque:NO];
		[feintButton setBackgroundImage:[UIImage imageNamed:@"OFLogoLarge"] forState:UIControlStateNormal];
		[feintButton setAdjustsImageWhenHighlighted:YES];
		[self addSubview:feintButton];
		
		if ([GameCenterManager isGameCenterAvailable])
		{
			[[shooterAppDelegate defaultDelegate].gameCenterMgr authenticateLocalUser];
			
			gcButton = [UIButton buttonWithType:UIButtonTypeCustom];
			[gcButton setFrame:CGRectMake(0.0f, 0.0f, 164.0f, 44.0f)];
			Position(gcButton, 140.0f, 180.0f);
			[gcButton addTarget:self action:@selector(handleGameCenterShowLeaderBoards:) forControlEvents:UIControlEventTouchUpInside];
			[gcButton setOpaque:NO];
			[gcButton setBackgroundImage:[UIImage imageNamed:@"OFLogoLarge"] forState:UIControlStateNormal];
			[gcButton setAdjustsImageWhenHighlighted:YES];
			[self addSubview:gcButton];
		}
		
		helpButton = [UIButton buttonWithType:UIButtonTypeInfoLight];
		[helpButton setFrame:CGRectMake(0.0f, 0.0f, 40.0f, 40.0f)];
		Position(helpButton, 40.0f, 440.0f);
		[helpButton addTarget:self action:@selector(handleHelp:) forControlEvents:UIControlEventTouchUpInside];
		[self addSubview:helpButton];
    }
	
    return self;
}

+(Class)layerClass
{
    return [CAEAGLLayer class];
}

#include "render.h"

-(void)update
{
	//LOG_DBG("update", 0);
	
	openGlState->MakeCurrent();
	
	main->Loop();
	
	[scoreLabel setText:[NSString stringWithFormat:@"SCORE: %07d", gGame->Score_get()]];
	[comboLabel setText:[NSString stringWithFormat:@"COMBO: %d", gWorld->mPlayer->ComboCountB_get()]];
	[comboLabel2 setText:[NSString stringWithFormat:@"x%d", gWorld->mPlayer->ComboCountA_get()]];
	
	openGlState->Present();
	
	//
	
	Event e;
	
	while (gEventMgr->Pop(e))
	{
		switch (e.type)
		{
			case EventType_Combo:
				[self checkAchievement_Combo:e.intValue];
				break;
			case EventType_Begin:
			{
				// increment play count
				int count = GetInt(@"play_count") + 1;
				SetInt(@"play_count", count);
				// check achievements
				[self checkAchievement_PlayCount:count];
				playTime1 = time(&playTime1);
				playTime = 0.0;
				break;
			}
			case EventType_End:
			{
				// update play time
				playTime2 = time(&playTime2);
				playTime += difftime(playTime1, playTime2);
				int time = GetInt(@"play_time") + (int)playTime;
				SetInt(@"play_time", time);
				// check achievements
				[self checkAchievement_PlayTime:time];
				break;
			}
			case EventType_Die:
			{
				// increment die count
				int count = GetInt(@"die_count") + 1;
				SetInt(@"die_count", count);
				// check achievements
				[self checkAchievement_Die:count];
				// submit highscore
				LOG_DBG("submitting score", 0);
				[OFHighScoreService setHighScore:gGame->Score_get() forLeaderboard:@"426514" onSuccess:OFDelegate() onFailure:OFDelegate()];
				break;
			}
		}
	}
	
	[gameOverLabel setHidden:gGame->IsPlaying_get()];
	[touchToRestart setHidden:gGame->IsPlaying_get() || fmodf(gGame->Time_get(), 1.0f) < 0.5f];
	[helpButton setHidden:gGame->IsPlaying_get()];
	
	[self menuShow:!gGame->IsPlaying_get()];
}

-(void)handleRestart:(id)sender
{
	if (gGame->IsPlaying_get())
	{
		gGame->End();
	}
	
	gGame->Begin();
	
	//
	
	RenderIphone* render = (RenderIphone*)gRender;
	
	render->CalibrateTilt();
}

-(void)handleGameCenterLogin:(id)sender
{
	[self handleGameCenterShowLeaderBoards:sender];
}

-(void)handleGameCenterShowLeaderBoards:(id)sender
{
	[[shooterAppDelegate defaultDelegate].gameCenterMgr reportScore:5000 forCategory:@"2000"];
	[[shooterAppDelegate defaultDelegate].gameCenterMgr reloadHighScoresForCategory:@"1000"];
	
	NSString* leaderBoard_Easy = @"1000";
	NSString* leaderBoard_Hard = @"2000";
	NSString* leaderBoard_Custom = @"3000";
	
	GKLeaderboardViewController* controller = [[GKLeaderboardViewController alloc] init];
	
	if (controller != nil)
	{
		controller.category = leaderBoard_Easy;
        controller.timeScope = GKLeaderboardTimeScopeAllTime;
        controller.leaderboardDelegate = self;
		[[shooterAppDelegate defaultDelegate].viewController presentModalViewController:controller animated:YES];
	}
}

-(void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController *)viewController
{
	[[shooterAppDelegate defaultDelegate].viewController dismissModalViewControllerAnimated:YES];
    [viewController release];
}
	
-(void)handleFeint:(id)sender
{
	[OpenFeint launchDashboard];
}

-(void)handleHelp:(id)sender
{
	[[shooterAppDelegate defaultDelegate] showView:[shooterAppDelegate defaultDelegate].helpView];
}

-(void)checkAchievement_Combo:(int)countA
{
	if (countA >= 100)
	{
		[OFAchievementService unlockAchievement:@"493484"];
	}
	
	if (countA >= 1000)
	{
		[OFAchievementService unlockAchievement:@"493494"];
	}
}

-(void)checkAchievement_Die:(int)count
{
	if (count >= 1)
	{
		[OFAchievementService unlockAchievement:@"493454"];
	}
	
	if (count >= 2)
	{
		[OFAchievementService unlockAchievement:@"493464"];
	}
	
	if (count >= 150)
	{
		[OFAchievementService unlockAchievement:@"493474"];
	}
}

-(void)checkAchievement_PlayCount:(int)count
{
	if (count >= 5)
	{
		[OFAchievementService unlockAchievement:@"518704"];
	}
	
	if (count >= 20)
	{
		[OFAchievementService unlockAchievement:@"518714"];
	}
	
	if (count >= 100)
	{
		[OFAchievementService unlockAchievement:@"518724"];
	}
	
	if (count >= 1000)
	{
		[OFAchievementService unlockAchievement:@"518734"];
	}
}

-(void)checkAchievement_PlayTime:(int)time
{
	if (time >= 600)
	{
		[OFAchievementService unlockAchievement:@"518674"];
	}
	
	if (time >= 3600)
	{
		[OFAchievementService unlockAchievement:@"518684"];
	}
	
	if (time >= 36000)
	{
		[OFAchievementService unlockAchievement:@"518694"];
	}
}

-(void)menuShow:(bool)show
{
	BOOL hidden = !show;
	
	[gameOverLabel setHidden:hidden || fmodf(gGame->Time_get(), 1.0f) < 0.5f];
	[restartButton setHidden:hidden || fmodf(gGame->Time_get(), 1.0f) < 0.5f];
	[restartButton2 setHidden:hidden || fmodf(gGame->Time_get(), 1.0f) >= 0.5f];
	[feintButton setHidden:hidden];
}

-(void)pause
{
	LOG_INF("pause: begin", 0);
	
	[updateTimer invalidate];
	updateTimer = nil;
}

-(void)resume
{
	LOG_INF("pause: end", 0);
	
	if (updateTimer == nil)
	{
		updateTimer = [NSTimer scheduledTimerWithTimeInterval:1.0f/120.0f target:self selector:@selector(update) userInfo:nil repeats:YES];
	}
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (gGame->IsPlaying_get())
	{
		[[shooterAppDelegate defaultDelegate] showView:[shooterAppDelegate defaultDelegate].pauseView];
	}
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)dealloc 
{
	main->Shutdown();
	delete main;
	main = 0;
	
    [super dealloc];
}


@end

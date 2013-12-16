#import <Foundation/Foundation.h>
#import <GameKit/GameKit.h>
#import "GameCenter.h"
#import "GameCenterManager.h"
#import "grs_types.h"

GameCenter * g_gameCenter = 0;

@interface GameCenterHelper : NSObject <GameCenterManagerDelegate, GKGameCenterControllerDelegate, GKLeaderboardViewControllerDelegate>
{
	GameCenterManager* gcMgr;
}

-(bool)gcIsAvailable;
-(bool)gcIsLoggedIn;
-(void)gcLogin;
-(void)gcScoreSubmit:(const char*)category score:(float)score;
-(void)gcScoreList:(const char*)category rangeBegin:(uint32_t)rangeBegin rangeEnd:(uint32_t)rangeEnd history:(uint32_t)history;

// -------------------------
// GameCenterManagerDelegate
// -------------------------
- (void)processGameCenterAuth:(NSError*)error;
- (void)scoreReported:(GKScore*)score error:(NSError*)error;
- (void)reloadScoresComplete:(GKLeaderboard*)leaderBoard scores:(NSArray*)scores players:(NSArray*)players error:(NSError*) error;
- (void)achievementSubmitted:(GKAchievement*)ach error:(NSError*)error;
- (void)achievementResetResult:(NSError*)error;
- (void)mappedPlayerIDToPlayer:(GKPlayer*)player error: (NSError*)error;

// -------------------------
// GameCenter UI
// -------------------------
-(void)displayGameCenter;
-(void)gameCenterViewControllerDidFinish:(GKGameCenterViewController *)gameCenterViewController;
-(void)displayLeaderBoard:(NSString*)name;
-(void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)leaderboardController;

@end

static GameCenterHelper * g_gameCenterHelper = nil;

@implementation GameCenterHelper

-(id)init
{
	@try
	{
		if ((self = [super init]))
		{
			if ([self gcIsAvailable])
			{
				gcMgr = [[GameCenterManager alloc] init];
				gcMgr.delegate = self;
			}
		}
		
		return self;
	}
	@catch (NSException* e)
	{
		LOG_ERR("failed to init game center helper", 0);
		
		return nil;
	}
}

-(void)dealloc
{
	[gcMgr dealloc];
	gcMgr = nil;
	
	[super dealloc];
}

-(bool)gcIsAvailable
{
	@try
	{
		return [GameCenterManager isGameCenterAvailable];
	}
	@catch (NSException* e)
	{
		LOG_ERR("gc is available failed", 0);
		
		return false;
	}
}

-(bool)gcIsLoggedIn
{
	@try
	{
		if (![self gcIsAvailable])
			return false;
		
		return [GKLocalPlayer localPlayer].authenticated == YES;
	}
	@catch (NSException* e)
	{
		LOG_ERR("gc is logged in failed", 0);
		
		return false;
	}
}

-(void)gcLogin
{
	@try
	{
		if (![self gcIsAvailable] || [self gcIsLoggedIn])
			return;
		
		[gcMgr authenticateLocalUser];
	}
	@catch (NSException* e)
	{
		LOG_ERR("gc login failed", 0);
	}
}

-(void)gcScoreSubmit:(const char*)category score:(float)score
{
	@try
	{
		if (![self gcIsAvailable] || ![self gcIsLoggedIn])
			return;
		
		uint64_t score64 = static_cast<uint64_t>(score * 100.0f);
		NSString* nsCategory = [NSString stringWithCString:category encoding:NSASCIIStringEncoding];
		
		[gcMgr reportScore:score64 forCategory:nsCategory];
	}
	@catch (NSException* e)
	{
		LOG_ERR("gc score submit failed", 0);
	}
}

-(void)gcScoreList:(const char*)category rangeBegin:(uint32_t)rangeBegin rangeEnd:(uint32_t)rangeEnd history:(uint32_t)history
{
	@try
	{
		if (![self gcIsAvailable] || ![self gcIsLoggedIn])
			return;
		
		NSString* nsCategory = [NSString stringWithCString:category encoding:NSASCIIStringEncoding];
		
		[gcMgr reloadHighScoresForCategory:nsCategory rangeBegin:rangeBegin rangeEnd:rangeEnd history:history];
	}
	@catch (NSException* e)
	{
		LOG_ERR("gc score list failed", 0);
	}
}

// -------------------------
// GameCenterManagerDelegate
// -------------------------

-(void)processGameCenterAuth:(NSError*)error
{
	LOG_INF("game center: completed auth", 0);
	
	if (error != nil)
	{
		LOG_ERR("game center: error: %s", [[error description] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	else if ([self gcIsLoggedIn])
	{
		if (g_gameCenter->OnLoginComplete.IsSet())
			g_gameCenter->OnLoginComplete.Invoke(0);
	}
	else
	{
		LOG_WRN("game center: no error, but not logged in?", 0);
	}
}

-(void)scoreReported:(GKScore*)score error:(NSError*)error
{
	LOG_INF("game center: completed score report", 0);
	
	if (error != nil)
	{
		LOG_ERR("game center: error: %s", [[error description] cStringUsingEncoding:NSASCIIStringEncoding]);
		
		if (g_gameCenter->OnScoreSubmitError.IsSet())
			g_gameCenter->OnScoreSubmitError.Invoke(0);
	}
	else if (score != nil)
	{
		int rank = score.rank - 1;
		
		if (g_gameCenter->OnScoreSubmitComplete.IsSet())
			g_gameCenter->OnScoreSubmitComplete.Invoke(&rank);
	}
	else
	{
		LOG_ERR("game center: score submit ok, but no score?", 0);
	}
}

-(void)reloadScoresComplete:(GKLeaderboard*)leaderBoard scores:(NSArray*)scores players:(NSArray*)players error:(NSError*)error
{
	LOG_INF("game center: completed score reload", 0);
	
	if (error != nil)
	{
		LOG_ERR("game center: error: %s", [[error description] cStringUsingEncoding:NSASCIIStringEncoding]);
		
		if (g_gameCenter->OnScoreListError.IsSet())
			g_gameCenter->OnScoreListError.Invoke(0);
	}
	else
	{
		//construct GRS query result object for compatibility's sake
		
		GRS::QueryResult qr;
		
		qr.m_ScoreCount = leaderBoard.maxRange;
		qr.m_ResultBegin = leaderBoard.range.location - 1;
		if (leaderBoard.localPlayerScore != nil)
			qr.m_RankBest = leaderBoard.localPlayerScore.rank - 1;
		else
			qr.m_RankBest = -1;
			
		uint32_t scoreCount = scores.count;
		
		for (uint32_t i = 0; i < scoreCount; ++i)
		{
			GKScore * score = [scores objectAtIndex:i];
			GKPlayer * player = [players objectAtIndex:i];
			//const char * playerName = [player.alias cStringUsingEncoding:NSASCIIStringEncoding];
//			if (playerName == 0)
//				playerName = "(unknown)";
			
			NSData* data = [player.alias dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
			std::string playerName;
			if (data.length == 0)
			{
				playerName = "(noname)";
			}
			else
			{
				const char* chars = (const char*)data.bytes;
				
				for (NSUInteger j = 0; j < data.length; ++j)
				{
					playerName.push_back(chars[j]);
				}
			}
			
			GRS::HighScore gs;
			
			gs.Setup(0, 0, GRS::UserId(""), playerName, "", static_cast<uint32_t>(score.value) / 100.0f, GRS::TimeStamp(), GRS::GpsLocation(), GRS::CountryCode(), false);

			qr.m_Scores.push_back(gs);
		}
		
		if (g_gameCenter->OnScoreListComplete.IsSet())
			g_gameCenter->OnScoreListComplete.Invoke(&qr);
	}
}

-(void)achievementSubmitted:(GKAchievement*)ach error:(NSError*)error
{
	LOG_INF("game center: completed achievement submit", 0);
	
	if (error != nil)
	{
		LOG_ERR("game center: error: %s", [[error description] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	else
	{
	}
}

-(void)achievementResetResult:(NSError*)error
{
	LOG_INF("game center: completed archievement reset", 0);
	
	if (error != nil)
	{
		LOG_ERR("game center: error: %s", [[error description] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	else
	{
	}
}

-(void)mappedPlayerIDToPlayer:(GKPlayer*)player error:(NSError*)error
{
	LOG_INF("game center: completed player ID map", 0);
	
	if (error != nil)
	{
		LOG_ERR("game center: error: %s", [[error description] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	else
	{
	}
}

// -------------------------
// GameCenter UI
// -------------------------

-(void)displayGameCenter
{
	UIViewController * viewController = [UIApplication sharedApplication].keyWindow.rootViewController;
	
    GKGameCenterViewController * gameCenterController = [[[GKGameCenterViewController alloc] init] autorelease];
	
    if (gameCenterController != nil)
    {
		gameCenterController.gameCenterDelegate = self;
		
		[viewController presentViewController:gameCenterController animated:YES completion:nil];
    }
}

-(void)gameCenterViewControllerDidFinish:(GKGameCenterViewController*)gameCenterViewController
{
	UIViewController * viewController = [UIApplication sharedApplication].keyWindow.rootViewController;
	
	[viewController dismissViewControllerAnimated:YES completion:nil];
}

-(void)displayLeaderBoard:(NSString*)name
{
	UIViewController * viewController = [UIApplication sharedApplication].keyWindow.rootViewController;
	
	GKLeaderboardViewController * leaderboardController = [[[GKLeaderboardViewController alloc] init] autorelease];
	
	if (leaderboardController != NULL)
	{
		if (name != nil)
			leaderboardController.category = name;
		leaderboardController.timeScope = GKLeaderboardTimeScopeAllTime;
		leaderboardController.leaderboardDelegate = self;
		
		[viewController presentViewController:leaderboardController animated:YES completion:nil];
	}
}

-(void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)leaderboardController
{
	UIViewController * viewController = [UIApplication sharedApplication].keyWindow.rootViewController;
	
	[viewController dismissViewControllerAnimated:YES completion:nil];
}

@end

//

GameCenter::GameCenter()
{
	Assert(g_gameCenterHelper == nil);
	
	if (g_gameCenterHelper == nil)
	{
		g_gameCenterHelper = [[GameCenterHelper alloc] init];
	}
}

GameCenter::~GameCenter()
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		[g_gameCenterHelper dealloc];
		g_gameCenterHelper = nil;
	}
}

bool GameCenter::IsAvailable()
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		return [g_gameCenterHelper gcIsAvailable];
	}
	else
	{
		return false;
	}
}

bool GameCenter::IsLoggedIn()
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		return [g_gameCenterHelper gcIsLoggedIn];
	}
	else
	{
		return false;
	}
}

void GameCenter::Login()
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		[g_gameCenterHelper gcLogin];
	}
}

void GameCenter::ScoreSubmit(const char* category, float score)
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		[g_gameCenterHelper gcScoreSubmit:category score:score];
	}
}

void GameCenter::ScoreList(const char* category, uint32_t rangeBegin, uint32_t rangeEnd, uint32_t history)
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		[g_gameCenterHelper gcScoreList:category rangeBegin:rangeBegin+1 rangeEnd:rangeEnd+1 history:history];
	}
}

void GameCenter::ShowGameCenter()
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		[g_gameCenterHelper displayGameCenter];
	}
}

void GameCenter::ShowLeaderBoard(const char* category)
{
	Assert(g_gameCenterHelper != nil);
	
	if (g_gameCenterHelper != nil)
	{
		[g_gameCenterHelper displayLeaderBoard:[NSString stringWithCString:category encoding:NSASCIIStringEncoding]];
	}
}

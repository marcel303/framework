#import <GameKit/GameKit.h>
#import <sys/utsname.h>
#import "Debugging.h"
#import "GameCenterManager.h"
#import "Log.h"

// Disable the annoying warnings about self being shadowed
#pragma GCC diagnostic ignored "-Wshadow"

@implementation GameCenterManager

@synthesize earnedAchievementCache;
@synthesize delegate;

-(id)init
{
	if ((self = [super init]))
	{
		earnedAchievementCache = NULL;
	}
	
	return self;
}

-(void)dealloc
{
	self.earnedAchievementCache = NULL;
	[super dealloc];
}

// NOTE:  GameCenter does not guarantee that callback blocks will be executed on the main thread. 
// As such, your application needs to be very careful in how it handles references to view
// controllers. If a view controller is referenced in a block that executes on a secondary queue,
// that view controller may be released (and dealloc'd) outside the main queue. This is true
// even if the actual block is scheduled on the main thread. In concrete terms, this code
// snippet is not safe, even though viewController is dispatching to the main queue:
//
//	[object doSomethingWithCallback:  ^()
//	{
//		dispatch_async(dispatch_get_main_queue(), ^(void)
//		{
//			[viewController doSomething];
//		});
//	}];
//
// UIKit view controllers should only be accessed on the main thread, so the snippet above may
// lead to subtle and hard to trace bugs. Many solutions to this problem exist. In this sample,
// I'm bottlenecking everything through "callDelegateOnMainThread" which calls "callDelegate". 
// Because "callDelegate" is the only method to access the delegate, I can ensure that delegate
// is not visible in any of my block callbacks.

-(void)callDelegate:(SEL)selector withArg:(id)arg error:(NSError*)error
{
	Assert([NSThread isMainThread]);
	
	if ([delegate respondsToSelector:selector])
	{
		if (arg != NULL)
		{
			[delegate performSelector:selector withObject:arg withObject:error];
		}
		else
		{
			[delegate performSelector:selector withObject:error];
		}
	}
	else
	{
		Assert(false);
		NSLog(@"Missed Method");
	}
}

-(void)callDelegateOnMainThread:(SEL)selector withArg:(id)arg error:(NSError*)error
{
	dispatch_async(dispatch_get_main_queue(), ^(void)
	{
		[self callDelegate:selector withArg:arg error:error];
	});
}

-(void)callSelf:(SEL)selector withArg:(id)arg error:(NSError*)error
{
	Assert([NSThread isMainThread]);
	
	if ([self respondsToSelector:selector])
	{
		if (arg != NULL)
		{
			[self performSelector:selector withObject:arg withObject:error];
		}
		else
		{
			[self performSelector:selector withObject:error];
		}
	}
	else
	{
		Assert(false);
		NSLog(@"Missed Method");
	}
}

-(void)callSelfOnMainThread:(SEL)selector withArg:(id)arg error:(NSError*)error
{
	dispatch_async(dispatch_get_main_queue(), ^(void)
	{
		[self callSelf:selector withArg:arg error:error];
	});
}

static NSError* MakeError(NSString* description)
{
	NSMutableDictionary* details = [NSMutableDictionary dictionary];
	[details setValue:description forKey:NSLocalizedDescriptionKey];
	return [NSError errorWithDomain:@"world" code:200 userInfo:details];
}

// --------------------------------
// GameCenterManager implementation
// --------------------------------

+(BOOL)isGameCenterAvailable
{
	Class gcClass = (NSClassFromString(@"GKLocalPlayer"));
	
	// check if the device is running iOS 4.1 or later
	NSString* req = @"4.1";
	NSString* cur = [[UIDevice currentDevice] systemVersion];
	BOOL osVersionSupported = ([cur compare:req options:NSNumericSearch] != NSOrderedAscending);
	
	if (!gcClass || !osVersionSupported)
	{
		return false;
	}
	else
	{
		struct utsname systemInfo;
		uname(&systemInfo);
		const char* name = systemInfo.machine;
		
		//iPhone 3G can run iOS 4.1 but does not support Game Center
		if (!strcmp(name, "iPhone1,1") || !strcmp(name, "iPhone1,2") || !strcmp(name, "iPod1,1"))
			return false;
		else
			return true;
	}
}


-(void)authenticateLocalUser
{
	Assert([GameCenterManager isGameCenterAvailable]);
	
	if ([GKLocalPlayer localPlayer].authenticated == NO)
	{
		[[GKLocalPlayer localPlayer] authenticateWithCompletionHandler:^(NSError *error) 
		{
			[self callDelegateOnMainThread:@selector(processGameCenterAuth:) withArg:NULL error:error];
		}];
	}
}

-(void)reloadHighScoresForCategory:(NSString*)category rangeBegin:(uint32_t)rangeBegin rangeEnd:(uint32_t)rangeEnd history:(uint32_t)history
{
	Assert(category != nil);
	Assert(rangeBegin >= 1 && rangeEnd >= rangeBegin);
	
	GKLeaderboard* leaderBoard = [[[GKLeaderboard alloc] init] autorelease];
	leaderBoard.category = category;
	if (history == 0)
		leaderBoard.timeScope = GKLeaderboardTimeScopeAllTime;
	else if (history == 7)
		leaderBoard.timeScope = GKLeaderboardTimeScopeWeek;
	else
	{
		Assert(false);
		leaderBoard.timeScope = GKLeaderboardTimeScopeAllTime;
	}
	leaderBoard.range = NSMakeRange(rangeBegin, rangeEnd);
	
	[leaderBoard loadScoresWithCompletionHandler:^(NSArray* scores, NSError* error)
	{
		if (error != nil)
		{
			LOG_WRN("game center: load scores: error: %s", [error.description cStringUsingEncoding:NSASCIIStringEncoding]);
			
			[self callSelfOnMainThread:@selector(reloadScoresComplete:error:) withArg:[[NSArray alloc] initWithObjects:leaderBoard, nil] error:error];
		}
		else
		{
			if (scores == nil)
			{
				LOG_DBG("game center: load scores: empty score list", 0);
				
				id ids[3] = { leaderBoard, [[[NSArray alloc] init] autorelease], [[[NSArray alloc] init] autorelease] };
				NSArray* args = [[NSArray alloc] initWithObjects:ids count:3];
				
                [self callSelfOnMainThread:@selector(reloadScoresComplete:error:) withArg:args error:nil];
			}
			else
			{
				const uint32_t scoreCount = scores.count;
				
				LOG_DBG("game center: load scores: got %u scores", scoreCount);
				
				// make array of player IDs
				
				NSString** ids = (NSString**)alloca(scoreCount * sizeof(NSString*));
				
				for (uint32_t i = 0; i < scoreCount; ++i)
				{
					GKScore * score = [scores objectAtIndex:i];
					//NSData * data = [NSKeyedArchiver archivedDataWithRootObject:_score];
					//GKScore * score = [[NSKeyedUnarchiver unarchiveObjectWithData:data] retain];
					NSString * playerID = score.playerID;
					ids[i] = playerID;
				}
				
				NSArray * playerIDs = [NSArray arrayWithObjects:ids count:scoreCount];
				
				[GKPlayer loadPlayersForIdentifiers:playerIDs withCompletionHandler:^(NSArray* players, NSError* error2)
				{
					if (error2 != nil)
					{
						LOG_WRN("game center: load player IDs: error: %s", [error2.description cStringUsingEncoding:NSASCIIStringEncoding]);
						
						[self callSelfOnMainThread:@selector(reloadScoresComplete:error:) withArg:[[NSArray alloc] initWithObjects:leaderBoard, nil] error:error2];
					}
					else
					{
						if (players == nil)
						{
							LOG_WRN("game center: load player IDs: got empty player ID list", 0);
							
							[self callSelfOnMainThread:@selector(reloadScoresComplete:error:) withArg:[[NSArray alloc] initWithObjects:leaderBoard, nil] error:MakeError(@"got empty player list for scores")];
						}
						else
						{
							id ids2[3] = { leaderBoard, scores, players };
							NSArray* args = [[NSArray alloc] initWithObjects:ids2 count:3];
							
							[self callSelfOnMainThread:@selector(reloadScoresComplete:error:) withArg:args error:error2];
						}
					}
				}];
			}
		}
	}];
}

-(void)reloadScoresComplete:(NSArray*)args error:(NSError*)error
{   
    Assert([args count] >= 1);
    
	GKLeaderboard* leaderBoard = [args objectAtIndex:0];
	
	if (error != nil)
	{
		LOG_ERR("game center: reload scores: error: %s", [error.description cStringUsingEncoding:NSASCIIStringEncoding]);
		
		[delegate reloadScoresComplete:leaderBoard scores:nil players:nil error:error];
	}
	else
	{
		NSArray* scores = [args objectAtIndex:1];
		NSArray* players = [args objectAtIndex:2];
		
		Assert(scores && players);
		
        if (scores && players)
        {
            [delegate reloadScoresComplete:leaderBoard scores:scores players:players error:nil];
        }
	}
	
	//[args dealloc];
}

-(void)reportScore:(int64_t)_score forCategory:(NSString*)category 
{
	Assert(category != nil);
	
	LOG_INF("game center: score submit: category=%s", [category cStringUsingEncoding:NSASCIIStringEncoding]);
	
	GKScore* score = [[[GKScore alloc] initWithCategory:category] autorelease];
	
	if (score == nil)
	{
		LOG_ERR("game center: score submit: score is nil", 0);
		
		[self callDelegateOnMainThread: @selector(scoreReported:error:) withArg:score error:MakeError(@"score is nil")];
	}
	else
	{
		score.value = _score;
		
		[score reportScoreWithCompletionHandler:^(NSError* error) 
		{
			if (error != nil)
			{
				LOG_ERR("game center: score submit: error: %s", [error.description cStringUsingEncoding:NSASCIIStringEncoding]);
				
				[self callDelegateOnMainThread: @selector(scoreReported:error:) withArg:score error:error];
			}
			else
			{
				//NSString* playerID = [[GKLocalPlayer localPlayer] playerID]; 
				//NSArray* playerIDs = [NSArray arrayWithObject:playerID];
				//GKLeaderboard* leaderBoard = [[[GKLeaderboard alloc] initWithPlayerIDs:playerIDs] autorelease];
				GKLeaderboard* leaderBoard = [[[GKLeaderboard alloc] init] autorelease];

				//Assert(playerID != nil);
				
				//LOG_INF("game center: score submit: playerID=%s", [playerID cStringUsingEncoding:NSASCIIStringEncoding]);
				
				if (leaderBoard == nil)
				{
					LOG_ERR("game center: score submit: leaderboard is nil", 0);
					
					[self callDelegateOnMainThread: @selector(scoreReported:error:) withArg:score error:MakeError(@"leaderboard is nil")];
					
					[leaderBoard dealloc];
				}
				else
				{
					[NSThread sleepForTimeInterval:1.0];
									
#if 1
					leaderBoard.category = category;
#endif
#if 0
					leaderBoard.timeScope = GKLeaderboardTimeScopeAllTime;
					leaderBoard.range = NSMakeRange(1, 1);
#endif
					
					[leaderBoard loadScoresWithCompletionHandler:^(NSArray* scores, NSError* error2)
					{
						if (error2 != nil)
						{
							LOG_ERR("game center: score submit: error: %s", [error2.description cStringUsingEncoding:NSASCIIStringEncoding]);
							
							[self callDelegateOnMainThread: @selector(scoreReported:error:) withArg:score error:error2];
						}
						else if (leaderBoard.localPlayerScore == nil/* && (scores == nil || scores.count == 0)*/)
						{
							LOG_ERR("game center: score submit: empty score list", 0);
							
							[self callDelegateOnMainThread: @selector(scoreReported:error:) withArg:score error:MakeError(@"unable to fetch current rank")];
						}
						else
						{
							GKScore* score2 = leaderBoard.localPlayerScore;
							
							//if (scores != nil && scores.count != 0)
							//	score2 = [scores objectAtIndex:0];

							Assert(score2);
							
							LOG_ERR("game center: score submit: submitted score: rank:%u, score:%u", (uint32_t)score2.rank, (uint32_t)score2.value);
								
							[self callDelegateOnMainThread: @selector(scoreReported:error:) withArg:score2 error:error];
						}
					}];
				}
			}
		}];
	}
}

- (void)submitAchievement:(NSString*)identifier percentComplete:(double)percentComplete
{
	//GameCenter check for duplicate achievements when the achievement is submitted, but if you only want to report 
	// new achievements to the user, then you need to check if it's been earned 
	// before you submit.  Otherwise you'll end up with a race condition between loadAchievementsWithCompletionHandler
	// and reportAchievementWithCompletionHandler.  To avoid this, we fetch the current achievement list once,
	// then cache it and keep it updated with any new achievements.
	if (self.earnedAchievementCache == NULL)
	{
		[GKAchievement loadAchievementsWithCompletionHandler: ^(NSArray *scores, NSError *error)
		{
			if (error == nil)
			{
				NSMutableDictionary* tempCache= [NSMutableDictionary dictionaryWithCapacity: [scores count]];
				for (GKAchievement* score in tempCache)
				{
					[tempCache setObject: score forKey: score.identifier];
				}
				self.earnedAchievementCache= tempCache;
				[self submitAchievement: identifier percentComplete: percentComplete];
			}
			else
			{
				//Something broke loading the achievement list.  Error out, and we'll try again the next time achievements submit.
				[self callDelegateOnMainThread: @selector(achievementSubmitted:error:) withArg: NULL error: error];
			}

		}];
	}
	else
	{
		 //Search the list for the ID we're using...
		GKAchievement* achievement = [self.earnedAchievementCache objectForKey:identifier];
		if (achievement != nil)
		{
			if ((achievement.percentComplete >= 100.0) || (achievement.percentComplete >= percentComplete))
			{
				//Achievement has already been earned so we're done.
				achievement = NULL;
			}
			
			achievement.percentComplete= percentComplete;
		}
		else
		{
			achievement = [[[GKAchievement alloc] initWithIdentifier: identifier] autorelease];
			achievement.percentComplete= percentComplete;
			//Add achievement to achievement cache...
			[self.earnedAchievementCache setObject: achievement forKey: achievement.identifier];
		}
		
		if (achievement != nil)
		{
			//Submit the Achievement...
			[achievement reportAchievementWithCompletionHandler: ^(NSError *error)
			{
				 [self callDelegateOnMainThread: @selector(achievementSubmitted:error:) withArg: achievement error: error];
			}];
		}
	}
}

-(void)resetAchievements
{
	self.earnedAchievementCache = NULL;
	[GKAchievement resetAchievementsWithCompletionHandler:^(NSError *error) 
	{
		 [self callDelegateOnMainThread:@selector(achievementResetResult:) withArg:NULL error:error];
	}];
}

-(void)mapPlayerIDtoPlayer:(NSString*)playerID
{
	[GKPlayer loadPlayersForIdentifiers:[NSArray arrayWithObject:playerID] withCompletionHandler:^(NSArray *playerArray, NSError *error)
	{
		GKPlayer* player = nil;
		
		for (GKPlayer* tempPlayer in playerArray)
		{
			if ([tempPlayer.playerID isEqualToString:playerID])
			{
				player = tempPlayer;
				break;
			}
		}
		
		[self callDelegateOnMainThread: @selector(mappedPlayerIDToPlayer:error:) withArg:player error:error];
	}];
	
}

@end

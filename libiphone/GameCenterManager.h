#import <Foundation/Foundation.h>

@class GKAchievement;
@class GKLeaderboard;
@class GKPlayer;
@class GKScore;

@protocol GameCenterManagerDelegate <NSObject>

@optional
-(void)processGameCenterAuth:(NSError*)error;
-(void)scoreReported:(GKScore*)score error:(NSError*)error;
-(void)reloadScoresComplete:(GKLeaderboard*)leaderBoard scores:(NSArray*)scores players:(NSArray*)players error:(NSError*)error;
-(void)achievementSubmitted:(GKAchievement*)ach error:(NSError*)error;
-(void)achievementResetResult:(NSError*)error;
-(void)mappedPlayerIDToPlayer:(GKPlayer*)player error:(NSError*)error;

@end

@interface GameCenterManager : NSObject
{
	NSMutableDictionary* earnedAchievementCache;
	
	id<GameCenterManagerDelegate, NSObject> delegate;
}

// Note: this property must be atomic to ensure that the cache is always in a viable state
@property (retain) NSMutableDictionary* earnedAchievementCache;
@property (nonatomic, assign) id<GameCenterManagerDelegate> delegate;

+(BOOL)isGameCenterAvailable;
-(void)authenticateLocalUser;
-(void)reportScore:(int64_t)score forCategory:(NSString*)category;
-(void)reloadHighScoresForCategory:(NSString*)category rangeBegin:(uint32_t)rangeBegin rangeEnd:(uint32_t)rangeEnd history:(uint32_t)history;
-(void)reloadScoresComplete:(NSArray*)args error:(NSError*)error;
-(void)submitAchievement:(NSString*)identifier percentComplete:(double) percentComplete;
-(void)resetAchievements;
-(void)mapPlayerIDtoPlayer:(NSString*)playerID;

@end

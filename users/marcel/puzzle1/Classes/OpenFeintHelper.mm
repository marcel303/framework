#import "AppDelegate.h"
#import "OFHighscoreService.h"
#import "OFAchievementService.h"
#import "OpenFeintHelper.h"

@implementation OpenFeintHelper

static void Achieve(NSString* id)
{
	if (![[AppDelegate mainApp] openfeintEnabled])
		return;
	
	[OFAchievementService unlockAchievement:id];
}

static void Score(NSString* id, int score)
{
	if (![[AppDelegate mainApp] openfeintEnabled])
		return;
	
	[OFHighScoreService setHighScore:score forLeaderboard:id onSuccess:OFDelegate() onFailure:OFDelegate()];
}

+(void)rankTime:(PuzzleMode)mode size:(int)size time:(int)time
{
	NSString* id_ = nil;
	
	switch (mode)
	{
		case PuzzleMode_Free:
		{
			switch (size)
			{
				case 3:
				{
					id_ = @"429324";
					break;
				}
					
				case 4:
				{
					id_ = @"429344";
					break;
				}
					
				case 5:
				{
					id_ = @"480144";
					break;
				}
			}
			
			break;
		}
			
		case PuzzleMode_Switch:
		{
			switch (size)
			{
				case 3:
				{
					id_ = @"429364";
					break;
				}
					
				case 4:
				{
					id_ = @"429384";
					break;
				}
					
				case 5:
				{
					id_ = @"480164";
					break;
				}
			}
			
			break;
		}
	}
	
	if (id_ != nil)
	{
		Score(id_, time);
	}
	else
	{
		LOG_ERR("unable to find score board", 0);
	}
}

+(void)rankMoves:(PuzzleMode)mode size:(int)size moveCount:(int)moveCount
{
	NSString* id_ = nil;
	
	switch (mode)
	{
		case PuzzleMode_Free:
		{
			switch (size)
			{
				case 3:
				{
					id_ = @"429334";
					break;
				}
					
				case 4:
				{
					id_ = @"429354";
					break;
				}
					
				case 5:
				{
					id_ = @"480154";
					break;
				}
			}
			
			break;
		}
			
		case PuzzleMode_Switch:
		{
			switch (size)
			{
				case 3:
				{
					id_ = @"429374";
					break;
				}
					
				case 4:
				{
					id_ = @"429394";
					break;
				}
					
				case 5:
				{
					id_ = @"480174";
					break;
				}
			}
			
			break;
		}
	}
	
	if (id_ != nil)
	{
		Score(id_, moveCount);
	}
	else
	{
		LOG_ERR("unable to find score board", 0);
	}
}

+(void)achieveTime:(PuzzleMode)mode size:(int)size time:(int)time
{
	switch (mode)
	{
		case PuzzleMode_Free:
		{
			switch (size)
			{
				case 3:
				{
					if (time <= 30) Achieve(@"497394");
					if (time <= 20) Achieve(@"497404");
					if (time <= 10) Achieve(@"497414");
					if (time <= 5) Achieve(@"497424");
					break;
				}
					
				case 4:
				{
					if (time <= 120) Achieve(@"497434");
					if (time <= 40) Achieve(@"497444");
					if (time <= 30) Achieve(@"497454");
					if (time <= 15) Achieve(@"497464");
					break;
				}
			}
			
			break;
		}
			
		case PuzzleMode_Switch:
		{
			switch (size)
			{
				case 3:
				{
					if (time <= 300) Achieve(@"497474");
					if (time <= 180) Achieve(@"497484");
					if (time <= 60) Achieve(@"497494");
					if (time <= 30) Achieve(@"497504");
					break;
				}
					
				case 4:
				{
					if (time <= 600) Achieve(@"497514");
					if (time <= 240) Achieve(@"497524");
					if (time <= 120) Achieve(@"497534");
					if (time <= 60) Achieve(@"497544");
					break;
				}
			}
			
			break;
		}
	}
}

+(void)achieveMoves:(PuzzleMode)mode size:(int)size moveCount:(int)moveCount
{
	switch (mode)
	{
		case PuzzleMode_Free:
		{
			switch (size)
			{
				case 3:
				{
					if (moveCount <= 6) Achieve(@"496804");
					if (moveCount <= 5) Achieve(@"496824");
					if (moveCount <= 4) Achieve(@"496834");
					break;
				}
					
				case 4:
				{
					if (moveCount <= 20) Achieve(@"497554");
					if (moveCount <= 16) Achieve(@"497564");
					if (moveCount <= 10) Achieve(@"497574");
					break;
				}
			}
			
			break;
		}
			
		case PuzzleMode_Switch:
		{
			switch (size)
			{
				case 3:
				{
					if (moveCount <= 100) Achieve(@"496754");
					if (moveCount <= 70) Achieve(@"496764");
					if (moveCount <= 60) Achieve(@"496774");
					if (moveCount <= 50) Achieve(@"496784");
					if (moveCount <= 40) Achieve(@"496794");
					break;
				}
					
				case 4:
				{
					if (moveCount <= 200) Achieve(@"497584");
					if (moveCount <= 80) Achieve(@"497594");
					if (moveCount <= 40) Achieve(@"497604");
					break;
				}
			}
			
			break;
		}
	}
}

+(void)achievePlayTime:(int)seconds
{
	if (seconds >= 60 * 10) Achieve(@"496844");
	if (seconds >= 60 * 60) Achieve(@"496854");
}

+(void)achievePlayCount:(int)playCount
{
	if (playCount >= 5) Achieve(@"496864");
	if (playCount >= 20) Achieve(@"496874");
	if (playCount >= 100) Achieve(@"496884");
}

@end

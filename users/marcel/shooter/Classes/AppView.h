#import <GameKit/GameKit.h>
#import <UIKit/UIKit.h>

void SetInt(NSString* name, int value);
int GetInt(NSString* name);

@interface AppView : UIView <GKLeaderboardViewControllerDelegate>
{
	class OpenGLState* openGlState;
	class Main* main;
	
	NSTimer* updateTimer;
	
	UILabel* scoreLabel;
	UILabel* comboLabel;
	UILabel* comboLabel2;
	UILabel* gameOverLabel;
	UILabel* touchToRestart;
	UIButton* restartButton;
	UIButton* restartButton2;
	UIButton* feintButton;
	UIButton* gcButton;
	UIButton* helpButton;
	
	double playTime;
	time_t playTime1;
	time_t playTime2;
}

-(void)handleRestart:(id)sender;
-(void)handleGameCenterLogin:(id)sender;
-(void)handleGameCenterShowLeaderBoards:(id)sender;
-(void)handleFeint:(id)sender;
-(void)handleHelp:(id)sender;

-(void)checkAchievement_Combo:(int)countA;
-(void)checkAchievement_Die:(int)count;
-(void)checkAchievement_PlayCount:(int)count;
-(void)checkAchievement_PlayTime:(int)time;

-(void)menuShow:(bool)show;

-(void)pause;
-(void)resume;

@end

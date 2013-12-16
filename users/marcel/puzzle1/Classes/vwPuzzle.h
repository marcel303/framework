#import <iAd/iAd.h>
#import <UIKit/UIKit.h>
#import "AdWhirlDelegateProtocol.h"
#import "AdWhirlView.h"
#import "MovesView.h"
#import "puzzle.h"
#import "PuzzleView.h"
#import "TimeView.h"

enum GameMode
{
	GameMode_Undefined,
	GameMode_Shuffle,
	GameMode_Play,
	GameMode_Finished
};

@interface vwPuzzle : UIViewController <PuzzleViewDelegate, ADBannerViewDelegate, UIAlertViewDelegate, AdWhirlDelegate>
{
	UIImage* image;
	Puzzle* puzzle;
	PuzzleView* puzzleView;
	int size;
	GameMode mode;
	int shuffleCount;
	NSTimer* timeTimer;
	int time;
	int moveCount;
	ADBannerView* banner;
	UIAlertView* winAlert;
	AdWhirlView* adwhirlView;
}

@property (nonatomic, assign) Puzzle* puzzle;
@property (nonatomic, assign) IBOutlet PuzzleView* puzzleView;
@property (nonatomic, retain) IBOutlet ADBannerView* banner;
@property (nonatomic, retain) IBOutlet AdWhirlView* adwhirlView;

-(id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil mode:(PuzzleMode)mode image:(UIImage*)image size:(int)size;
-(void)updateUi;
-(IBAction)handleBack:(id)sender;
-(IBAction)handleShuffle:(id)sender;
-(IBAction)handleShowSolution:(id)sender;
-(void)shuffle:(int)count;
-(void)performShuffle;
-(void)updatePlayCount;

// Time tracking
-(void)timeBegin;
-(void)timeEnd;
-(void)timeUpdate;

// PuzzleViewDelegate
-(void)pieceDidFinishedMove:(PuzzlePieceView*)piece;
-(bool)playIsActive;

// ADBannerViewDelegate
-(void)bannerViewDidLoadAd:(ADBannerView *)banner;
-(void)bannerView:(ADBannerView *)banner didFailToReceiveAdWithError:(NSError *)error;

// UIAlertViewDelegate
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

// AdWhirlDelegate
-(NSString*)adWhirlApplicationKey;
-(UIViewController*)viewControllerForPresentingModalView;

-(void)dbg_iAdToggle;

@end

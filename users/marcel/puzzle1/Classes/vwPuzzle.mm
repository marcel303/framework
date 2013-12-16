#import "AppDelegate.h"
#import "vwPuzzle.h"
#import "PuzzlePieceView.h"
#import "SolutionView.h"
#import "OpenFeintHelper.h"
#import "State.h"

@implementation vwPuzzle

@synthesize puzzle;
@synthesize puzzleView;
@synthesize banner;
@synthesize adwhirlView;

-(id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil mode:(PuzzleMode)_mode image:(UIImage*)_image size:(int)_size 
{
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) 
	{
		image = _image;
		puzzle = new Puzzle();
		puzzle->Size_set(_size, _size, _mode);
		size = _size;
		mode = GameMode_Undefined;
		winAlert = [[UIAlertView alloc] initWithTitle:@"You win" message:@"You won the game!" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:@"Try Again", nil];
		
#ifdef DEBUG
		[NSTimer scheduledTimerWithTimeInterval:5.0f target:self selector:@selector(dbg_iAdToggle) userInfo:nil repeats:YES];
#endif
    }
	
    return self;
}

-(void)viewDidLoad 
{
	[puzzleView setup:image puzzle:puzzle delegate:self controller:self];
//	[adwhirlView requestFreshAd];
	
#ifndef DEBUG || 0
	self.adwhirlView = [AdWhirlView requestAdWhirlViewWithDelegate:self];
	self.adwhirlView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin;
#endif
	
    [super viewDidLoad];
}

-(void)viewDidAppear:(BOOL)animated
{
	[self handleShuffle:nil];
}

-(void)updateUi
{
	[puzzleView updateUi];
}

-(void)handleBack:(id)sender
{
	[self dismissModalViewControllerAnimated:YES];
}

-(IBAction)handleShuffle:(id)sender
{
	if (mode == GameMode_Shuffle)
		return;
	
#ifdef DEBUG
	[self shuffle:30];
#else
	[self shuffle:70];
#endif
	
	[self timeEnd];
	[puzzleView updateUi_Time:0];
	[puzzleView updateUi_Move:0];
}

-(IBAction)handleShowSolution:(id)sender
{
	CGRect r = self.view.bounds;
	[self.view addSubview:[[[SolutionView alloc] initWithFrame:r image:image] autorelease]];
}

//

-(void)shuffle:(int)count
{
	Assert(mode != GameMode_Shuffle);
	
	mode = GameMode_Shuffle;
	
	shuffleCount = count;
	
	if (shuffleCount > 0)
	{
		[self performShuffle];
	}
}

-(void)performShuffle
{
	Assert(mode == GameMode_Shuffle);
	
	Vec2I prev = puzzle->mFreeLocation;
	Vec2I next = puzzle->ShuffleTarget_get();
	
	puzzle->Move(next);
	
	PuzzlePieceView* piece = [puzzleView getPieceView:prev];
	
	[piece animationBegin:true];
	
	[puzzleView updateUi];
	
	shuffleCount--;
}

-(void)updatePlayCount
{
	int playCount = GetInt(@"play_count");
	playCount++;
	[OpenFeintHelper achievePlayCount:playCount];
	SetInt(@"play_count", playCount);
	
	LOG_DBG("playCount: update: %d", playCount);
}

// Time tracking

-(void)timeBegin
{
	[self timeEnd];
	
	Assert(timeTimer == nil);
	
	time = 0;
	timeTimer = [NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(timeUpdate) userInfo:nil repeats:YES];
}

-(void)timeEnd
{
	if (timeTimer == nil)
		return;
	
	[timeTimer invalidate];
	timeTimer = nil;
}

-(void)timeUpdate
{
	time++;
	
	[puzzleView updateUi_Time:time];
}

// --------------------
// PuzzleViewDelegate
// --------------------

-(void)pieceDidFinishedMove:(PuzzlePieceView*)piece
{
	if (mode == GameMode_Shuffle)
	{
		if (shuffleCount > 0)
		{
			[self performShuffle];
		}
		else
		{
			mode = GameMode_Play;
			
			[self timeBegin];
			
			moveCount = 0;
		}
	}
	else if (mode == GameMode_Play)
	{
		if (piece.piece->mLocation[0] != piece.piece->mLocationPrev[0] || piece.piece->mLocation[1] != piece.piece->mLocationPrev[1])
		{
			moveCount++;
			
			[puzzleView updateUi_Move:moveCount];
			
			bool win = puzzle->CheckWin();
			
			LOG_DBG("win: %d", win?1:0);
			
			if (win)
			{
				mode = GameMode_Finished;
				
				[self timeEnd];
				
				[self updatePlayCount];
				
				[AppDelegate playSound:@"win"];
				
				[winAlert show];
				
				[OpenFeintHelper rankMoves:puzzle->Mode_get() size:size moveCount:moveCount];
				[OpenFeintHelper rankTime:puzzle->Mode_get() size:size time:time];
				[OpenFeintHelper achieveMoves:puzzle->Mode_get() size:size moveCount:moveCount];
				[OpenFeintHelper achieveTime:puzzle->Mode_get() size:size time:time];
			}
		}
	}
}

-(bool)playIsActive
{
	return mode == GameMode_Play;
}

// --------------------
// ADBannerViewDelegate
// --------------------

-(void)bannerViewDidLoadAd:(ADBannerView *)_banner
{
	LOG_INF("bannerViewDidLoadAd", 0);
	
	if (_banner.hidden)
		[_banner setAlpha:0.0f];
	
	[UIView beginAnimations:@"iAd" context:nil];
	
	[_banner setHidden:NO];
	[_banner setAlpha:1.0f];
	
	[puzzleView setFrame:CGRectMake(puzzleView.frame.origin.x, 96.0f, puzzleView.frame.size.width, puzzleView.frame.size.height)];
	
	[UIView commitAnimations];
}
 
-(void)bannerView:(ADBannerView *)_banner didFailToReceiveAdWithError:(NSError *)error
{
	LOG_INF("bannerViewDidFailToReceiveAdWithError", 0);
	NSLog(@"error: %@", error);
	
	[UIView beginAnimations:@"iAd" context:nil];
	
	[_banner setHidden:YES];
	
	[puzzleView setFrame:CGRectMake(puzzleView.frame.origin.x, 74.0f, puzzleView.frame.size.width, puzzleView.frame.size.height)];
	
	[UIView commitAnimations];
}

-(BOOL)bannerViewActionShouldBegin:(ADBannerView *)banner willLeaveApplication:(BOOL)willLeave
{
	LOG_INF("bannerViewActionShouldBegin", 0);
	
//	if (willLeave)
//		return NO;
	
	return YES;
}

// -------------------
// UIAlertViewDelegate
// -------------------

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	if (alertView == winAlert)
	{
		if (buttonIndex == 0)
		{
			[self dismissModalViewControllerAnimated:YES];
		}
		else
		{
			[self handleShuffle:nil];
		}
	}
}

// ---------------
// AdWhirlDelegate
// ---------------

-(NSString*)adWhirlApplicationKey
{
	LOG_DBG("adWhirlApplicationKey", 0);
	
	return @"90f2c98315a142a7b237a376b5e3be7c";
}

-(UIViewController*)viewControllerForPresentingModalView
{
	LOG_DBG("viewControllerForPresentingModalView", 0);
	
//	return [AppDelegate mainApp].rootController;
	return self;
}

-(void)adWhirlDidReceiveAd:(AdWhirlView *)adWhirlView
{
	LOG_DBG("adWhirlDidReceiveAd", 0);
	
	[self.adwhirlView removeFromSuperview];
	CGRect rect = self.adwhirlView.bounds;
	rect.origin.y = 44;
	[self.adwhirlView setFrame:rect];
	[self.adwhirlView setAlpha:0.0f];
	
	[UIView beginAnimations:@"adWhirl" context:nil];
	
	[self.view addSubview:self.adwhirlView];
	[self.adwhirlView setAlpha:1.0f];
//	[self.view sendSubviewToBack:self.adwhirlView];
	
	[puzzleView setFrame:CGRectMake(puzzleView.frame.origin.x, 96.0f, puzzleView.frame.size.width, puzzleView.frame.size.height)];
	
	[UIView commitAnimations];
}

-(void)adWhirlDidFailToReceiveAd:(AdWhirlView *)adWhirlView usingBackup:(BOOL)yesOrNo
{
	LOG_DBG("adWhirlDidFailToReceiveAd", 0);
	
	[UIView beginAnimations:@"adWhirl" context:nil];
	
	[adWhirlView removeFromSuperview];
	
	[puzzleView setFrame:CGRectMake(puzzleView.frame.origin.x, 74.0f, puzzleView.frame.size.width, puzzleView.frame.size.height)];
	
	[UIView commitAnimations];
}

-(void)adWhirlWillPresentFullScreenModal
{
	LOG_DBG("adWhirlWillPresentFullScreenModal", 0);
}

-(void)adWhirlDidDismissFullScreenModal
{
	LOG_DBG("adWhirlDidDismissFullScreenModal", 0);
}

-(BOOL)adWhirlTestMode
{
	LOG_DBG("adWhirlTestMode", 0);
	
#ifdef DEBUG
	return YES;
#else
	return NO;
#endif
}

-(NSString *)admobPublisherID
{
	LOG_DBG("admobPublisherID", 0);
	
	return @"a14c703890a92ac";
}

//

-(void)dbg_iAdToggle
{
#if 0
	if (rand() % 2)
		[self bannerViewDidLoadAd:banner];
	else
		[self bannerView:banner didFailToReceiveAdWithError:[NSError errorWithDomain:@"domain" code:0 userInfo:nil]];
#endif
}

-(void)viewWillAppear:(BOOL)animated
{
//	[AppDelegate playSound:@"open"];
}

-(void)didReceiveMemoryWarning 
{
    [super didReceiveMemoryWarning];
}

-(void)viewDidUnload 
{
    [super viewDidUnload];
}

-(void)dealloc 
{
	LOG_DBG("dealloc: vwPuzzle", 0);
	
	[puzzleView killTimers];
	
	[self timeEnd];
	
	Assert(timeTimer == nil);
	
	mode = GameMode_Undefined;
	
	delete puzzle;
	puzzle = 0;
	
	puzzleView = nil;
	
	self.banner.delegate = nil;
	self.banner = nil;
	
	self.adwhirlView.delegate = nil;
	self.adwhirlView = nil;
	
	[winAlert release];
	
    [super dealloc];
}

@end

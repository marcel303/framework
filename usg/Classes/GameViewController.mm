#import "GameViewController.h"

@implementation GameViewController

@synthesize gameView;

-(id)initWithGameView:(GameView*)_gameView
{
	if ((self = [super init]))
	{
		self.wantsFullScreenLayout = YES;
		
		self.gameView = _gameView;
	}
	
	return self;
}

-(void) loadView
{
	[gameView initForReal];
	
	self.view = gameView;
}

-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
//	return orientation == UIInterfaceOrientationLandscapeRight;
	return false;
}

-(void)didReceiveMemoryWarning
{
	// note: super didReceiveMemoryWarning releases view..
//	[super didReceiveMemoryWarning];

	NSLog(@"received memory warning");
	
	[gameView handleMemoryWarning];
}

-(void)dealloc
{
	[gameView release];
	gameView = nil;
	
	[super dealloc];
}

@end

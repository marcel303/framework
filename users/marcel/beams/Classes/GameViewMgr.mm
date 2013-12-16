#import "AppDelegate.h"
#import "board.h"
#import "Debugging.h"
#import "game.h"
#import "GameViewMgr.h"
#import "Log.h"

@implementation GameViewMgr

@synthesize adController;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil app:(AppDelegate*)_app
{
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) 
	{
    }
	
    return self;
}

- (void)viewDidLoad 
{
	Assert(adController != nil);
	
//	adController.currentViewController = app.rootController;
	
	gGame->Begin();
	
    [super viewDidLoad];
}

-(IBAction)handleShuffle:(id)sender
{
	LOG_DBG("handleShuffle", 0);
	
	{
		const int x = rand() % gGame->Board_get()->Sx_get();
		const int y = rand() % gGame->Board_get()->Sy_get();
		
		gGame->Board_get()->Tile_get(x, y)->RotateX(+1, 0.0f);
	}
	
	{
		const int x = rand() % gGame->Board_get()->Sx_get();
		const int y = rand() % gGame->Board_get()->Sy_get();
		
		gGame->Board_get()->Tile_get(x, y)->RotateY(-1, 0.0f);
	}
}

-(void)performShuffle
{
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}


@end

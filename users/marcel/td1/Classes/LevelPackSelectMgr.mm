#import "AppDelegate.h"
#import "level_pack.h"
#import "LevelPackSelectMgr.h"
#import "LevelSelectMgr.h"
#import "Log.h"

@implementation LevelPackSelectMgr

@synthesize scrollView;

/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }
    return self;
}
*/

-(void)viewDidLoad 
{
	// scan for level packs
	packList = [td1AppDelegate scanLevelPacks];
	
	// add button for each level pack
	float y = 0.0f;
	float sy = 50.0f;
	for (size_t i = 0; i < packList.size(); ++i)
	{
		LevelPackDescription& pack = packList[i];
		
		LOG_DBG("adding '%s'", pack.name.c_str());
		
		UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:[NSString stringWithCString:pack.name.c_str() encoding:NSASCIIStringEncoding] forState:UIControlStateNormal];
		[button addTarget:self action:@selector(handleLevelSelect:) forControlEvents:UIControlEventTouchUpInside];
		[button setTag:i];
		[button setFrame:CGRectMake(0.0f, y, scrollView.bounds.size.width, sy)];
		[scrollView addSubview:button];
		 y += sy;
	}
	
	// todo: add buy button
    [super viewDidLoad];
}

-(IBAction)handleBack:(id)sender
{
	[self dismissModalViewControllerAnimated:YES];
}

-(void)handleLevelSelect:(id)sender
{
	UIButton* button = sender;
	int index = button.tag;
	LevelPackDescription desc = packList[index];
	NSString* path = [NSString stringWithCString:desc.path.c_str() encoding:NSASCIIStringEncoding];
	
	LevelSelectMgr* mgr = [[[LevelSelectMgr alloc] initWithNibName:@"LevelSelectMgr" bundle:nil pack:path] autorelease];
	[self presentModalViewController:mgr animated:YES];
}

-(void)viewDidUnload
{
	self.scrollView = nil;
}

- (void)dealloc {
    [super dealloc];
}


@end

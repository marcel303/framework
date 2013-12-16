#import <vector>
#import "AppDelegate.h"
#import "AppViewMgr.h"
#import "DirectoryScanner.h"
#import "LevelSelectMgr.h"
#import "Log.h"
#import "StringEx.h"

@implementation LevelSelectMgr

@synthesize scrollView;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil pack:(NSString*)_path
{
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
		path = _path;
    }
    return self;
}

-(void)viewDidLoad 
{
	// todo: scan path for level files
	levelList = [td1AppDelegate scanLevels:path];
	
	// todo: add button for each level
	float y = 0.0f;
	float sy = 50.0f;
	for (size_t i = 0; i < levelList.size(); ++i)
	{
		LevelDescription& level = levelList[i];
		
		LOG_DBG("adding '%s'", level.name.c_str());
		
		UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
		[button setTitle:[NSString stringWithCString:level.name.c_str() encoding:NSASCIIStringEncoding] forState:UIControlStateNormal];
		[button addTarget:self action:@selector(handleLevelSelect:) forControlEvents:UIControlEventTouchUpInside];
		[button setTag:i];
		[button setFrame:CGRectMake(0.0f, y, scrollView.bounds.size.width, sy)];
		[scrollView addSubview:button];
		 y += sy;
	}
	
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
	LevelDescription desc = levelList[index];
	NSString* _path = [NSString stringWithCString:desc.path.c_str() encoding:NSASCIIStringEncoding];
	
	AppViewMgr* mgr = [[[AppViewMgr alloc] initWithNibName:@"AppViewMgr" bundle:nil level:_path] autorelease];
	[self presentModalViewController:mgr animated:YES];
}

- (void)viewDidUnload {
    [super viewDidUnload];
	self.scrollView = nil;
}


- (void)dealloc {
    [super dealloc];
}


@end

#import <UIKit/UIKit.h>
#import "AdViewController.h"

@class AppDelegate;

@interface GameViewMgr : UIViewController 
{
	AppDelegate* app;
	AdViewController* adController;
}

@property (nonatomic, retain) IBOutlet AdViewController* adController;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil app:(AppDelegate*)app;

-(IBAction)handleShuffle:(id)sender;
-(void)performShuffle;

@end

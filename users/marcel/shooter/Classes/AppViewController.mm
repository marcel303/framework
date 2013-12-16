#import <QuartzCore/CALayer.h>
#import "AppView.h"
#import "AppViewController.h"

@implementation AppViewController

-(id)initWithNibName:(NSString*)nibNameOrNil bundle:(NSBundle*)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	
    if (self)
	{
    }
	
    return self;
}

-(void)dealloc
{
    [super dealloc];
}

-(void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

-(void)viewDidUnload
{
    [super viewDidUnload];
}

-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	return NO;
    //return interfaceOrientation == UIInterfaceOrientationLandscapeRight;
}

@end

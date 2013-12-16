#import "paint_01AppDelegate.h"
#import "PaintView.h"
#import "PaintViewController.h"

@implementation paint_01AppDelegate

@synthesize window;

- (void)applicationDidFinishLaunching:(UIApplication *)application {    
	
	[application setNetworkActivityIndicatorVisible:FALSE];
	[application setStatusBarHidden:TRUE animated:FALSE];
	[application setIdleTimerDisabled:TRUE];
	
	//window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
	
	[window setBackgroundColor:[UIColor greenColor]];  

	//
	
	controller = [[PaintViewController alloc] init];
	[window addSubview:[controller view]];  
	
	//
	
	imagePicker = [[UIImagePickerController alloc] init];
	imagePicker.delegate = self;
	imagePicker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
	[window addSubview:imagePicker.view];
	
	//
	
	[window makeKeyAndVisible];  
}

- (void)imagePickerController:(UIImagePickerController*)picker didFinishPickingImage:(UIImage*)image editingInfo:(NSDictionary*)editingInfo
{
	[picker dismissModalViewControllerAnimated:YES];
	picker.view.hidden = YES;
	[controller->paintView loadImage:image];
}

- (void)dealloc {
	[imagePicker release];
	[controller release];
	[window release];
	[super dealloc];
}

@end

    //
//  AppViewMgr.mm
//  td1
//
//  Created by Marcel Smit on 12-08-10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "AppViewMgr.h"

@implementation AppViewMgr

@synthesize livesLabel;
@synthesize scoreLabel;
@synthesize moneyLabel;

/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}
*/

-(IBAction)handleTowerSelectA:(id)sender
{
}

-(IBAction)handleTowerSelectB:(id)sender
{
}

-(IBAction)handleTowerSelectC:(id)sender
{
}

-(IBAction)handleTowerSelectD:(id)sender
{
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

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
	self.livesLabel = nil;
	self.scoreLabel = nil;
	self.moneyLabel = nil;
    [super dealloc];
}


@end

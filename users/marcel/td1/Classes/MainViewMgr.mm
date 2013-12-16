//
//  MainViewMgr.mm
//  td1
//
//  Created by Marcel Smit on 12-08-10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "MainViewMgr.h"
#import "LevelPackSelectMgr.h"

@implementation MainViewMgr

/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }
    return self;
}
*/

-(IBAction)handlePlay:(id)sender
{
	LevelPackSelectMgr* mgr = [[[LevelPackSelectMgr alloc] initWithNibName:@"LevelPackSelectMgr" bundle:nil] autorelease];
	[self presentModalViewController:mgr animated:YES];
}

-(IBAction)handleAbout:(id)sender
{
	[[[[UIAlertView alloc] initWithTitle:@"About" message:@"about.." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
}

- (void)dealloc {
    [super dealloc];
}


@end

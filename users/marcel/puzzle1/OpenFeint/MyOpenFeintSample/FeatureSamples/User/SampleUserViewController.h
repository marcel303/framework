//
//  SampleUserViewController.h
//  OpenFeint
//
//  Created by Danilo Campos on 6/17/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "OFUser.h"
#import "OFCurrentUser.h"

@interface SampleUserViewController : UITableViewController <OFUserDelegate, OFCurrentUserDelegate> {
	
	OFUser *selectedUser;
	UIImage *selectedUserImage;
	NSArray *friends;
	NSArray *friendsWithThisApp;
	
	UIButton *friendButton;
	
}

@property (nonatomic, retain) OFUser *selectedUser;
@property (nonatomic, retain) UIImage *selectedUserImage;
@property (nonatomic, retain) NSArray *friends;
@property (nonatomic, retain) NSArray *friendsWithThisApp;
@property (nonatomic, retain) UIButton *friendButton;

@end

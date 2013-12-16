//
//  LevelSelectMgr.h
//  td1
//
//  Created by Marcel Smit on 12-08-10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "level_pack.h"

@interface LevelSelectMgr : UIViewController {
	NSString* path;
	std::vector<LevelDescription> levelList;
	UIScrollView* scrollView;
}

@property (nonatomic, retain) IBOutlet UIScrollView* scrollView;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil pack:(NSString*)path;

-(IBAction)handleBack:(id)sender;
-(void)handleLevelSelect:(id)sender;

@end

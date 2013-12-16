//
//  SampleAnnouncementController.h
//  OpenFeint
//
//  Created by Joe on 11/13/09.
//  Copyright 2009 Aurora Feint, Inc.  All rights reserved.
//

#import <UIKit/UIKit.h>
#import "OFCallbackable.h"
#import "OFAnnouncement.h"


enum AnnouncementType {
	AnnouncementType_App = 0,
	AnnouncementType_Dev,
	//
	AnnouncementType_None = -1
};


@interface SampleAnnouncementController : UIViewController<OFCallbackable, OFAnnouncementDelegate>
{
	IBOutlet	UILabel			*statusTextBox;
	IBOutlet	UILabel			*announcementIndexText;
	IBOutlet	UILabel			*announcementCountText;
	IBOutlet	UILabel			*announcementText;
	IBOutlet	UILabel			*postIndexText;
	IBOutlet	UILabel			*postCountText;
	IBOutlet	UILabel			*postText;
	//
	AnnouncementType	currentAnnouncementType;
	//
	NSArray		*appAnnouncementList;
	NSArray		*devAnnouncementList;
	NSArray		*curAnnouncementList;
	NSArray		*postList;
	//
	NSInteger	announcementIndex;
	NSInteger	postIndex;
}

@property (retain, nonatomic) UILabel	*statusTextBox;
@property (retain, nonatomic) UILabel	*announcementIndexText;
@property (retain, nonatomic) UILabel	*announcementCountText;
@property (retain, nonatomic) UILabel	*announcementText;
@property (retain, nonatomic) UILabel	*postIndexText;
@property (retain, nonatomic) UILabel	*postCountText;
@property (retain, nonatomic) UILabel	*postText;

- (IBAction) onRadioBtnAnnouncementType:(id)sender;
- (IBAction) onBtnAnnouncementIndexUp:(id) sender;
- (IBAction) onBtnAnnouncementIndexDown:(id) sender;
- (IBAction) onBtnPostIndexUp:(id) sender;
- (IBAction) onBtnPostIndexDown:(id) sender;

@end

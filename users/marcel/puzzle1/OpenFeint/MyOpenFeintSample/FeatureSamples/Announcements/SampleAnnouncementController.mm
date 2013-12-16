//
//  SampleAnnouncementController.m
//  OpenFeint
//
//  Created by Joe on 11/13/09.
//  Copyright 2009 Aurora Feint, Inc.  All rights reserved.
//

#import "SampleAnnouncementController.h"
#import "OpenFeint.h"
#import "OFForumPost.h"


@interface SampleAnnouncementController ()
- (void)setAnnouncementType: (AnnouncementType) requestedType;
- (void)announcementIndexChange: (NSInteger) delta;
- (void)postIndexChange: (NSInteger) delta;
- (void)refreshStatusText: (OFAnnouncement*) announcement;
@end


@implementation SampleAnnouncementController

@synthesize statusTextBox;
@synthesize announcementIndexText;
@synthesize announcementCountText;
@synthesize announcementText;
@synthesize postIndexText;
@synthesize postCountText;
@synthesize postText;


- (void)viewWillDisappear:(BOOL)animated
{
	[OFAnnouncement setDelegate:nil];
	[super viewWillDisappear:animated];
}


- (void)viewDidAppear:(BOOL)animated
{
	[OFAnnouncement setDelegate:self];
	//OFSafeRelease(postList);
	[OFAnnouncement downloadAnnouncements];
	
	[super viewDidAppear:animated];
}


- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}


- (void)viewDidLoad {
	[super viewDidLoad];
}


- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
	NSLog(@"viewDidUnload");
}


///////////////////////////////////////////////////////////////////
// traverse announcements and posts
///////////////////////////////////////////////////////////////////

- (void)setAnnouncementType: (AnnouncementType) requestedType {
	currentAnnouncementType = requestedType;
	
	switch (currentAnnouncementType) {
		case AnnouncementType_App:{
			curAnnouncementList = appAnnouncementList;
		}	break;
		case AnnouncementType_Dev:{
			curAnnouncementList = devAnnouncementList;
		}	break;
		default:
			curAnnouncementList = nil;
			break;
	}
	
	announcementIndex = 0;
	[self announcementIndexChange:0];	
}


- (void)announcementIndexChange: (NSInteger) delta {
	announcementIndex += delta;
	if (announcementIndex < 0){
		announcementIndex = 0;
	}
	postIndex = 0;

	[self postIndexChange:0];
}


- (void)postIndexChange: (NSInteger) delta {
	NSInteger announcementCount = [curAnnouncementList count];
	
	// Set some display values using defaults where not yet known.
	announcementCountText.text = [NSString stringWithFormat: @"%d", announcementCount];
	announcementIndexText.text =  @"0";
	announcementText.text = @"";
	postCountText.text = @"0";
	postIndexText.text = @"0";
	postText.text = @"";
	
	if (announcementCount > 0){
		if ((announcementIndex < 0) || (announcementCount <= announcementIndex)){
			announcementIndex = 0;
		}
		announcementIndexText.text = [NSString stringWithFormat: @"%d", announcementIndex + 1];
		
		postIndex += delta;
		if (postIndex < 0){
			postIndex = 0;
		}
		
		OFAnnouncement *curAnnouncement = (OFAnnouncement*)
			[curAnnouncementList objectAtIndex:announcementIndex];
		[curAnnouncement retain];
		
		announcementText.text = [NSString stringWithString:curAnnouncement.body];
		[curAnnouncement getPosts];
		[self refreshStatusText: curAnnouncement];
		[curAnnouncement release];
	}else{
		announcementIndex = 0;
		postIndex = 0;
		statusTextBox.text = @"";
	}
}


- (void)refreshStatusText: (OFAnnouncement*) announcement {
	NSString *importanceText = @"";
	
	if(announcement.isImportant){
		importanceText = @"important and ";
	}
	
	if(announcement.isUnread){
		statusTextBox.text =[NSString stringWithFormat: @"%@unread", importanceText];
	}else{
		statusTextBox.text =[NSString stringWithFormat: @"%@has been read", importanceText];
	}
}


///////////////////////////////////////////////////////////////////
// buttons
///////////////////////////////////////////////////////////////////

- (IBAction) onRadioBtnAnnouncementType:(id)sender{
	UISegmentedControl* control = (UISegmentedControl*)sender;
	
	[self setAnnouncementType: (AnnouncementType) control.selectedSegmentIndex];
}


- (IBAction) onBtnAnnouncementIndexUp:(id) sender{
	[self announcementIndexChange: 1];
}


- (IBAction) onBtnAnnouncementIndexDown:(id) sender{
	[self announcementIndexChange: -1];
}


- (IBAction) onBtnPostIndexUp:(id) sender{
	[self postIndexChange: 1];
}


- (IBAction) onBtnPostIndexDown:(id) sender{
	[self postIndexChange: -1];
}


///////////////////////////////////////////////////////////////////
// OFAnnouncementDelegate protocol
///////////////////////////////////////////////////////////////////

- (void)didDownloadAnnouncementsAppAnnouncements:(NSArray*)appAnnouncements devAnnouncements:(NSArray*)devAnnouncements;
{
	OFSafeRelease(appAnnouncementList);
	OFSafeRelease(devAnnouncementList);
	
	appAnnouncementList = [appAnnouncements retain];
	devAnnouncementList = [devAnnouncements retain];
	
	[self setAnnouncementType: AnnouncementType_App];
}

- (void)didFailDownloadAnnouncements
{
	NSLog(@"Failed to download Announcements");
}

- (void)didGetPosts:(NSArray*)posts OFAnnouncement:(OFAnnouncement*)announcement {
	NSInteger postCount = [posts count];
	
	if (postCount > 0) {
		if ((postIndex < 0) || (postCount <= postIndex)){
			postIndex = 0;
		}
		OFForumPost *curPost = (OFForumPost*)
			[posts objectAtIndex:postIndex];
		postText.text = [NSString stringWithString:curPost.body];
		postIndexText.text = [NSString stringWithFormat: @"%d", postIndex + 1];
	}else{
		postText.text = @"";
	}

	postCountText.text = [NSString stringWithFormat: @"%d", postCount];
}


- (void)didFailGetPostsOFAnnouncement:(OFAnnouncement*)announcement {
	postText.text = @"";
}


///////////////////////////////////////////////////////////////////
// init & dealloc
///////////////////////////////////////////////////////////////////

- (id) init{
	if (![super init]){
		return nil;
	}
	
	announcementIndex = 0;
	postIndex = 0;
	
	return self;
}


- (void)dealloc {
	OFSafeRelease(statusTextBox);
	OFSafeRelease(announcementIndexText);
	OFSafeRelease(announcementCountText);
	OFSafeRelease(announcementText);
	OFSafeRelease(postIndexText);
	OFSafeRelease(postCountText);
	OFSafeRelease(postText);
	OFSafeRelease(appAnnouncementList);
	OFSafeRelease(devAnnouncementList);
	OFSafeRelease(postList);
    [super dealloc];
}


///////////////////////////////////////////////////////////////////
// OFCallbackable protocol
///////////////////////////////////////////////////////////////////

- (bool)canReceiveCallbacksNow
{
	return YES;
}


///////////////////////////////////////////////////////////////////
// Screen orientation
///////////////////////////////////////////////////////////////////

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	const unsigned int numOrientations = 4;
	UIInterfaceOrientation myOrientations[numOrientations] = { UIInterfaceOrientationPortrait, UIInterfaceOrientationLandscapeLeft, UIInterfaceOrientationLandscapeRight, UIInterfaceOrientationPortraitUpsideDown };
	return [OpenFeint shouldAutorotateToInterfaceOrientation:interfaceOrientation withSupportedOrientations:myOrientations andCount:numOrientations];
}


- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	[OpenFeint setDashboardOrientation:self.interfaceOrientation];
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}


@end

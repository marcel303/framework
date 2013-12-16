//
//  SampleInviteController.m
//  OpenFeint
//
//  Created by Ron on 6/25/10.
//  Copyright 2010 Aurora Feint, Inc.  All rights reserved.
//

#import "SampleInviteController.h"
#import "OFViewHelper.h"
#import "OpenFeint.h"


@implementation SampleInviteController

@synthesize definitionLabel, alternateDefinitionText, loadedDefinition;
@synthesize defSenderParam, defReceiverParam, defDevMessage, defReceiverNotification, defSenderNotification, defSenderIncentive, defIcon;


- (void)viewWillDisappear:(BOOL)animated
{
	[OFInvite setDelegate:nil];
    [OFInviteDefinition setDelegate:nil];
	[super viewWillDisappear:animated];
}

- (void)viewWillAppear:(BOOL)animated
{
	UIScrollView* scrollView = OFViewHelper::findFirstScrollView(self.view);
	scrollView.contentSize = OFViewHelper::sizeThatFitsTight(scrollView);
	
	[super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{	
	[OFInvite setDelegate:self];
    [OFInviteDefinition setDelegate:self];
    [OFInviteDefinition getPrimaryInviteDefinition];
	[super viewDidAppear:animated];
}




- (void)viewDidLoad {
	[super viewDidLoad];
}


- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}

-(void) setLoadedDefinition:(OFInviteDefinition *)_definition {
    if(loadedDefinition) [loadedDefinition release];
    loadedDefinition = [_definition retain];
    
    //if it's nil, these all default properly
    self.defSenderParam.text = loadedDefinition.senderParameter;
    self.defReceiverParam.text = loadedDefinition.receiverParameter;
    self.defDevMessage.text = loadedDefinition.developerMessage;
    self.defReceiverNotification.text = loadedDefinition.receiverNotification;
    self.defSenderNotification.text = loadedDefinition.senderNotification;
    self.defSenderIncentive.text = loadedDefinition.senderIncentiveText;
    [loadedDefinition getInviteIcon];
    
}

///////////////////////////////////////////////////////////////////
// Button actions
///////////////////////////////////////////////////////////////////
-(IBAction) loadAlternate {
    if(alternateDefinitionText.text.length) {   
        self.definitionLabel.text = [NSString stringWithFormat:@"%@: loading", alternateDefinitionText.text];
        self.loadedDefinition = nil;
        [OFInviteDefinition getInviteDefinition:alternateDefinitionText.text];
    }
}

- (IBAction) onTextFieldDoneEditing:(id) sender{
	[sender resignFirstResponder];
}

- (IBAction) chooseFriend {
    [OFFriendPickerController launchPickerWithDelegate:self];
}


-(IBAction) displayInviteScreen {
    if(self.loadedDefinition) {
        OFInvite*invite = [[[OFInvite alloc]initWithInviteDefinition:self.loadedDefinition]autorelease];
        [invite displayAndSendInviteScreen];
    }    
}

///////////////////////////////////////////////////////////////////
// OFFriendPickerDelegate protocol
///////////////////////////////////////////////////////////////////
- (void)pickerFinishedWithSelectedUser:(OFUser*)selectedUser {
    if(self.loadedDefinition) {
        OFInvite*invite = [[[OFInvite alloc]initWithInviteDefinition:self.loadedDefinition]autorelease];
        [invite sendInviteToUsers:[NSArray arrayWithObject:selectedUser]];
    }
}

///////////////////////////////////////////////////////////////////
// OFInviteSendDelegate protocol
///////////////////////////////////////////////////////////////////

- (void)didSendInvite:(OFInvite*)invite{
    [[[[UIAlertView alloc] initWithTitle:@"Invitation" message:@"Invitation was sent successfully" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    
}


- (void)didFailSendInvite:(OFInvite*)invite{
    [[[[UIAlertView alloc] initWithTitle:@"Invitation" message:@"Invitation failed." delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    
}


- (void)didGetInviteIcon:(UIImage*)image OFInvite:(OFInvite*)invite{
}


- (void)didFailGetInviteIconOFInvite:(OFInvite*)invite{
}


///////////////////////////////////////////////////////////////////
// OFInviteDefinitionDelegate protocol
///////////////////////////////////////////////////////////////////

- (void)didGetPrimaryInviteDefinition:(OFInviteDefinition*)definition{
    self.definitionLabel.text = @"Primary Definition: Loaded";
    self.loadedDefinition = definition;
}


- (void)didFailGetPrimaryInviteDefinition{
    self.definitionLabel.text = @"Primary Definition: Failed!";
    self.loadedDefinition = nil;
}


- (void)didGetInviteDefinition:(OFInviteDefinition*)definition{
    self.definitionLabel.text = [NSString stringWithFormat:@"%@: loaded", definition.inviteIdentifier];
    self.loadedDefinition = definition;
}


- (void)didFailGetInviteDefinition{
    [[[[UIAlertView alloc] initWithTitle:@"Cannot load invite definition" message:@"Does it exist on the developer dashboard?" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    
    self.definitionLabel.text = @"Primary Definition: Loading";    
    [OFInviteDefinition getPrimaryInviteDefinition];
    
}


- (void)didGetInviteIcon:(UIImage*)image OFInviteDefinition:(OFInviteDefinition*)inviteDef{
    self.defIcon.image = image;
}


- (void)didFailGetInviteIconOFInviteDefinition:(OFInviteDefinition*)inviteDef{
    [[[[UIAlertView alloc] initWithTitle:@"Cannot load invite image" message:@"Does it exist on the developer dashboard?" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    
}


///////////////////////////////////////////////////////////////////
// init & dealloc
///////////////////////////////////////////////////////////////////

- (id) init{
	if (![super init]){
		return nil;
	}
	
	return self;
}


- (void)dealloc {
    self.definitionLabel = nil;
    self.alternateDefinitionText = nil;
    self.loadedDefinition = nil;
    
    self.defSenderParam = nil;
    self.defReceiverParam = nil;
    self.defDevMessage = nil;
    self.defReceiverNotification = nil;
    self.defSenderNotification = nil;
    self.defSenderIncentive = nil;
    self.defIcon = nil; 
    
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

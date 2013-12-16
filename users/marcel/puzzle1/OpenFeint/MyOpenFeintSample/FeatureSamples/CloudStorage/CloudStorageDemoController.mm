//
//  CloudStorageDemoController.m
//  OpenFeint
//
//  Created by Joe on 11/13/09.
//  Copyright 2009 Aurora Feint, Inc.  All rights reserved.
//

#import "CloudStorageDemoController.h"
#import "OFCloudStorage.h"
#import "OpenFeint.h"


@implementation CloudStorageDemoController

@synthesize statusTextBox;
@synthesize blobNameTextBox;
@synthesize blobPayloadTextBox;

- (void)viewWillDisappear:(BOOL)animated
{
	[OFCloudStorage setDelegate:nil];
	[super viewWillDisappear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
	[OFCloudStorage setDelegate:self];
	[super viewDidAppear:animated];
}


- (void)didUpload{
	blobNameTextBox.text = @"";
	blobPayloadTextBox.text = @"";

	statusTextBox.text = @"sent";
}


- (void)didFailUpload:(OFCloudStorageStatus_Code)statusCode{

	// [status isAnyError] is a general test for error status.
	OFAssert(statusCode != CSC_Ok, @"Failure delegate handles errors, never handles CSC_Ok.");

	switch (statusCode) {
		case CSC_GatewayTimeout:
			statusTextBox.text = @"not reachable";
			break;
		case CSC_NotAcceptable:
			statusTextBox.text = @"invalid request";
			break;
		case CSC_InsufficientStorage:
			statusTextBox.text = @"item is too big";
			break;
		default:
			statusTextBox.text = @"failed to send";
			break;
	}
}


- (void)didDownloadData:(NSData*)blob{
	NSString		*receivedString = nil;
	NSUInteger		 blobLen	= [blob length];
	//const void	*blobBytes	= [blob bytes]; // Can enable this for peaking in debugger.
	
	if (blobLen <= 0){
		receivedString = @"";
	}else{
		receivedString = [[NSString alloc] initWithData: blob encoding: NSUTF8StringEncoding];
		[receivedString autorelease];
	}
		
	blobPayloadTextBox.text = receivedString;
	statusTextBox.text = @"fetched";
}


- (void)didFailDownloadData:(OFCloudStorageStatus_Code)statusCode
{
	switch (statusCode) {
		case CSC_GatewayTimeout:
			statusTextBox.text = @"not reachable";
			break;
		case CSC_NotAcceptable:
			statusTextBox.text = @"invalid request";
			break;
		case CSC_NotFound:
			statusTextBox.text = @"no such item";
			break;
		default:
			statusTextBox.text = @"failed to fetch";
			break;
	}
}

- (void)didDownloadKeysForCurrentUser:(NSArray*)keys
{
	for(uint i = 0; i < [keys count]; i++)
	{
		NSLog(@"%@", (NSString*)[keys objectAtIndex:i]);
	}
	
	statusTextBox.text = @"printed this user's keys to stdout";
}

- (void)didFailDownloadKeysForCurrentUser
{
	statusTextBox.text = @"failed to get keys for user";
}


- (IBAction)onBtnBlobSend:(id)sender{
	NSData		*payloadData = [blobPayloadTextBox.text dataUsingEncoding: NSUTF8StringEncoding];
	
	statusTextBox.text = @"sending";
	
	[OFCloudStorage upload:payloadData
				   withKey:blobNameTextBox.text];
}


- (IBAction)onBtnBlobFetch:(id)sender{
	statusTextBox.text = @"fetching";

	[OFCloudStorage downloadDataWithKey:blobNameTextBox.text];
}

- (IBAction)onBtnClear:(id)sender{
	
	statusTextBox.text = @"clearing";
	
	blobNameTextBox.text = @"";
	blobPayloadTextBox.text = @"";
	statusTextBox.text = @"";
}

- (IBAction) onBtnPrintAllCurrentUsersDataKeys:(id) sender
{
	statusTextBox.text = @"Getting Keys";
	
	[OFCloudStorage downloadKeysForCurrentUser];
}

- (IBAction) onTextFieldDoneEditing:(id) sender{
	statusTextBox.text = @"";
	[sender resignFirstResponder];
}


- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}


- (void)viewDidLoad {
	statusTextBox.text = @"Ready for command.";
	[super viewDidLoad];
}


- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
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
	[statusTextBox release];
	[blobNameTextBox release];
	[blobPayloadTextBox release];
    [super dealloc];
}

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

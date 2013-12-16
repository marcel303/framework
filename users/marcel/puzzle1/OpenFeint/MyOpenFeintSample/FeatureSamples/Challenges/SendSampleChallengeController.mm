#import "SendSampleChallengeController.h"

#import "OFChallengeDefinition.h"


#import "OFViewHelper.h"
#import "OFDefaultTextField.h"
#import "OpenFeint.h"
#import "OpenFeint+NSNotification.h"
#import "OFCurrentUser.h"

@implementation SendSampleChallengeController

- (void)_sendChallenge
{
    [challengeText resignFirstResponder];
    
	
	NSData* data = [NSData dataWithBytes:[challengeData.text UTF8String] length:[challengeData.text length]];
	
	OFChallenge* challenge = [[[OFChallenge alloc] initWithDefinition:selectedChallenge challengeDescription:challengeText.text challengeData:data] autorelease];

	[challenge displayAndSendChallenge];
}

- (void)setNumUnviewedChallenges:(int)numUnviewedChallenges
{
	newChallengesLabel.text = [NSString stringWithFormat:@"New Challenges: %d", numUnviewedChallenges];
}

- (void)_reloadAllData
{
	[self setNumUnviewedChallenges:[OFCurrentUser unviewedChallengesCount]];
	[challengeDefs removeAllObjects];
	
	[OFChallengeDefinition downloadAllChallengeDefinitions];
}

- (void)didDownloadAllChallengeDefinitions:(NSArray*)challengeDefinitions;
{
	[challengeDefs addObjectsFromArray:challengeDefinitions];
	
	[challengePicker reloadAllComponents];
	if ([challengeDefs count] > 0)
	{
		selectedChallenge = [(OFChallengeDefinition*)[challengeDefs objectAtIndex:0] retain];
	}
}

- (void)didFailDownloadChallengeDefinitions
{
	[[[[UIAlertView alloc] initWithTitle:@"Error" message:@"Failed downloading challenge definitions" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];
	OFSafeRelease(selectedChallenge);
	[challengeDefs removeAllObjects];
}

- (void)_unviewedChallengesChanged:(NSNotification*)notice
{
	NSUInteger unviewedChallenges = [(NSNumber*)[[notice userInfo] objectForKey:OFNSNotificationInfoUnviewedChallengeCount] unsignedIntegerValue];
	[self setNumUnviewedChallenges:unviewedChallenges];
}

- (void)awakeFromNib
{
	challengeDefs = [[NSMutableArray arrayWithCapacity:10] retain];
	
	challengeText.manageScrollViewOnFocus = YES;
	challengeText.closeKeyboardOnReturn = YES;
	challengeData.manageScrollViewOnFocus = NO;
	challengeData.closeKeyboardOnReturn = YES;
	
	[OFChallengeDefinition setDelegate:self];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_unviewedChallengesChanged:) name:OFNSNotificationUnviewedChallengeCountChanged object:nil];
	UIScrollView* scrollView = OFViewHelper::findFirstScrollView(self.view);
	scrollView.contentSize = OFViewHelper::sizeThatFitsTight(scrollView);

	[self _reloadAllData];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:OFNSNotificationUnviewedChallengeCountChanged object:nil];
	[super viewWillDisappear:animated];
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

- (void)dealloc
{
	OFSafeRelease(challengePicker);

	OFSafeRelease(challengeText);
	OFSafeRelease(challengeData);

	OFSafeRelease(challengeDefs);
	OFSafeRelease(selectedChallenge);
	OFSafeRelease(newChallengesLabel);

	[super dealloc];
}

- (IBAction)sendChallenge
{
	if (!selectedChallenge)
		return;
	
	if ([challengeText.text length] == 0)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Problem" message:@"Challenge text must not be (null)" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];
		return;
	}
	
	if ([OpenFeint hasUserApprovedFeint])
	{
		[self _sendChallenge];
	}
	else
	{
		OFDelegate nilDelegate;
		OFDelegate sendChallengeDelegate(self, @selector(_sendChallenge));
		[OpenFeint presentUserFeintApprovalModal:sendChallengeDelegate deniedDelegate:nilDelegate];
	}
}

- (bool)canReceiveCallbacksNow
{
	return YES;
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	return challengeDefs ? 1 : 0;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
	return [challengeDefs count];
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
	if ((NSInteger)[challengeDefs count] <= row)
		return @"";
		
	OFChallengeDefinition* challenge = (OFChallengeDefinition*)[challengeDefs objectAtIndex:row];
	return challenge.title;
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
	NSInteger numDefinitions = (NSInteger)[challengeDefs count];
	if (challengeDefs && numDefinitions > row && row >= 0)
	{
		selectedChallenge = [(OFChallengeDefinition*)[challengeDefs objectAtIndex:row] retain];
	}
	else
	{
		OFSafeRelease(selectedChallenge);
	}
}

@end

#import "PlayAChallengeController.h"

#import "OFChallengeToUser.h"
#import "OFChallenge.h"
#import "OFChallengeDefinition.h"
#import "OFViewHelper.h"
#import "OFDefaultTextField.h"

#import "OpenFeint.h"

#import "SampleChallengeData.h"
#import "OFTransactionalSaveFile.h"
#import "OFUser.h"

@implementation PlayAChallengeController

- (void)setChallenge:(OFChallengeToUser*)_challenge
{
	OFSafeRelease(challenge);
	challenge = [_challenge retain];
	
	challengeTextLabel.text = challenge.challenge.challengeDescription;
	userMessageLabel.text = challenge.challenge.userMessage;
}

- (void)setData:(NSData*)_data
{
	OFSafeRelease(data);
	data = [_data retain];
    challengeDataLabel.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

- (void)awakeFromNib
{
	resultDescription.manageScrollViewOnFocus = YES;
	resultDescription.closeKeyboardOnReturn = YES;
    
    [OFChallengeToUser setDelegate:self];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	UIScrollView* scrollView = OFViewHelper::findFirstScrollView(self.view);
	scrollView.contentSize = OFViewHelper::sizeThatFitsTight(scrollView);
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
	OFSafeRelease(challengeTextLabel);
	OFSafeRelease(userMessageLabel);
	OFSafeRelease(result);
	OFSafeRelease(resultDescription);

    OFSafeRelease(challengeDataLabel);
    OFSafeRelease(data);

	[super dealloc];
}

- (void)didCompleteChallenge:(OFChallengeToUser*)challengeToUser;
{
    [self.navigationController popViewControllerAnimated:YES];
	[challengeToUser displayCompletionWithData:data 
						reChallengeDescription:@"new challenge description based on resultData goes here"];
}

- (void)didFailCompleteChallenge:(OFChallengeToUser*)challengeToUser;
{
	[[[[UIAlertView alloc] initWithTitle:@"Error" message:@"Failed submitting challenge result!" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];
}

- (IBAction)completeChallenge
{
	if (!challenge)
		return;

	OFChallengeResult ofResult;
	switch (result.selectedSegmentIndex)
	{
		case 0: ofResult = kChallengeResultRecipientWon; break;
		case 1: ofResult = kChallengeResultRecipientLost; break;
		case 2: ofResult = kChallengeResultTie; break;
	}

	challenge.resultDescription = resultDescription.text;
	[challenge completeWithResult:ofResult];
}

-(IBAction)rejectChallenge {
    if(challenge) [challenge reject];
}

-(void)didRejectChallenge:(OFChallengeToUser *)challengeToUser {
    [self.navigationController popViewControllerAnimated:YES];
}

-(void)didFailRejectChallenge:(OFChallengeToUser *)challengeToUser {
	[[[[UIAlertView alloc] initWithTitle:@"Error" message:@"Failed to reject the challenge!" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    
}

-(IBAction) writeToFile {
    if(challenge) {
        NSString* filePath = OFTransactionalSaveFile::getSavePathForFile(@"CHALLENGE.XML");
        [challenge writeToFile:filePath];
        OFChallengeToUser* verify = [OFChallengeToUser readFromFile:filePath];
        BOOL different = FALSE;
        different |= [verify.resourceId compare:challenge.resourceId];
        different |= [verify.recipient.name compare:challenge.recipient.name];
        if(different) 
            [[[[UIAlertView alloc] initWithTitle:@"Error" message:@"File did not write correctly!" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    
        else 
            [[[[UIAlertView alloc] initWithTitle:@"Saved" message:@"Challenge written to file" delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil] autorelease] show];    

    }
}

@end

//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import "OFResource.h"
#import "OFChallengeToUser.h"
#import "OFCallbackable.h"

@class OFService;
@class OFChallenge;
@class OFUser;
@class OFRequestHandle;

@protocol OFChallengeToUserDelegate;

enum OFChallengeResult {kChallengeIncomplete, kChallengeResultRecipientWon, kChallengeResultRecipientLost, kChallengeResultTie};

//////////////////////////////////////////////////////////////////////////////////////////
/// A OFChallengeToUser is received when another user has challenged this user.
/// The public interface for OFChallengeToUser allows you to see the details of that
/// challenge, as well as commit a result for that challenge (win/lose/tie) and also
/// send a rechallenge.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFChallengeToUser : OFResource<OFCallbackable>
{
@private
	OFChallenge* challenge;
	OFUser*		recipient;
	OFChallengeResult result;
	NSString* resultDescription;
	NSUInteger attempts;
	BOOL isCompleted;
	BOOL hasBeenViewed;
	
	BOOL hasDecrementedChallengeCount;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFChallengeToUser related actions. Must adopt the 
/// OFChallengeToUserDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFChallengeToUserDelegate>)delegate;

////////////////////////////////////////////////////////////
/// Read the ChallengeToUser data from a file
/// 
/// @param fileName				The name of the file to read the data from
///
/// @return OFChallengeToUser	The  OFChallengeToUser deserialized from the file
////////////////////////////////////////////////////////////
+ (OFChallengeToUser*)readFromFile:(NSString*)fileName;

//////////////////////////////////////////////////////////////////////////////////////////
/// Send when the challenge is complete.
/// 
/// @param challengeResult		OFChallengeResult enum with either win, lose or tie
///
/// @note						This should be called before you call displayChallengeCompletedModal
/// @note						You can not call this with a result of value kChallengeIncomplete.
/// @note						If you would like a description, please set the resultDescription 
///								property on the OFChallenengeToUser before calling this.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didCompleteChallenge:(OFChallengeToUser*)challengeToUser on success and
///					- (void)didFailCompleteChallenge:(OFChallengeToUser*)challengeToUser on failure
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)completeWithResult:(OFChallengeResult)challengeResult;

////////////////////////////////////////////////////////////
/// Reject a challenge
///
/// @note			A rejected challenge will no longer appear in the pending challenges list
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didRejectChallenge:(OFChallengeToUser*)challengeToUser; on success and
///					- (void)didFailRejectChallenge:(OFChallengeToUser*)challengeToUser; on failure
////////////////////////////////////////////////////////////
- (OFRequestHandle*)reject;
 
//////////////////////////////////////////////////////////////////////////////////////////
/// Displays the default OpenFeint View for completing Challenges.  If the user submits this
/// view the challenger will be sent a message about the challenge result.
/// 
/// @param resultData				Only used for multiAttempt challenges. The data needed to send the challenge result out as a new challenge
/// @param reChallengeDescription	If the challenge result is sent out as a new challenge (multi attempt only), this will be the description for
///									the new challenge. Should be formatted the same way as challengeText in sendChallenge
///
/// @note							Call completeWithResult before calling this.	It stores nessiary data for this view
///									If the user turns off the game during the challenge you must serialize 
///									the OFChallengeToUser as its used to display the completion modal. Serialize it to and from disc by calling 
///									writeChallengeToUserToFile or readChallengeToUserFromFile.
///
//////////////////////////////////////////////////////////////////////////////////////////
 - (void)displayCompletionWithData:(NSData*)resultData
			reChallengeDescription:(NSString*)reChallengeDescription;

////////////////////////////////////////////////////////////
/// Write the OFChallengeToUser data to a file
/// 
/// @param fileName				The name of the file to write the data to
/// @param challengeToUser		The OFChallengeToUser to serialize to the file
////////////////////////////////////////////////////////////
- (void)writeToFile:(NSString*)fileName;

//////////////////////////////////////////////////////////////////////////////////////////
/// The challenge which was sent.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) OFChallenge* challenge;

//////////////////////////////////////////////////////////////////////////////////////////
/// the user taht started this challenge
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) OFUser*		recipient;

//////////////////////////////////////////////////////////////////////////////////////////
/// The result of the challenge
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, assign)	OFChallengeResult	result;

//////////////////////////////////////////////////////////////////////////////////////////
/// Description of the result. This is set well calling sendChallengeComplete
///
/// The result description will be prefixed by either the recipients name or You if it's the local
/// player whenever displayed. The result description should not state if the recipient won or lost
///	but contain the statistics of his attempt. 
///	Example: "beat 30 monsters" will turn into "You beat 30 monsters" and will be display next to a icon for win or lose
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain)			NSString* resultDescription;

//////////////////////////////////////////////////////////////////////////////////////////
/// The descprtion of the result defined by resultDescription with You or the user name
/// appended to the front.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)			NSString* formattedResultDescription;

//////////////////////////////////////////////////////////////////////////////////////////
/// Wether or not the challenge has be viewed by the challengee
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)			BOOL hasBeenViewed;

//////////////////////////////////////////////////////////////////////////////////////////
/// Number of challenge attempts by this user.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)			NSUInteger attempts;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
+ (NSString*)getChallengeResultIconName:(OFChallengeResult)result;
+ (NSString*)getResourceName;
@property (nonatomic, assign)			BOOL hasDecrementedChallengeCount;
@property (nonatomic, assign)			BOOL isCompleted;

@end

//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFChallengeToUserDelegate Protocol to receive information regarding 
/// OFChallengeToUser.  You must call OFChallengeToUser's +(void)setDelegate: method to receive
/// information.
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFChallengeToUserDelegate
@optional
//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeToUser class when completeWithResult successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didCompleteChallenge:(OFChallengeToUser*)challengeToUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeToUser class when completeWithResult fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailCompleteChallenge:(OFChallengeToUser*)challengeToUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeToUser class when reject successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didRejectChallenge:(OFChallengeToUser*)challengeToUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeToUser class when reject fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailRejectChallenge:(OFChallengeToUser*)challengeToUser;
@end








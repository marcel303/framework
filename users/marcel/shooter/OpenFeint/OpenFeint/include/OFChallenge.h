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
#import "OFCallbackable.h"

@protocol OFChallengeSendDelegate;
@class OFRequestHandle;
@class OFService;
@class OFChallengeDefinition;
@class OFUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFChallenge allows you to create a challenge and send it to
/// other users or qurery challenge data (if its a member of an OFChallengeToUser).
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFChallenge : OFResource<OFCallbackable>
{
@private
	OFChallengeDefinition* challengeDefinition;
	OFUser* challenger;
	NSString* challengeDescription;
	NSString* userMessage;
	NSString* challengeDataUrl;
	NSString* hiddenText;   //developer customization
	NSData* challengeData;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFChallenge related actions. Must adopt the 
/// OFChallengeDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFChallengeSendDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Initializes a challenge
///
/// @param definition			The the challenge definition
/// @param text					Should state what needs to be fullfilled to complete the challenge
/// @param data					The data needed to replay the challenge
///
/// @note	You may optionally also fillout
///			userMessage		A message entered by the user specific to this challenge. If the user does not enter
///							one a default message should be provided instead
///			hiddenText		Not used directly by OpenFeint. Use this if you want to display any extra data with the 
///							challenge when implementing your own UI
//////////////////////////////////////////////////////////////////////////////////////////
- (OFChallenge*)initWithDefinition:(OFChallengeDefinition*)definition challengeDescription:(NSString*)text challengeData:(NSData*)data;

//////////////////////////////////////////////////////////////////////////////////////////
/// Displays the default OpenFeint View for sending Challenges.  If the user submits this
/// view the challenge will be sent to the players selected in the view.
///
/// @note	If this is called, there is no need to call sendChallenge:
/// @note	When this UI is displayed, it will fill prompt the user to fill out the user message
///			so there is no need to fill out this optional data.
/// @note	Currently hiddenText, and sending challenges as a response to a challenge are not supported through this call.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)displayAndSendChallenge;

//////////////////////////////////////////////////////////////////////////////////////////
/// Sends a challenge to a list of users.  Use this call when you create your own UI for
/// challenges.
///
/// @param toUsers					Array of NSStrings with the id of the users who will receive the challenge (OFUsers resourceId property)
/// @param inResponseToChallenge	For multi attempt challenges only. This is an optional parameter that is currently only used to track statistics
///									on whether a challenge was created directly or as a "re-challenge" after beating a challenge.
///
/// @note							If you call displayAndSendChallengeStart you should not call this function directly.
/// @note							Invokes - (void)didSendChallenge: on success and
///											- (void)didFailSendChallenge: on failure.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)sendChallenge:(OFChallengeDefinition*)challengeDefinition
						  toUsers:(NSArray*)userIds 
			inResponseToChallenge:(OFChallenge*)instigatingChallenge;

//////////////////////////////////////////////////////////////////////////////////////////
///
/// Download's challenge data for a particular challenge.
/// 
/// @param challengeDataUrl		The url of the blob (OFChallengeToUser's challengeDataUrl property)
/// 
/// @note						You should never have to call this function directly unless you implement your own UI.
///								If you are using the OpenFeint views to accept challenges, then you should checkout out
///								OFChallengeDelegate's - (void)userLaunchedChallenge:(OFChallengeToUser*)challengeToLaunch withChallengeData:(NSData*)challengeData;
///								method.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes				- (void)didDownloadChallengeData:(NSData*)data OFChallenge:(OFChallenge*)challenge on success and
///								- (void)didFailDownloadChallengeDataOFChallenge:(OFChallenge*)challenge on failure
///
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)downloadChallengeData;

//////////////////////////////////////////////////////////////////////////////////////////
/// The Challenge Definition related to this challenge
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) OFChallengeDefinition* challengeDefinition;

//////////////////////////////////////////////////////////////////////////////////////////
/// The challenger
/// @note When calling sendChallenge:... this is nil b/c the challenger is always the current user
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) OFUser* challenger;

//////////////////////////////////////////////////////////////////////////////////////////
/// The Challenge Description
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* challengeDescription;

//////////////////////////////////////////////////////////////////////////////////////////
/// The user message for the challenge
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain) NSString* userMessage;

//////////////////////////////////////////////////////////////////////////////////////////
/// hidden text feild defined by the developer.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain) NSString*	hiddenText;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
+ (NSString*)getResourceName;
- (BOOL)usesChallengeData;
@property (nonatomic, readonly) NSData* challengeData;
@property (nonatomic, readonly) NSString* challengeDataUrl;

@end

//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFChallengeDelegate Protocol to receive information regarding 
/// OFChallenge.  You must call OFChallenge's +(void)setDelegate: method to receive
/// information.
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFChallengeSendDelegate<NSObject>
@optional
//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallenge class when sendChallenge successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didSendChallenge:(OFChallenge*)challenge;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallenge class when sendChallenge fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailSendChallenge:(OFChallenge*)challenge;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadChallengeData:(NSData*)data OFChallenge:(OFChallenge*)challenge;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadChallengeDataOFChallenge:(OFChallenge*)challenge;

@end

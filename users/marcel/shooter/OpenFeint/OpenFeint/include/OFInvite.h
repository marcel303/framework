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
#import "OFPointer.h"

@class OFUser, OFInviteDefinition, OFService;
@class OFRequestHandle;
class OFHttpService;
class OFImageViewHttpServiceObserver;

@protocol OFInviteSendDelegate;


//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface to OFInvite allows you to create invites and send them to other users
/// You can also query invites a certain user has.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFInvite : OFResource<OFCallbackable>
{
@private
	OFUser* senderUser;						
	OFUser* receiverUser;					
	
	// copied out of the invite definition.  carrot and suggestedUserMessage are no longer needed.
	NSString* clientApplicationName;		
	NSString* clientApplicationID;			
	NSString* inviteIdentifier;				
	NSString* senderParameter;				
	NSString* receiverParameter;			
	NSString* inviteIconURL;				
	NSString* developerMessage;				
	NSString* receiverNotification;			
	NSString* senderNotification;			
	
	NSString* userMessage;					
	NSString* state;
	
	OFInviteDefinition* inviteDefinition;
	
	OFPointer<OFHttpService> mHttpService;
	OFPointer<OFImageViewHttpServiceObserver> mHttpServiceObserver;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFInvite related actions. Must adopt the 
/// OFInviteSendDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFInviteSendDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Initializes a OFInvite for sending.
///
/// @param inviteDefinition		Which invite definition will define this invite?
///
/// @note	You may optionally also fillout the
///			userMessage
///			@note		If userMessage is not filled out the userMessage will default to the Invite Definitions
///						suggestedSenderMessage.
//////////////////////////////////////////////////////////////////////////////////////////
- (OFInvite*)initWithInviteDefinition:(OFInviteDefinition*)inviteDefinitionIn;

//////////////////////////////////////////////////////////////////////////////////////////
/// Send the invite to an array of users
///
/// @param users				The users which will receive the invite.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didSendInvite:(OFInvite*)invite on success and
///						- (void)didFailSendInvite:(OFInvite*)invite on failure
///
/// @note You must init an invite with initWithInviteDefinition in order to call this function
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)sendInviteToUsers:(NSArray*)users;

//////////////////////////////////////////////////////////////////////////////////////////
/// Displays the OpenFeint invite screen for this invite and prompts to user to send invites
///
/// @note You must init an invite with initWithInviteDefinition in order to call this function
//////////////////////////////////////////////////////////////////////////////////////////
- (void)displayAndSendInviteScreen;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get the Invite Icon associated with this invite
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didGetInviteIcon:(UIImage*)image OFInvite:(OFInvite*)invite; on success and
///						- (void)didFailGetInviteIconOFInvite:(OFInvite*)invite; on failure
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getInviteIcon;

//////////////////////////////////////////////////////////////////////////////////////////
/// Actual user message after the user optionally edited it.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain) NSString* userMessage;

//////////////////////////////////////////////////////////////////////////////////////////
/// Copied from the definition when created
/// @see OFInviteDefinition
/// @note Only for Premium Members
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* senderParameter;

//////////////////////////////////////////////////////////////////////////////////////////
/// Copied from the definition when created
/// @see OFInviteDefinition
/// @note Only for Premium Members
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* receiverParameter;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
+ (NSString*)getResourceName;
@property (nonatomic, readonly) OFUser* senderUser;
@property (nonatomic, readonly) OFUser* receiverUser;
@property (nonatomic, readonly) NSString* clientApplicationName;
@property (nonatomic, readonly) NSString* clientApplicationID;
@property (nonatomic, readonly) NSString* inviteIdentifier;
@property (nonatomic, readonly) NSString* inviteIconURL;
@property (nonatomic, readonly) NSString* developerMessage;
@property (nonatomic, readonly) NSString* receiverNotification;
@property (nonatomic, readonly) NSString* senderNotification;
@property (nonatomic, readonly) NSString* state;

@end

//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFInviteSendDelegate Protocol to receive information regarding OFInvite.  
/// You must call OFInvite's +(void)setDelegate: method to receive information
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFInviteSendDelegate
@optional

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when sendInviteToUsers successfully completes
///
/// @param invite			The invite that was sent successfully
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didSendInvite:(OFInvite*)invite;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when sendInviteToUser fails
///
/// @param invite			The invite that failed to send.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailSendInvite:(OFInvite*)invite;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getInviteIcon successfully completes
///
/// @param image			The image requested
/// @param invite			The invite the image belongs to.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didGetInviteIcon:(UIImage*)image OFInvite:(OFInvite*)invite;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getInviteIcon fails
///
/// @param invite			The OFInvite for which the image was requested.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetInviteIconOFInvite:(OFInvite*)invite;

@end




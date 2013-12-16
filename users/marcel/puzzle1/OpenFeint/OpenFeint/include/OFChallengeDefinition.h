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

@class OFService;
@class OFRequestHandle;
class OFHttpService;
class OFImageViewHttpServiceObserver;

@protocol OFChallengeDefinitionDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFChallengeDefinition allows you to see details on a challenge
/// Definition.  These are the objects related to the challenge types you've defined in
/// the developer dashboard.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFChallengeDefinition : OFResource<OFCallbackable>
{
	@package
	NSString* title;
	NSString* clientApplicationId;
	NSString* iconUrl;
	BOOL multiAttempt;
	
	OFPointer<OFHttpService> mHttpService;
	OFPointer<OFImageViewHttpServiceObserver> mHttpServiceObserver;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFChallengeDefinition related actions. Must adopt the 
/// OFChallengeDefinitionDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFChallengeDefinitionDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// download all challenge definitions
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didDownloadAllChallengeDefinitions:(NSArray*)challengeDefinitions on success and
///						- (void)didFailDownloadChallengeDefinitions on failure.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)downloadAllChallengeDefinitions;

//////////////////////////////////////////////////////////////////////////////////////////
/// get a challenge definition from the server.  
/// 
/// @param challengeDefinitionId	The resource id of the OFChallengeDefinition to return
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didDownloadChallengeDefinition:(OFChallengeDefinition*)challengeDefinition on success and
///					- (void)didFailDownloadChallengeDefinition on failure
///
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)downloadChallengeDefinitionWithId:(NSString*)challengeDefinitionId;

//////////////////////////////////////////////////////////////////////////////////////////
/// Get the challenge icon image
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	- (void)didGetIcon:(UIImage*)image OFChallengeDefintion:(OFChallengeDefinition*)challengeDef; on success and
///					- (void)getFailGetIconOFChallengeDefinition:(OFChallengeDefinition*)challengeDef; on failure
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getIcon;

//////////////////////////////////////////////////////////////////////////////////////////
/// The title defined on the developer dashboard for this challenge definition
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* title;

//////////////////////////////////////////////////////////////////////////////////////////
/// Wether or not the receiver of this challenge can attempt it multiple times to complete.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) BOOL multiAttempt;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* clientApplicationId;
@property (nonatomic, readonly) NSString* iconUrl;
+ (NSString*)getResourceName;

@end

//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFChallengeDefinitionDelegate Protocol to receive information regarding 
/// OFChallengeDefinition.  You must call OFChallengeDefinition's +(void)setDelegate: method to receive
/// information.
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFChallengeDefinitionDelegate
@optional
//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeDefinition class when downloadAllChallengeDefinitions successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadAllChallengeDefinitions:(NSArray*)challengeDefinitions;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeDefinition class when downloadAllChallengeDefinitions fails
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadChallengeDefinitions;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeDefinition class when downloadChallengeDefinitionWithId successfully completes.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadChallengeDefinition:(OFChallengeDefinition*)challengeDefinition;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFChallengeDefinition class when downloadChallengeDefinitionWithId fails
////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadChallengeDefinition;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getIcon completes successfully
///
/// @param image		The icon image for the challenge Definition
/// @param challengeDef	The challenge definition the image belongs to.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didGetIcon:(UIImage*)image OFChallengeDefintion:(OFChallengeDefinition*)challengeDef;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getIcon fails
///
/// @param challengeDef	The OFChallengeDefinition for which the image was requested.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)getFailGetIconOFChallengeDefinition:(OFChallengeDefinition*)challengeDef;
@end

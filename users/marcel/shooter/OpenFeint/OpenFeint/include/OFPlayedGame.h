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
@class OFUserGameStat;
@class OFRequestHandle;
class OFHttpService;
class OFImageViewHttpServiceObserver;

@protocol OFPlayedGameDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// The OFPlayedGame interface allows you to get a list of your featured games.  This object
/// will allow you to query information about the game, such as its name and its apple store url,
/// but also allow you to query specific relationships between this game and the current user.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFPlayedGame : OFResource<OFCallbackable>
{
@private
	NSString* name;
	NSString* iconUrl;
	NSString* clientApplicationId;
	NSUInteger totalGamerscore;
	NSUInteger friendsWithApp;
	NSMutableArray* userGameStats;
	NSString* iTunesAppStoreUrl;
	BOOL favorite;
	NSString* review;
	OFPointer<OFHttpService> mHttpService;
	OFPointer<OFImageViewHttpServiceObserver> mHttpServiceObserver;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFPlayedGame related actions. Must adopt the 
/// OFPlayedGameDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFPlayedGameDelegate>)delegate;

/////////////////////////////////////////////////////////////////////////////////////
/// Get a list of OFPlayedGames which are featured by the app.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didGetFeaturedGames:(NSArray*)featuredGames on success and
///						- (void)didFailGetFeaturedGames on failure.
/////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)getFeaturedGames;

/////////////////////////////////////////////////////////////////////////////////////
/// Whether or not the current user has the game.
///
/// @return BOOL	@c YES if the current user owns the game.
/////////////////////////////////////////////////////////////////////////////////////
- (BOOL)isOwnedByCurrentUser;

/////////////////////////////////////////////////////////////////////////////////////
/// get the game icon image
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didGetGameIcon:(UIImage*)image OFPlayedGame:(OFPlayedGame*)game; on success and
///						- (void)didFailGetGameIconOFPlayedGame(OFPlayedGame*)game on failure.
/////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getGameIcon;

/////////////////////////////////////////////////////////////////////////////////////
/// The name of the game
/////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* name;

/////////////////////////////////////////////////////////////////////////////////////
/// The client application id for the game
/////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* clientApplicationId;

/////////////////////////////////////////////////////////////////////////////////////
/// The iTunes app store url to buy the app
/////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* iTunesAppStoreUrl;

/////////////////////////////////////////////////////////////////////////////////////
/// @internal
/////////////////////////////////////////////////////////////////////////////////////
+ (NSString*)getResourceName;
- (OFUserGameStat*)getLocalUsersGameStat;
@property (nonatomic, readonly) NSMutableArray* userGameStats;
@property (nonatomic, readonly) NSString* iconUrl;
@property (nonatomic, readonly) NSUInteger totalGamerscore;
@property (nonatomic, readonly) NSUInteger friendsWithApp;
@property (nonatomic, readonly)	BOOL favorite;
@property (nonatomic, readonly)	NSString* review;

@end

/////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFPlayedGameDelegate Protocol to receive information regarding OFPlayedGame.
/// You must call OFPlayedGames's +(void)setDelegate: method to receive information.
/////////////////////////////////////////////////////////////////////////////////////
@protocol OFPlayedGameDelegate
@optional
/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getFeaturedGames successfully completes
///
/// @param NSArray		An array of OFPlayedGame objects which are featured by your app
/////////////////////////////////////////////////////////////////////////////////////
- (void)didGetFeaturedGames:(NSArray*)featuredGames;

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getFeatureGames fails.
/////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetFeaturedGames;

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getGameIcon successfully completes.
///
/// @param image		The image requested
/// @param game			The OFPlayedGame that the image belongs to.
/////////////////////////////////////////////////////////////////////////////////////
- (void)didGetGameIcon:(UIImage*)image OFPlayedGame:(OFPlayedGame*)game;

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getGameIcon successfully completes
///
/// @param game			The OFPlayedGame for which the image was requested.
/////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetGameIconOFPlayedGame:(OFPlayedGame*)game;

@end

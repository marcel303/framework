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

#pragma once

@class OFAnnouncement;
@class OFHighScore;

@protocol OpenFeintDelegate<NSObject>

@optional

////////////////////////////////////////////////////////////
///
/// @note This is where you should pause your game.
///
////////////////////////////////////////////////////////////
- (void)dashboardWillAppear;

////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////
- (void)dashboardDidAppear;

////////////////////////////////////////////////////////////
///
/// @note This is where Cocoa based games should unpause and resume playback. 
///
/// @warning	Since an exit animation will play, this will cause negative performance if your game 
///				is rendering on an EAGLView. For OpenGL games, you should refresh your view once and
///				resume your game in dashboardDidDisappear
///
////////////////////////////////////////////////////////////
- (void)dashboardWillDisappear;

////////////////////////////////////////////////////////////
///
/// @note This is where OpenGL games should unpause and resume playback.
///
////////////////////////////////////////////////////////////
- (void)dashboardDidDisappear;

////////////////////////////////////////////////////////////
///
/// @note This is called when a user logs in and OpenFeint is offline.
/// @note The userId of "0" represents a user who has not successfully connected to OpenFeint at least once.
///
////////////////////////////////////////////////////////////
- (void)offlineUserLoggedIn:(NSString*)userId;

////////////////////////////////////////////////////////////
///
/// @note This is called when a user logs in and OpenFeint is online.
///
////////////////////////////////////////////////////////////
- (void)userLoggedIn:(NSString*)userId;

////////////////////////////////////////////////////////////
///
/// @note This is called whenever a user explicitly logs out from OpenFeint from within the dashboard
///	      This is NOT called when the player switches account within the OpenFeint dashboard. In that case only userLoggedIn is called with the new user.
///
////////////////////////////////////////////////////////////
- (void)userLoggedOut:(NSString*)userId;

////////////////////////////////////////////////////////////
///
/// @note Developers can override this method to customize the OpenFeint approval screen. Returning YES will prevent OpenFeint from
///       displaying it's default approval screen. If a developer chooses to override the approval screen they MUST call 
///       [OpenFeint userDidApproveFeint:(BOOL)approved] before OpenFeint will function.
///
////////////////////////////////////////////////////////////
- (BOOL)showCustomOpenFeintApprovalScreen;

////////////////////////////////////////////////////////////
///
/// @note Developers can override this method to customize the developer announcement screen. Returning YES will prevent OpenFeint from
///       displaying it's default screen.
///
////////////////////////////////////////////////////////////
- (BOOL)showCustomScreenForAnnouncement:(OFAnnouncement*)announcement;

////////////////////////////////////////////////////////////
///
/// @note This gets called when the player from a leaderboard within OpenFeint decides to download and interact with it's data.
///		  The game should here close the dashboard and take the appropriate action. The blob is also available as a property on the highScore object.
///
////////////////////////////////////////////////////////////
- (void)userDownloadedBlob:(NSData*)blob forHighScore:(OFHighScore*)highScore;

@end

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

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFSocialNotificationApi allows you to post notifications to
/// social networks if the user is connected via OpenFeint to a social network.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFSocialNotificationApi : NSObject

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a url for social notifications you post through the api.  This will be linked to 
/// social notification posts.
///
/// @param url			the url linked on social notifications
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setCustomUrl:(NSString*)url;

//////////////////////////////////////////////////////////////////////////////////////////
/// Prompt the current user to post to social networks.
///
/// @param text			Text to attach to the notification
/// @param imageName	Name of the image on the developer dashboard
///
/// @note imageName can be nil if there is no imgage to post.
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)sendWithText:(NSString*)text imageNamed:(NSString*)imageName;

@end

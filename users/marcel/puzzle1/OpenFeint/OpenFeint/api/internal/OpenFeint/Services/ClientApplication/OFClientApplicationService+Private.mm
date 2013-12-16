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

#import "OFClientApplicationService+Private.h"
#import "OFHttpNestedQueryStringWriter.h"
#import "OFService+Private.h"
#import "OpenFeint+UserOptions.h"


@implementation OFClientApplicationService (Private)

+ (OFRequestHandle*)setGameIsFavorite:(NSString*)clientApplicationId favorite:(bool)favorite review:(NSString*)review onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	
	{
		OFHttpNestedQueryStringWriter::Scope scope(params, "client_application_user");
		if (review)
		{
			OFAssert(favorite, @"Can't submit reviews for game's that aren't favorite");
			params->io("review", review);
		}
		params->io("favorite", favorite);
	}
	
	
	return [[self sharedInstance] 
	 postAction:[NSString stringWithFormat:@"client_applications/%@/users/review.xml", clientApplicationId ? clientApplicationId : [OpenFeint clientApplicationId]]
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:nil];
}

+ (OFRequestHandle*) makeGameFavorite:(NSString*)clientApplicationId reviewText:(NSString*)review onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFClientApplicationService setGameIsFavorite:clientApplicationId favorite:true review:review onSuccess:onSuccess onFailure:onFailure];
}

+ (void) unfavoriteGame:(NSString*)clientApplicationId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFClientApplicationService setGameIsFavorite:clientApplicationId favorite:NO review:nil onSuccess:onSuccess onFailure:onFailure];
}

+ (void) viewedFanClub:(NSString*)clientApplicationId
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	if (clientApplicationId == nil)
	{
		clientApplicationId = [OpenFeint clientApplicationId];
	}
	params->io("client_application_id", clientApplicationId);
	
	[[self sharedInstance] 
	 getAction:@"client_application_users/fan_club"
	 withParameters:params
	 withSuccess:OFDelegate()
	 withFailure:OFDelegate()
	 withRequestType:OFActionRequestSilentIgnoreErrors
	 withNotice:nil];
}

@end

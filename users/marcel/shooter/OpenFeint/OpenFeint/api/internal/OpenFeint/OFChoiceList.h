////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2010 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 Presents the user with a list of choices, the choice that was made is returned back to the caller in a delegate
On the iPad, this will basically be a wrapper for a popover, on the iPhone, a small view with buttons 
 */
@protocol OFChoiceListDelegate
//a -1 is returned if the background object is selected
-(void)userDidChoiceListWithAnswer:(int)answer;

@end

@interface OFChoiceList : NSObject

+(void) showChoiceListWithDelegate:(NSObject<OFChoiceListDelegate>*) delegate location:(CGPoint) location choices:(NSArray*) choices;

@end



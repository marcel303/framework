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

#import "OFTabBarController.h"
#import "OFTabBarControllerDelegate.h"
#import "OFTickerView.h"
#import "OFTickerProxy.h"
#import "OFWebNavController.h"

@interface OFTabbedDashboardController : OFTabBarController<OFTabBarControllerDelegate, OFTickerViewDelegate, OFTickerProxyRecipient>
{
    UIView* hostView;
    OFTickerView* tickerView;
    NSTimer* tickerTimer;
    OFWebNavController *mModalDashboardContentController;
}

@property (nonatomic, retain) IBOutlet UIView *hostView;
@property (nonatomic, retain) IBOutlet OFTickerView *tickerView;
@property (nonatomic, retain) OFWebNavController *mModalDashboardContentController;


+ (void)setOnlineMode:(OFTabBarController*)tabBarController;
+ (void)setOfflineMode:(OFTabBarController*)tabBarController;
- (NSString*)defaultTab;
- (NSString*)offlineTab;

- (id)init;
- (id)initWithCoder:(NSCoder *)aDecoder;

@end

extern NSString* globalChatRoomControllerName;
extern NSString* globalChatRoomTabName;

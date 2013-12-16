/*

 AdWhirlView+.h

 Copyright 2009 AdMob, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

#import "AdWhirlAdNetworkAdapter.h"

@interface AdWhirlView ()

- (id)initWithDelegate:(id<AdWhirlDelegate>)delegate;
- (void)prepAdNetworks;
- (AdWhirlAdNetworkConfig *)nextNetworkByPercent;
- (AdWhirlAdNetworkConfig *)nextNetworkByPriority;
- (void)makeAdRequest:(BOOL)isFirstRequest;
- (void)scheduleNextAdRefresh;
- (void)notifyExImpression:(NSString *)nid netType:(AdWhirlAdNetworkType)type;
- (void)notifyExClick:(NSString *)nid netType:(AdWhirlAdNetworkType)type;
- (BOOL)canRefresh;
- (void)resignActive:(NSNotification *)notification;
- (void)becomeActive:(NSNotification *)notification;

@property (retain) AdWhirlConfig *config;
@property (retain) NSMutableArray *prioritizedAdNetworks;
@property (nonatomic,retain) AdWhirlAdNetworkAdapter *currAdapter;
@property (nonatomic,retain) AdWhirlAdNetworkAdapter *lastAdapter;
@property (nonatomic,retain) NSDate *lastRequestTime;
@property (nonatomic,retain) NSTimer *refreshTimer;
@property (nonatomic) BOOL showingModalView;

@end

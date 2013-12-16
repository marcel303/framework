/*

 AdWhirlView.h

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

#import <UIKit/UIKit.h>
#import "AdWhirlDelegateProtocol.h"
#import "AdWhirlConfig.h"

#define kAdWhirlAppVer 255

#define kAdWhirlViewWidth 320
#define kAdWhirlViewHeight 50
#define kAdWhirlViewDefaultSize (CGSizeMake(kAdWhirlViewWidth, kAdWhirlViewHeight))
#define kAdWhirlViewDefaultFrame (CGRectMake(0,0,kAdWhirlViewWidth, kAdWhirlViewHeight))

#define kAdWhirlDefaultConfigURL @"http://mob.adwhirl.com/getInfo.php"
#define kAdWhirlDefaultImpMetricURL @"http://met.adwhirl.com/exmet.php"
#define kAdWhirlDefaultClickMetricURL @"http://met.adwhirl.com/exclick.php"
#define kAdWhirlDefaultCustomAdURL @"http://mob.adwhirl.com/custom.php"


@class AdWhirlAdNetworkConfig;
@class AdWhirlAdNetworkAdapter;

@interface AdWhirlView : UIView <AdWhirlConfigDelegate> {
  id<AdWhirlDelegate> delegate;
  AdWhirlConfig *config;

  NSMutableArray *prioritizedAdNetworks;
  double totalPercent;

  BOOL ignoreAutoRefreshTimer;
  BOOL ignoreNewAdRequests;
  BOOL appInactive;
  BOOL showingModalView;

  BOOL requesting;
  AdWhirlAdNetworkAdapter *currAdapter;
  AdWhirlAdNetworkAdapter *lastAdapter;
  NSDate *lastRequestTime;

  NSTimer *refreshTimer;
  id lastNotifyAdapter; // remember which adapter we last sent click stats for so we don't send twice

  NSError *lastError;
}

/**
 * Call this method to get a view object that you can add to your own view. You
 * must also provide a delegate.  The delegate provides AdWhirl's application key
 * and can listen for important messages.  You can configure the view's settings
 * and specific ad network information on AdWhirl.com or your own adwhirl
 * server instance.
 */
+ (AdWhirlView *)requestAdWhirlViewWithDelegate:(id<AdWhirlDelegate>)delegate;

/**
 * Starts pre-fetching ad network configurations from an AdWhirl server. If the
 * configuration has been fetched when you are ready to request an ad, you save
 * a round-trip to the network and hence your ad may show up faster. You
 * typically call this in the applicationDidFinishLaunching: method of your
 * app delegate. The request is non-blocking. You only need to call this
 * at most once per run of your application. Subsequent calls to this function
 * will be ignored.
 */
+ (void)startPreFetchingConfigurationDataWithDelegate:(id<AdWhirlDelegate>)delegate;

/**
 * Call this method to request a new configuration from the AdWhirl servers.
 * This can be useful to support iOS 4.0 backgrounding.
 */
+ (void)updateAdWhirlConfigWithDelegate:(id<AdWhirlDelegate>)delegate;

/**
 * Call this method to request a new configuration from the AdWhirl servers.
 * This can be useful to support iOS 4.0 backgrounding.
 */
- (void)updateAdWhirlConfig;

/**
 * Call this method to get another ad to display. You can also specify under
 * "app settings" on the AdWhirl server to automatically get new ads
 * periodically (30 seconds, 45 seconds, 1 min, etc.)
 */
- (void)requestFreshAd;

/**
 * Call this method if you prefer a rollover instead of a getNextAd call.  This
 * is offered primarily for developers who want to use generic notifications and
 * then execute a rollover when an ad network fails to serve an ad.
 */
- (void)rollOver;

/**
 * The delegate is informed asynchronously whether an ad succeeds or fails to load.
 * If you prefer to poll for this information, you can do so using this method.
 *
 */
- (BOOL)adExists;

/**
 * Different ad networks may return different ad sizes. You may adjust the size
 * of the AdWhirlView and your UI to avoid unsightly borders or chopping off
 * pixels from ads. Call this method when you receive the adWhirlDidReceiveAd
 * delegate method to get the size of the underlying ad network ad.
 */
- (CGSize)actualAdSize;

/**
 * Some ad networks may offer different banner sizes for different orientations.
 * Call this function when the orientation of your UI changes so the underlying
 * ad may handle the orientation change properly. You may also want to
 * call the actualAdSize method right after calling this to get the size of
 * the ad after the orientation change.
 */
- (void)rotateToOrientation:(UIInterfaceOrientation)orientation;

/**
 * Call this method to get the name of the most recent ad network that an ad request was made to.
 */
- (NSString *)mostRecentNetworkName;

/**
 * Call this method to ignore the automatic refresh timer.
 *
 * Note that the refresh timer is NOT invalidated when you call ignoreAutoRefreshTimer.
 * This will simply ignore the refresh events that are called by the automatic
 * refresh timer (if the refresh timer is enabled via AdWhirl.com).  So, for
 * example, let's say you have a refresh cycle of 60 seconds.  If you call
 * ignoreAutoRefreshTimer at 30 seconds, and call resumeRefreshTimer at 90 sec,
 * then the first refresh event is ignored, but the second refresh event at 120
 * sec will run.
 */
- (void)ignoreAutoRefreshTimer;
- (void)doNotIgnoreAutoRefreshTimer;

/**
 * Call this method to ignore automatic refreshes AND manual refreshes entirely.
 *
 * This is provided for developers who asked to disable refreshing entirely,
 * whether automatic or manual.
 * If you call ignoreNewAdRequests, the AdWhirl will:
 * 1) Ignore any Automatic refresh events (via the refresh timer) AND
 * 2) Ignore any manual refresh calls (via getNextAd and rollOver)
 * whether they are run automatically (via the refresh timer) or manually (via
 * calls to requestFreshAd and rollOver).
 */
- (void)ignoreNewAdRequests;
- (void)doNotIgnoreNewAdRequests;

/**
 * Call this to replace the content of this AdWhirlView with the view.
 */
- (void)replaceBannerViewWith:(UIView*)bannerView;

/**
 * Make sure you set the delegate to nil when you release an AdWhirlView
 * instance to avoid the AdWhirlView from calling to a non-existent delegate.
 */
@property (nonatomic, assign) IBOutlet id<AdWhirlDelegate> delegate;

/**
 * Use this to retrieve more information after the delegate received a
 * adWhirlDidFailToReceiveAd message.
 */
@property (nonatomic, readonly) NSError *lastError;


#pragma mark for ad network adapters use only

/**
 * Called by Adapters when there's a new ad view.
 */
- (void)adapter:(AdWhirlAdNetworkAdapter *)adapter didReceiveAdView:(UIView *)view;

/**
 * Called by Adapters when ad view failed.
 */
- (void)adapter:(AdWhirlAdNetworkAdapter *)adapter didFailAd:(NSError *)error;

/**
 * Called by Adapters when the ad request is finished, but the ad view is
 * furnished elsewhere. e.g. Generic Notification
 */
- (void)adapterDidFinishAdRequest:(AdWhirlAdNetworkAdapter *)adapter;

@end

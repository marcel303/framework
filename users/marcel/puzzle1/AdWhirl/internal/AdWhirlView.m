/*

 AdWhirlView.m

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

#import "AdWhirlView.h"
#import "AdWhirlView+.h"
#import "AdWhirlConfigStore.h"
#import "AdWhirlAdNetworkConfig.h"
#import "CJSONDeserializer.h"
#import "AdWhirlLog.h"
#import "AdWhirlAdNetworkAdapter.h"
#import "AdWhirlError.h"
#import "AdWhirlConfigStore.h"

#define kAdWhirlViewAdSubViewTag   1000
#define kAWMinimumTimeBetweenFreshAdRequests 4.9f

NSInteger adNetworkPriorityComparer(id a, id b, void *ctx) {
  AdWhirlAdNetworkConfig *acfg = a, *bcfg = b;
  if(acfg.priority < bcfg.priority)
    return NSOrderedAscending;
  else if(acfg.priority > bcfg.priority)
    return NSOrderedDescending;
  else
    return NSOrderedSame;
}

@implementation AdWhirlView

@synthesize delegate;
@synthesize config;
@synthesize prioritizedAdNetworks;
@synthesize currAdapter;
@synthesize lastAdapter;
@synthesize lastRequestTime;
@synthesize refreshTimer;
@synthesize lastError;
@synthesize showingModalView;

+ (AdWhirlView *)requestAdWhirlViewWithDelegate:(id<AdWhirlDelegate>)delegate {
  if (![delegate respondsToSelector:@selector(viewControllerForPresentingModalView)]) {
    [NSException raise:@"AdWhirlIncompleteDelegateException"
                format:@"AdWhirlDelegate must implement viewControllerForPresentingModalView"];
  }
  return [[[AdWhirlView alloc] initWithDelegate:delegate] autorelease];
}

static id<AdWhirlDelegate> classAdWhirlDelegateForConfig = nil;

+ (void)startPreFetchingConfigurationDataWithDelegate:(id<AdWhirlDelegate>)delegate {
  if (classAdWhirlDelegateForConfig != nil) {
    AWLogWarn(@"Called startPreFetchingConfig when another fetch is in progress");
    return;
  }
  classAdWhirlDelegateForConfig = delegate;
  [[AdWhirlConfigStore sharedStore] getConfig:[delegate adWhirlApplicationKey]
                                     delegate:(id<AdWhirlConfigDelegate>)self];
}

+ (void)updateAdWhirlConfigWithDelegate:(id<AdWhirlDelegate>)delegate {
  if (classAdWhirlDelegateForConfig != nil) {
    AWLogWarn(@"Called updateConfig when another fetch is in progress");
    return;
  }
  classAdWhirlDelegateForConfig = delegate;
  [[AdWhirlConfigStore sharedStore] fetchConfig:[delegate adWhirlApplicationKey]
                                       delegate:(id<AdWhirlConfigDelegate>)self];
}

- (id)initWithDelegate:(id<AdWhirlDelegate>)d {
  self = [super initWithFrame:kAdWhirlViewDefaultFrame];
  if (self != nil) {
    delegate = d;
    self.backgroundColor = [UIColor clearColor];
    self.clipsToBounds = YES; // to prevent ugly artifacts if ad network banners are bigger than the default frame
    showingModalView = NO;
    appInactive = NO;

    AdWhirlConfig *cfg
      = [[AdWhirlConfigStore sharedStore] getConfig:[delegate adWhirlApplicationKey]
                                           delegate:(id<AdWhirlConfigDelegate>)self];
    self.config = cfg;

    NSNotificationCenter *notifCenter = [NSNotificationCenter defaultCenter];
    [notifCenter addObserver:self
                    selector:@selector(resignActive:)
                        name:UIApplicationWillResignActiveNotification
                      object:nil];
    [notifCenter addObserver:self
                    selector:@selector(becomeActive:)
                        name:UIApplicationDidBecomeActiveNotification
                      object:nil];
  }
  return self;
}

- (void)setDelegate:(id <AdWhirlDelegate>)theDelegate {
  [self willChangeValueForKey:@"delegate"];
  delegate = theDelegate;
  if (self.currAdapter) {
    self.currAdapter.adWhirlDelegate = theDelegate;
  }
  if (self.lastAdapter) {
    self.lastAdapter.adWhirlDelegate = theDelegate;
  }
  [self didChangeValueForKey:@"delegate"];
}

- (void)updateAdWhirlConfig {
  self.config = [[AdWhirlConfigStore sharedStore]
                 fetchConfig:[delegate adWhirlApplicationKey]
                    delegate:(id<AdWhirlConfigDelegate>)self];

}

- (void)prepAdNetworks {
  NSMutableArray *freshNets = [[NSMutableArray alloc] initWithArray:config.adNetworkConfigs];
  [freshNets sortUsingFunction:adNetworkPriorityComparer context:nil];
  totalPercent = 0.0;
  for (AdWhirlAdNetworkConfig *cfg in freshNets) {
    totalPercent += cfg.trafficPercentage;
  }
  self.prioritizedAdNetworks = freshNets;
  [freshNets release];
}

static BOOL randSeeded = NO;

- (AdWhirlAdNetworkConfig *)nextNetworkByPercent {
  // get random number based on the current totalPercent
  // walk down the prioritizedAdNetworks array subtracting the totalPercent
  // return AdNetwork.
  if (!randSeeded) {
    srandom(CFAbsoluteTimeGetCurrent());
    randSeeded = YES;
  }

  double dart = ((double)random()/RAND_MAX) * totalPercent;

  double tempTotal = 0.0;

  AdWhirlAdNetworkConfig *result = nil;
  for (AdWhirlAdNetworkConfig *network in prioritizedAdNetworks) {
    result = network; // make sure there is always a network chosen
    tempTotal += network.trafficPercentage;
    if (dart < tempTotal) {
      // this is the one to use.
      break;
    }
  }

  AWLogDebug(@"nextNetworkByPercent chosen %@ (%@), dart %lf in %lf",
        result.nid, result.networkName, dart, totalPercent);
  return result;
}

- (AdWhirlAdNetworkConfig *)nextNetworkByPriority {
  AdWhirlAdNetworkConfig *result = [prioritizedAdNetworks objectAtIndex:0];
  AWLogDebug(@"nextNetworkByPriority chosen %@ (%@)", result.nid, result.networkName);
  return result;
}

- (void)makeFirstAdRequest
{
  [self makeAdRequest:YES];
}

- (void)makeAdRequest:(BOOL)isFirstRequest
{
  // only make request in main thread
  if (![NSThread isMainThread]) {
    // respawn this call on the main thread.
    if (isFirstRequest) {
      [self performSelectorOnMainThread:@selector(makeFirstAdRequest) withObject:nil waitUntilDone:NO];
    }
    else {
      [self performSelectorOnMainThread:@selector(rollOver) withObject:nil waitUntilDone:NO];
    }
    return;
  }

  if ([prioritizedAdNetworks count] == 0) {
    // ran out of ad networks
    [lastError release];
    lastError = [[AdWhirlError alloc] initWithCode:AdWhirlAdRequestNoMoreAdNetworks
                                       description:@"No more ad networks for roll over"];
    if ([delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)]) {
      [[self retain] autorelease]; // to prevent self being freed before this returns
      [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
    }
    return;
  }

  @synchronized(self) {
    if (requesting) {
      AWLogWarn(@"Already requesting ad");
      return;
    }
    requesting = YES;
  }

  AdWhirlAdNetworkConfig *nextAdNetwork = nil;

  if(isFirstRequest)
  {
    nextAdNetwork = [self nextNetworkByPercent];
  }
  else
  {
    nextAdNetwork = [self nextNetworkByPriority];
  }

  AdWhirlAdNetworkAdapter *adapter =
    [[nextAdNetwork.adapterClass alloc] initWithAdWhirlDelegate:delegate
                                                           view:self
                                                         config:config
                                                  networkConfig:nextAdNetwork];
  self.lastAdapter = self.currAdapter;
  self.currAdapter = adapter;
  [adapter release];

  // take nextAdNetwork out so we don't request again when we roll over
  [prioritizedAdNetworks removeObject:nextAdNetwork];

  if(lastRequestTime)
    [lastRequestTime release];
  lastRequestTime = [[NSDate date] retain];

  [currAdapter getAd];
}

- (void)_requestFreshAdInternal {
  NSTimeInterval sinceLast = -[lastRequestTime timeIntervalSinceNow];
  if (lastRequestTime == nil || sinceLast > kAWMinimumTimeBetweenFreshAdRequests) {
    @synchronized(self) {
      if (!config) {
        [lastError release];
        lastError = [[AdWhirlError alloc] initWithCode:AdWhirlAdRequestNoConfigError
                                           description:@"No ad configuration"];
        // no config, can't do anything
        if ([delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)]) {
          [[self retain] autorelease]; // to prevent self being freed before this returns
          [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
        }
        return;
      }
      [self prepAdNetworks];
    }
    [self makeFirstAdRequest];
  }
  else if (sinceLast <= kAWMinimumTimeBetweenFreshAdRequests) {
    [lastError release];
    NSString *desc = [NSString stringWithFormat:
                      @"Requesting fresh ad too soon! It has been only %lfs. Minimum %lfs",
                      sinceLast, kAWMinimumTimeBetweenFreshAdRequests];
    lastError = [[AdWhirlError alloc] initWithCode:AdWhirlAdRequestTooSoonError
                                       description:desc];
    if ([delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)]) {
      [[self retain] autorelease]; // to prevent self being freed before this returns
      [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
    }
    return;
  }
}

- (void)requestFreshAd {
  // public method, hence more checks with delegate calls
  if (ignoreNewAdRequests) {
    // don't request new ad
    [lastError release];
    lastError = [[AdWhirlError alloc] initWithCode:AdWhirlAdRequestIgnoredError
                                       description:@"ignoreNewAdRequests flag set"];
    if ([delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)]) {
      [[self retain] autorelease]; // to prevent self being freed before this returns
      [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
    }
  }
  if (requesting) {
    // don't request if there's a request outstanding
    [lastError release];
    lastError = [[AdWhirlError alloc] initWithCode:AdWhirlAdRequestInProgressError
                                       description:@"Ad request already in progress"];
    if ([delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)]) {
      [[self retain] autorelease]; // to prevent self being freed before this returns
      [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
    }
  }
  [self _requestFreshAdInternal];
}

- (void)requestFreshAdTimer {
  self.refreshTimer = nil;
  if (![self canRefresh]) {
    // don't make ad request, but schedule the next one
    [self scheduleNextAdRefresh];
  }
  else if (ignoreNewAdRequests) {
    // don't make ad request at all
    AWLogDebug(@"Received ad timer, but ignoreNewAdRequests flag set. Do not make request");
  }
  else {
    [self _requestFreshAdInternal];
  }
}

- (void)scheduleNextAdRefresh {
  if (config.refreshInterval > kAWMinimumTimeBetweenFreshAdRequests
      && !ignoreNewAdRequests) {
    AWLogDebug(@"Scheduling next ad refresh after %lfs", config.refreshInterval);
    if (refreshTimer) {
      [refreshTimer invalidate];
    }
    self.refreshTimer = [NSTimer scheduledTimerWithTimeInterval:config.refreshInterval
                                                         target:self
                                                       selector:@selector(requestFreshAdTimer)
                                                       userInfo:nil
                                                        repeats:NO];
  }
}

- (void)rollOver {
  if (ignoreNewAdRequests) return; // don't request ad
  [self makeAdRequest:NO];
}

- (BOOL)adExists {
  UIView *currAdView = [self viewWithTag:kAdWhirlViewAdSubViewTag];
  return currAdView != nil;
}

- (CGSize)actualAdSize {
  if (currAdapter == nil || currAdapter.adNetworkView == nil)
    return kAdWhirlViewDefaultSize;
  return currAdapter.adNetworkView.frame.size;
}

- (void)rotateToOrientation:(UIInterfaceOrientation)orientation {
  if (currAdapter == nil) return;
  [currAdapter rotateToOrientation:orientation];
}

- (NSString *)mostRecentNetworkName {
  if (currAdapter == nil) return nil;
  return currAdapter.networkConfig.networkName;
}

- (void)ignoreAutoRefreshTimer {
  ignoreAutoRefreshTimer = YES;
}

- (void)doNotIgnoreAutoRefreshTimer {
  ignoreAutoRefreshTimer = NO;
}

- (void)ignoreNewAdRequests {
  ignoreNewAdRequests = YES;
}

- (void)doNotIgnoreNewAdRequests {
  ignoreNewAdRequests = NO;
}

- (void)transitionToView:(UIView *)view {
  UIView *currAdView = [self viewWithTag:kAdWhirlViewAdSubViewTag];
  if (view == currAdView) {
    AWLogDebug(@"ignoring ad transition to itself");
    return; // no need to transition to itself
  }
  view.tag = kAdWhirlViewAdSubViewTag;
  if (currAdView) {
    // swap
    currAdView.tag = 0;
    if (config.bannerAnimationType == AWBannerAnimationTypeNone) {
      [currAdView removeFromSuperview];
      [self addSubview:view];
      if ([delegate respondsToSelector:@selector(adWhirlDidAnimateToNewAdIn:)]) {
        // no animation, callback right away
        [(NSObject *)delegate performSelectorOnMainThread:@selector(adWhirlDidAnimateToNewAdIn:)
                                               withObject:self
                                            waitUntilDone:NO];
      }
    }
    else {
      AWBannerAnimationType animType;
      if (config.bannerAnimationType == AWBannerAnimationTypeRandom) {
        if (!randSeeded) {
          srandom(CFAbsoluteTimeGetCurrent());
        }
        // range is 1 to 7, inclusive
        animType = (random() % 7) + 1;
        AWLogDebug(@"Animation type chosen by random is %d", animType);
      }
      else {
        animType = config.bannerAnimationType;
      }

      switch (animType) {
        case AWBannerAnimationTypeSlideFromLeft:
        {
          CGRect f = view.frame;
          f.origin.x = -f.size.width;
          view.frame = f;
          [self addSubview:view];
          break;
        }
        case AWBannerAnimationTypeSlideFromRight:
        {
          CGRect f = view.frame;
          f.origin.x = self.frame.size.width;
          view.frame = f;
          [self addSubview:view];
          break;
        }
        case AWBannerAnimationTypeFadeIn:
          view.alpha = 0;
          [self addSubview:view];
          break;
        default:
          // no setup required for other animation types
          break;
      }

      [currAdView retain]; // will be released when animation is done
      AWLogDebug(@"Beginning AdWhirlAdTransition animation currAdView %x incoming %x", currAdView, view);
      [UIView beginAnimations:@"AdWhirlAdTransition" context:currAdView];
      [UIView setAnimationDelegate:self];
      [UIView setAnimationDidStopSelector:@selector(newAdAnimationDidStopWithAnimationID:finished:context:)];
      [UIView setAnimationBeginsFromCurrentState:YES];
      [UIView setAnimationDuration:1.0];
      // cache has to set to NO because of VideoEgg
      switch (animType) {
        case AWBannerAnimationTypeFlipFromLeft:
          [self addSubview:view];
          [currAdView removeFromSuperview];
          [UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft
                                 forView:self cache:NO];
          break;
        case AWBannerAnimationTypeFlipFromRight:
          [self addSubview:view];
          [currAdView removeFromSuperview];
          [UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight
                                 forView:self cache:NO];
          break;
        case AWBannerAnimationTypeCurlUp:
          [self addSubview:view];
          [currAdView removeFromSuperview];
          [UIView setAnimationTransition:UIViewAnimationTransitionCurlUp
                                 forView:self cache:NO];
          break;
        case AWBannerAnimationTypeCurlDown:
          [self addSubview:view];
          [currAdView removeFromSuperview];
          [UIView setAnimationTransition:UIViewAnimationTransitionCurlDown
                                 forView:self cache:NO];
          break;
        case AWBannerAnimationTypeSlideFromLeft:
        case AWBannerAnimationTypeSlideFromRight:
        {
          CGRect f = view.frame;
          f.origin.x = 0;
          view.frame = f;
          break;
        }
        case AWBannerAnimationTypeFadeIn:
          view.alpha = 1.0;
          break;
        default:
          [self addSubview:view];
          AWLogWarn(@"Unrecognized Animation type: %d", animType);
          break;
      }
      [UIView commitAnimations];
    }
  }
  else {
    // new
    [self addSubview:view];
    if ([delegate respondsToSelector:@selector(adWhirlDidAnimateToNewAdIn:)]) {
      // no animation, callback right away
      [(NSObject *)delegate performSelectorOnMainThread:@selector(adWhirlDidAnimateToNewAdIn:)
                                             withObject:self
                                          waitUntilDone:NO];
    }
  }
}

- (void)replaceBannerViewWith:(UIView*)bannerView {
  [self transitionToView:bannerView];
}

// Called at the end of the new ad animation; we use this opportunity to do memory management cleanup.
// See the comment in adDidLoad:.
- (void)newAdAnimationDidStopWithAnimationID:(NSString *)animationID
                                    finished:(BOOL)finished
                                     context:(void *)context
{
  AWLogDebug(@"animation %@ finished %@ context %x", animationID, finished? @"YES":@"NO", context);
  UIView *adViewToRemove = (UIView *)context;
  [adViewToRemove removeFromSuperview];
  [adViewToRemove release]; // was retained before beginAnimations
  lastAdapter.adWhirlDelegate = nil, lastAdapter.adWhirlView = nil;
  self.lastAdapter = nil;
  if ([delegate respondsToSelector:@selector(adWhirlDidAnimateToNewAdIn:)]) {
    [delegate adWhirlDidAnimateToNewAdIn:self];
  }
}

- (void)adapter:(AdWhirlAdNetworkAdapter *)adapter didReceiveAdView:(UIView *)view {
  UIView *currAdView = [self viewWithTag:kAdWhirlViewAdSubViewTag];
  AWLogDebug(@"Received ad from adapter (nid %@)", adapter.networkConfig.nid);
  [self transitionToView:view];
  requesting = NO;
  if ([adapter shouldSendExMetric]) {
    [self notifyExImpression:adapter.networkConfig.nid netType:adapter.networkConfig.networkType];
  }
  if ([delegate respondsToSelector:@selector(adWhirlDidReceiveAd:)]) {
    [delegate adWhirlDidReceiveAd:self];
  }
  if (currAdView != view) {
    // for ad networks that refreshes itself and calls the call back function,
    // don't call schedule next ad refresh as that may push AdWhirl refresh
    // back again and again.
    [self scheduleNextAdRefresh];
  }
}

- (void)adapter:(AdWhirlAdNetworkAdapter *)adapter didFailAd:(NSError *)error {
  AWLogDebug(@"Failed to receive ad from adapter (nid %@): %@",
             adapter.networkConfig.nid, error);
  requesting = NO;
  BOOL doNotify = [delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)];
  if ([prioritizedAdNetworks count] == 0) {
    // we have run out of networks to try and need to error out.
    // just call the delegate's failed method.
    [lastError release];
    lastError = [[AdWhirlError alloc] initWithCode:AdWhirlAdRequestNoMoreAdNetworks
                                       description:@"No more ad networks for roll over"];
    if (doNotify) {
      [[self retain] autorelease]; // to prevent self being freed before this returns
      [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
    }
    return;
  }
  [lastError release], lastError = nil; // no error really, just need to roll over
  if (doNotify) {
    [delegate adWhirlDidFailToReceiveAd:self usingBackup:YES];
  }

  // keep trying, but don't call makeAdRequest or rollOver directly. Let
  // the current stack finish
  [self performSelectorOnMainThread:@selector(rollOver) withObject:nil waitUntilDone:NO];
}

- (void)adapterDidFinishAdRequest:(AdWhirlAdNetworkAdapter *)adapter {
  // view is supplied via other mechanism (e.g. Generic Notification)
  requesting = NO;
  if ([adapter shouldSendExMetric]) {
    [self notifyExImpression:adapter.networkConfig.nid netType:adapter.networkConfig.networkType];
  }
  [self scheduleNextAdRefresh];
}

- (void)metricPing:(NSURL *)endPointBaseURL nid:(NSString *)nid netType:(AdWhirlAdNetworkType)type {
  NSString *query = [NSString stringWithFormat:@"?appid=%@&nid=%@&type=%d&country_code=%@&appver=%d&client=1",
                     [delegate adWhirlApplicationKey],
                     nid,
                     type,
                     [[NSLocale currentLocale] localeIdentifier],
                     kAdWhirlAppVer];
  NSURL *metURL = [NSURL URLWithString:query
                         relativeToURL:endPointBaseURL];
  AWLogDebug(@"Sending metric ping to %@", metURL);
  NSURLRequest *metRequest = [NSURLRequest requestWithURL:metURL];
  [NSURLConnection connectionWithRequest:metRequest
                                delegate:nil]; // fire and forget
}

- (void)notifyExImpression:(NSString *)nid netType:(AdWhirlAdNetworkType)type {
  NSURL *baseURL = nil;
  if ([delegate respondsToSelector:@selector(adWhirlImpMetricURL)]) {
    baseURL = [delegate adWhirlImpMetricURL];
  }
  if (baseURL == nil) {
    baseURL = [NSURL URLWithString:kAdWhirlDefaultImpMetricURL];
  }
  [self metricPing:baseURL nid:nid netType:type];
}

- (void)notifyExClick:(NSString *)nid netType:(AdWhirlAdNetworkType)type {
  NSURL *baseURL = nil;
  if ([delegate respondsToSelector:@selector(adWhirlClickMetricURL)]) {
    baseURL = [delegate adWhirlClickMetricURL];
  }
  if (baseURL == nil) {
    baseURL = [NSURL URLWithString:kAdWhirlDefaultClickMetricURL];
  }
  [self metricPing:baseURL nid:nid netType:type];
}

- (BOOL)canRefresh {
  return !ignoreAutoRefreshTimer && !appInactive && !showingModalView;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  delegate = nil;
  [config removeDelegate:self];
  [config release], config = nil;
  [prioritizedAdNetworks release], prioritizedAdNetworks = nil;
  totalPercent = 0.0;
  requesting = NO;
  currAdapter.adWhirlDelegate = nil, currAdapter.adWhirlView = nil;
  [currAdapter release], currAdapter = nil;
  lastAdapter.adWhirlDelegate = nil, lastAdapter.adWhirlView = nil;
  [lastAdapter release], lastAdapter = nil;
  [lastRequestTime release], lastRequestTime = nil;
  [refreshTimer release], refreshTimer = nil;
  [lastError release], lastError = nil;

  [super dealloc];
}

#pragma mark UIView touch methods

- (BOOL)_isEventATouch30:(UIEvent *)event {
  if ([event respondsToSelector:@selector(type)]) {
    return event.type == UIEventTypeTouches;
  }
  return YES; // UIEvent in 2.2.1 has no type property, so assume yes.
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
  BOOL itsInside = [super pointInside:point withEvent:event];
  if (itsInside && currAdapter != nil && lastNotifyAdapter != currAdapter
      && [self _isEventATouch30:event]
      && [currAdapter shouldSendExMetric]) {
    self.showingModalView = YES; // prevent reload
    lastNotifyAdapter = currAdapter;
    [self notifyExClick:currAdapter.networkConfig.nid netType:currAdapter.networkConfig.networkType];
  }
  return itsInside;
}

#pragma mark UIView methods

- (void)willMoveToSuperview:(UIView *)newSuperview {
  if (newSuperview == nil) {
    [refreshTimer invalidate];
    self.refreshTimer = nil;
  }
}

#pragma mark AdWhirlConfigDelegate methods

+ (NSURL *)adWhirlConfigURL {
  if (classAdWhirlDelegateForConfig != nil
      && [classAdWhirlDelegateForConfig respondsToSelector:@selector(adWhirlConfigURL)]) {
    return [classAdWhirlDelegateForConfig adWhirlConfigURL];
  }
  return nil;
}

+ (void)adWhirlConfigDidReceiveConfig:(AdWhirlConfig *)config {
  AWLogDebug(@"Fetched Ad network config: %@", config);
  if (classAdWhirlDelegateForConfig != nil
      && [classAdWhirlDelegateForConfig respondsToSelector:@selector(adWhirlDidReceiveConfig:)]) {
    [classAdWhirlDelegateForConfig adWhirlDidReceiveConfig:nil];
  }
  classAdWhirlDelegateForConfig = nil;
}

+ (void)adWhirlConfigDidFail:(AdWhirlConfig *)cfg error:(NSError *)error {
  AWLogError(@"Failed pre-fetching AdWhirl config: %@", error);
  classAdWhirlDelegateForConfig = nil;
}

- (void)adWhirlConfigDidReceiveConfig:(AdWhirlConfig *)cfg {
  if (self.config != cfg) {
    AWLogWarn(@"AdWhirlView: getting adWhirlConfigDidReceiveConfig callback from unknown AdWhirlConfig object");
    return;
  }
  // now decide which ad network to use and fetch ad
  AWLogDebug(@"Fetched Ad network config: %@", cfg);
  if ([delegate respondsToSelector:@selector(adWhirlDidReceiveConfig:)]) {
    [delegate adWhirlDidReceiveConfig:self];
  }
  @synchronized(self) {
    if (cfg.adsAreOff) {
      if ([delegate respondsToSelector:@selector(adWhirlReceivedNotificationAdsAreOff:)]) {
        [[self retain] autorelease]; // to prevent self being freed before this returns
        [delegate adWhirlReceivedNotificationAdsAreOff:self];
      }
      return;
    }
    [self prepAdNetworks];
  }
  [self makeFirstAdRequest];
}

- (void)adWhirlConfigDidFail:(AdWhirlConfig *)cfg error:(NSError *)error {
  if (self.config != nil && self.config != cfg) {
    // self.config could be nil if this is called before init is finished
    AWLogWarn(@"AdWhirlView: getting adWhirlConfigDidFail callback from unknown AdWhirlConfig object");
    return;
  }
  self.config = cfg;
  AWLogError(@"Failed fetching AdWhirl config: %@", error);
  [lastError release];
  lastError = [error retain];
  if ([delegate respondsToSelector:@selector(adWhirlDidFailToReceiveAd:usingBackup:)]) {
    [[self retain] autorelease]; // to prevent self being freed before this returns
    [delegate adWhirlDidFailToReceiveAd:self usingBackup:NO];
  }
}

- (NSURL *)adWhirlConfigURL {
  if ([delegate respondsToSelector:@selector(adWhirlConfigURL)]) {
    return [delegate adWhirlConfigURL];
  }
  return nil;
}


#pragma mark active status notification methods

- (void)resignActive:(NSNotification *)notification {
  AWLogDebug(@"App become inactive, AdWhirlView will stop requesting ads");
  appInactive = YES;
}

- (void)becomeActive:(NSNotification *)notification {
  AWLogDebug(@"App become active, AdWhirlView will resume requesting ads");
  appInactive = NO;
}


@end

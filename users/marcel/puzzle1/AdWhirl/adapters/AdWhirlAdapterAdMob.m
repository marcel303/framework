/*
 
 AdWhirlAdapterAdmob.m
 
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

#import "AdWhirlAdapterAdMob.h"
#import "AdWhirlAdNetworkConfig.h"
#import "AdWhirlView.h"
#import "AdMobView.h"
#import "AdWhirlLog.h"
#import "AdWhirlAdNetworkAdapter+Helpers.h"
#import "AdWhirlAdNetworkRegistry.h"

@implementation AdWhirlAdapterAdMob

+ (AdWhirlAdNetworkType)networkType {
  return AdWhirlAdNetworkTypeAdMob;
}

+ (void)load {
  [[AdWhirlAdNetworkRegistry sharedRegistry] registerClass:self];
}

- (void)getAd {
  AdMobView *adMobView = [AdMobView requestAdWithDelegate:self];
  self.adNetworkView = adMobView;
}

- (void)dealloc {
  AdMobView *adMobView = (AdMobView *)self.adNetworkView;
  if (adMobView != nil) {
    adMobView.delegate = nil;
  }
  [super dealloc];
}

#pragma mark AdMobDelegate required methods

- (NSString *)publisherIdForAd:(AdMobView *)adView {
  if ([adWhirlDelegate respondsToSelector:@selector(admobPublisherID)]) {
    return [adWhirlDelegate admobPublisherID];
  }
  return networkConfig.pubId;
}

- (UIViewController *)currentViewControllerForAd:(AdMobView *)adView {
  return [adWhirlDelegate viewControllerForPresentingModalView];
}

#pragma mark AdMobDelegate notification methods

- (void)didReceiveAd:(AdMobView *)adView {
  [adWhirlView adapter:self didReceiveAdView:adView];
}

- (void)didFailToReceiveAd:(AdMobView *)adView {
  [adWhirlView adapter:self didFailAd:nil];
}

- (void)willPresentFullScreenModalFromAd:(AdMobView *)adView {
  [self helperNotifyDelegateOfFullScreenModal];
}

- (void)didDismissFullScreenModalFromAd:(AdMobView *)adView {
  [self helperNotifyDelegateOfFullScreenModalDismissal];
}

#pragma mark AdMobDelegate config methods
- (UIColor *)adBackgroundColorForAd:(AdMobView *)adView {
  return [self helperBackgroundColorToUse];
}

- (UIColor *)primaryTextColorForAd:(AdMobView *)adView {
  return [self helperTextColorToUse];
}

- (UIColor *)secondaryTextColorForAd:(AdMobView *)adView {
  return [self helperSecondaryTextColorToUse];
}

- (NSArray *)testDevices {
  if ([adWhirlDelegate respondsToSelector:@selector(adWhirlTestMode)]
      && [adWhirlDelegate adWhirlTestMode]) {
    return [NSArray arrayWithObjects:
            ADMOB_SIMULATOR_ID,                             // Simulator
            nil];
  }
  return nil;
}

#pragma mark AdMobDelegate optional config methods

- (BOOL)respondsToSelector:(SEL)selector {
  if ((selector == @selector(locationLatitude)
       || selector == @selector(locationLongitude)
       || selector == @selector(locationTimestamp))
      && (!adWhirlConfig.locationOn
          || ![adWhirlDelegate respondsToSelector:@selector(locationInfo)])) {
    return NO;
  }
  else if (selector == @selector(postalCode)
           && ![adWhirlDelegate respondsToSelector:@selector(postalCode)]) {
    return NO;
  }
  else if (selector == @selector(areaCode)
           && ![adWhirlDelegate respondsToSelector:@selector(areaCode)]) {
    return NO;
  }
  else if (selector == @selector(dateOfBirth)
           && ![adWhirlDelegate respondsToSelector:@selector(dateOfBirth)]) {
    return NO;
  }
  else if (selector == @selector(gender)
           && ![adWhirlDelegate respondsToSelector:@selector(gender)]) {
    return NO;
  }
  else if (selector == @selector(keywords)
           && ![adWhirlDelegate respondsToSelector:@selector(keywords)]) {
    return NO;
  }
  return [super respondsToSelector:selector];
}

- (double)locationLatitude {
  return [adWhirlDelegate locationInfo].coordinate.latitude;
}

- (double)locationLongitude {
  return [adWhirlDelegate locationInfo].coordinate.longitude;
}

- (NSDate *)locationTimestamp {
  return [adWhirlDelegate locationInfo].timestamp;
}

- (NSString *)postalCode {
  return [adWhirlDelegate postalCode];
}

- (NSString *)areaCode {
  return [adWhirlDelegate areaCode];
}

- (NSDate *)dateOfBirth {
  return [adWhirlDelegate dateOfBirth];
}

- (NSString *)gender {
  return [adWhirlDelegate gender];
}

- (NSString *)keywords {
  return [adWhirlDelegate keywords];
}

@end

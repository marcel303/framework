/**
 * AdMobInterstitialDelegateProtocol.h
 * AdMob iPhone SDK publisher code.
 *
 * Defines the AdMobInterstitialDelegate protocol.  It is notified of interstitial-related
 * events such as it being shown and dismissed.
 */
#import <UIKit/UIKit.h>
@class AdMobInterstitialAd;

@protocol AdMobInterstitialDelegate<NSObject>

@required

@optional

// Sent when an interstitial ad request succefully returned an ad.  At the next transition
// point in your application call [ad show] to display the interstitial.
- (void)didReceiveInterstitial:(AdMobInterstitialAd *)ad;

// Sent when an interstitial ad request completed without an interstitial to show.  This is
// common since interstitials are shown sparingly to users.
- (void)didFailToReceiveInterstitial:(AdMobInterstitialAd *)ad;

// Sent just before presenting an interstitial and immediately after you called [ad show].
// After this method finishes the interstitial will animate onto the screen.  Use this
// opportunity to save the state of your application in case the user leaves while the
// interstitial is on screen (e.g. to visit the App Store from a link on the interstitial).
- (void)interstitialWillAppear:(AdMobInterstitialAd *)ad;

// Sent when the interstitial has fully taken over the screen.  Use this opportunity to stop
// animations, time-sensitive interactions, etc.
- (void)interstitialDidAppear:(AdMobInterstitialAd *)ad;

// Sent just after dismissing a full screen view.  After this method finishes the interstitial
// will animate off the screen.  Use this opportunity to setup the screen the user should see
// next.  If it is the same screen as before the interstitial then nothing is required.  If it
// is a new level then you should replace your previous view controller with the new level.
- (void)interstitialWillDisappear:(AdMobInterstitialAd *)ad;

// Sent when the interstitial has fully animated off the screen and gives control back to
// your application.  Use this opportunity to start animations, time-sensitive interactions,
// etc.
- (void)interstitialDidDisappear:(AdMobInterstitialAd *)ad;

@end

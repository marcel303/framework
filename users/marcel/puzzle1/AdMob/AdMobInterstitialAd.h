/**
 * AdMobInterstitialAd.h
 * AdMob iPhone SDK publisher code.
 *
 * The entry point for requesting interstitial (i.e. full-screen) AdMob ads.  Interstitial
 * ads are to be inserted between screens in the application's flow (e.g. 
 * on start or between game levels).
 *
 * Special note: when using AdMob Interstitial Ads in your application, please be 
 * sure to instantiate a new MPMoviePlayerController each time you play your own 
 * application's videos.  The MPMoviePlayerController's ContentURL changes if a different 
 * MPMoviePlayerController is instantiated with a different URL, which may occur if
 * AdMob serves a video interstitial ad.
 */
#import <UIKit/UIKit.h>
@protocol AdMobDelegate, AdMobInterstitialDelegate;

typedef enum {
  AdMobInterstitialEventAppOpen,       // From UIApplicationDelegate applicationDidFinishLaunching
  AdMobInterstitialEventScreenChange,  // New level, whenever UIViewController presentModalViewController is called, etc.
  AdMobInterstitialEventPreRoll,       // Just before a movie is going to be played (See Special note above)
  AdMobInterstitialEventPostRoll,      // Just after a movie has been played
  AdMobInterstitialEventOther,         // An application event not described above
} AdMobInterstitialEvent;

@interface AdMobInterstitialAd : NSObject

/**
 * Initiates an interstitial ad request.  The interstitialDelegate is alerted when the
 * request completes.  The event provides a hint about the context in which the
 * interstitial will be shown so that the most suitable interstitial can be chosen.
 *
 * This method should only be called from a run loop in default run loop mode.
 * If you don't know what that means, you're probably ok. If in doubt, check
 * whether ([[NSRunLoop currentRunLoop] currentMode] == NSDefaultRunLoopMode).
 *
 * This method does not return a retained object for you.  You must retain the 
 * returned object if you mean to keep it for later use.
 *
 * Note that if calling this from within applicationDidFinishLaunching, the network
 * calls for the ad request will not start executing until the applicationDidFinishLaunching
 * call returns.
 * 
 * Only one interstitial request is allowed at a time.  If you request an interstitial while
 * a request is already in progress, the SDK will return nil.
 */
+ (AdMobInterstitialAd *)requestInterstitialAt:(AdMobInterstitialEvent)event
                                      delegate:(id<AdMobDelegate>)delegate
                          interstitialDelegate:(id<AdMobInterstitialDelegate>)interstitialDelegate;

/**
 * Causes the ad to appear over the entire screen.  The delegate is alerted
 * when the user dismisses the ad.  If you are running a pre-roll ad, please see
 * special note above.
 */
- (void)show;

/**
 * Returns YES if the interstitial is ready to be displayed.  Never call the -show
 * method unless this is YES.  The delegate's -didReceiveInterstitial: will be called
 * immediately after this switches from NO to YES.
 */
@property(readonly) BOOL ready;

/**
 * At what point in the application the requested interstitial can be shown.  This is
 * the same value used to request the interstitial.
 */
@property(readonly) AdMobInterstitialEvent applicationEvent;

/**
 * This property is exposed so that the delegate can be cleared if it is going to 
 * be dealloc'ed.  This ensures that the AdMob SDK will never make a call to a 
 * deallocated instance.
 */
@property (retain)id<AdMobDelegate> delegate;

/**
 * This property is exposed so that the interstitial delegate can be cleared if it 
 * is going to be dealloc'ed.  This ensures that the AdMob SDK will never make a call 
 * to a deallocated instance.
 */
@property (retain)id<AdMobInterstitialDelegate> interstitialDelegate;


@end

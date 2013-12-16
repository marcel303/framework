/**
 * AdViewController.h
 * AdMob iPhone SDK publisher code.
 *
 * Helper file for using IB with AdMob ads. To integrate an ad using Interface Builder
 * and this code, add an NSObject and set its type to AdViewController. Then drag a UIView
 * where you want the ad to display; its dimensions must be 320x48. Set the
 * AdViewController's view outlet to that UIView. In this file's implementation, check that
 * your publisher id is set correctly. Build and run. If you have questions, look at the
 * sample IB-based project.
 *
 * Note that top level objects in nibs other than MainWindow.xib in Cocoa Touch are autoreleased, not retained like in OS X.
 * Be sure to use [self retain] in -awakeFromNib when part of a custom nib (as in this example).
 * See http://developer.apple.com/releasenotes/DeveloperTools/RN-InterfaceBuilder/index.html#//apple_ref/doc/uid/TP40001016-SW5
 */

#import <UIKit/UIKit.h>
#import "AdMobDelegateProtocol.h";
@class AdMobView;

@interface AdViewController : UIViewController<AdMobDelegate> {

  // The actual ad, intentially _not_ an IBOutlet; instead, assign this controller's
  // view outlet (defined by UIViewController) to a UIView in Interface Builder
  // to indicate where the ad should be placed.
  AdMobView *adMobAd;

  UIViewController *currentViewController;

}

// The currentViewController used by the AdMobView to display modal views, e.g. when
// users tap on ads. Set to a view controller higher up in the view controller
// hierarchy, such as the navigation controller or split view controller.
@property (nonatomic,assign) IBOutlet UIViewController *currentViewController;

@end

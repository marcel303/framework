#import <iAd/iAd.h>
#import <UIKit/UIKit.h>
#import "AlbumDelegate.h"
#import "puzzle.h"

@interface vwMain : UIViewController <UIActionSheetDelegate, UIImagePickerControllerDelegate, UINavigationControllerDelegate, UIAlertViewDelegate, ADBannerViewDelegate, AlbumDelegate>
{
	UISegmentedControl* difficulty;
	ADBannerView* banner;
	UIActionSheet* asPictureSource;
	int asPictureSource_PhotoAlbum;
	int asPictureSource_Camera;
	int asPictureSource_Preset;
	UIImagePickerController* imagePickerAlbum;
	UIImagePickerController* imagePickerCamera;
	UIViewController* imagePickerPreset;
	PuzzleMode mode;
	UIAlertView* infoAlert;
	UIAlertView* shareAlert;
	UIAlertView* shareInfoAlert;
	UIAlertView* rateAlert;
	UIAlertView* feintAlertEnable;
	UIAlertView* feintAlertOpen;
	UIAlertView* feintInfoAlert;
}

@property (nonatomic, retain) IBOutlet UISegmentedControl* difficulty;
@property (nonatomic, retain) IBOutlet ADBannerView* banner;

-(IBAction)handleBeginFree:(id)sender;
-(IBAction)handleBeginSwitch:(id)sender;
-(IBAction)handleShare:(id)sender;
-(void)handleShare2:(id)sender;
-(IBAction)handleInfo:(id)sender;
-(IBAction)handleOpenfeint:(id)sender;
-(IBAction)handleRate:(id)sender;
-(void)handleAcquirePhotoAlbum;
-(void)handleAcquirePhotoCamera;
-(void)handleAcquirePreset;

+(UIImage*)processImage:(UIImage*)image size:(Vec2I)size;
-(void)selectImage:(UIImage*)image;

// UIActionSheetDelegate
-(void)actionSheet:(UIActionSheet*)actionSheet clickedButtonAtIndex:(NSInteger)index;

// UIAlertViewDelegate
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

// UIImagePickerControllerDelegate
-(void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info;
-(void)imagePickerControllerDidCancel:(UIImagePickerController *)picker;

// AlbumDelegate
-(void)albumImageSelected:(UIImage*)image;

// ADBannerViewDelegate
-(void)bannerViewDidLoadAd:(ADBannerView *)banner;
-(void)bannerView:(ADBannerView *)banner didFailToReceiveAdWithError:(NSError *)error;

@end

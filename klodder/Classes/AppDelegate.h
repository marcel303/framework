#import <UIKit/UIKit.h>
#import "ChangeType.h"
#import "ColorPickerState.h"
#import "FBConnect/FBConnect.h"
#import "klodder_forward.h"
#import "klodder_forward_objc.h"
#import "Tool.h"
#import "ViewBase.h"

#ifdef IPAD
#define IMG(x) x ".png"
#else
#define IMG(x) x
#endif

#ifdef IPAD
extern NSString* cancelTitle;
#else
extern NSString* cancelTitle;
#endif

class BrushSettings;

@interface AppDelegate : NSObject <UIApplicationDelegate>
{
	@private
	
    UIWindow* window;
	
	NSDictionary* addresses;
	
	@public
	
	UINavigationController* rootController;
	
#if BUILD_FACEBOOK
	FacebookState* facebookState;
#endif
#if BUILD_FLICKR
	FlickrState* flickrState;
#endif
	
	View_EditingMgr* vcEditing;
	View_PictureGalleryMgr* vcPictureGallery;
	
	BrushSettings* brushSettings;
	PickerState* colorPickerState;
	
	Application* mApplication;
	
	UIViewController* mActiveView;
}

@property (nonatomic, retain) IBOutlet UIWindow* window;

@property (assign) Application* mApplication;

#if BUILD_FACEBOOK
@property (nonatomic, assign) FacebookState* facebookState;
#endif
#if BUILD_FLICKR
@property (nonatomic, assign) FlickrState* flickrState;
#endif

@property (nonatomic, retain) UINavigationController* rootController;

@property (nonatomic, retain) View_EditingMgr* vcEditing;
@property (nonatomic, retain) View_PictureGalleryMgr* vcPictureGallery;
@property (nonatomic, assign) BrushSettings* brushSettings;
@property (nonatomic, assign) PickerState* colorPickerState;

@property (nonatomic, readonly, assign) NSDictionary* addresses;

-(void)handleChange:(ChangeType)type;
-(void)show:(UIViewController*)vc animated:(BOOL)animated;
-(void)show:(UIViewController*)vc;
-(void)hide;
-(void)hideWithAnimation:(BOOL)animated;

-(void)newApplication;

-(void)setBrushSettings:(BrushSettings*)brushSettings;
-(void)applyBrushSettings:(ToolType)toolType;
-(void)loadCurrentBrushSettings;
-(void)saveBrushSettings;
-(void)applyColor;

-(void)handleAddressesResolved:(NSNotification*)notification;

+(UIImage*)loadImageResource:(NSString*)name;
+(UIImage*)uiImageResize:(UIImage*)image size:(Vec2I)size;
+(UIImage*)macImageToUiImage:(const MacImage*)image size:(Vec2I)size;
+(UIImage*)macImageToUiImage:(const MacImage*)image;
+(MacImage*)uiImageToMacImage:(UIImage*)image size:(Vec2I)size;
+(MacImage*)uiImageToMacImage:(UIImage*)image;

+(UIImage*)flipY:(UIImage*)image;
-(UIImage*)drawingToImage;
+(NSArray*)scanForPictures:(const char*)path;

+(void)applyWatermark:(MacImage*)dst;

+(float)displayScale;

-(void)memoryReport;

@end

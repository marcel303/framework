#import <UIKit/UIView.h>
#import "ImageId.h"
#import "klodder_forward_objc.h"
#import "ViewControllerBase.h"

@interface View_PictureGalleryMgr : ViewControllerBase <UIAlertViewDelegate>
{
	UIAlertView* overwritePictureAlert;
	ImageId overwriteId;
	ImageId lastImageId;
}

-(id)initWithApp:(AppDelegate*)_app;

-(void)handleNew;
-(void)beginNew:(ImageId)imageId;
-(void)pictureSelected:(id)object;
-(void)handleServeHttp;
-(void)handleAbout;
-(void)handleHelp;
-(void)handleDebug;

// UIAlertViewDelegate
-(void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

@end

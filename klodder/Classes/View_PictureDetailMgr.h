#pragma once

#import <MessageUI/MessageUI.h>
#import <UIKit/UIView.h>
#import "ViewControllerBase.h"

enum ImageEncoding
{
	ImageEncoding_Png,
	ImageEncoding_Jpg,
	ImageEncoding_Psd
};

@interface View_PictureDetailMgr : ViewControllerBase <UIAlertViewDelegate, UIActionSheetDelegate, MFMailComposeViewControllerDelegate>
{
	ImageId imageId;
	
	UIActionSheet* asMore;
	int asMoreDuplicate;
	int asMoreDelete;
	
	UIActionSheet* asShare;
	int asSharePhotoAlbum;
	int asShareEmail;
	int asShareEmailPsd;
	int asShareFlickr;
	int asShareFacebook;
		
	UIAlertView* deleteAlert;
	UIAlertView* flickrLoginAlert;
	UIAlertView* flickrUploadAlert;
	UIAlertView* facebookUploadAlert;
	
	bool loginAndUpload;
}

-(id)initWithApp:(AppDelegate*)_app;
-(void)setImageId:(ImageId)imageId;
-(void)handleEdit;
-(void)handleMore;
-(void)handleDuplicate;
-(void)handleDelete;
-(void)handleReplay;
-(void)handleShare;
-(void)handleValidateCommandStream;

-(NSData*)getImageData:(ImageEncoding)encoding;
-(void)sharePhotoAlbum;
-(void)shareEmail;
-(void)shareEmailPsd;
-(void)shareFacebook;
-(void)shareFlickr;

-(void)logIntoFlickrPrompt;
-(void)handleFacebookLogin;
#if BUILD_FLICKR
-(void)uploadToFlickr;
#endif
#if BUILD_FACEBOOK
-(void)uploadToFacebook;
#endif

// UIAlertViewDelegate
-(void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

// UIActionSheetDelegate
-(void)actionSheet:(UIActionSheet*)actionSheet clickedButtonAtIndex:(NSInteger)index;

// MFMailComposeViewControllerDelegate
-(void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error;

@end

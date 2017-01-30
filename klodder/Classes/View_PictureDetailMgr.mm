#import "AppDelegate.h"
#import "Application.h"
#import "Deployment.h"
#import "ExceptionLoggerObjC.h"
#import "FacebookState.h"
#import "FileStream.h"
#import "FlickrState.h"
#import "Image.h"
#import "ImageLoader_Photoshop.h"
#import "KlodderSystem.h"
#import "Log.h"
#import "ObjectiveFlickr.h"
#import "View_EditingMgr.h"
#import "View_PictureDetail.h"
#import "View_PictureDetailMgr.h"
#import "View_ReplayMgr.h"
#import "View_ToolSelectMgr.h"

@implementation View_PictureDetailMgr

-(id)initWithApp:(AppDelegate*)_app
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithApp:_app])
	{
		self.title = @"Preview";
		
		[self setFullScreenLayout];
		
		/*
		UIBarButtonSystemItemAction
		UIBarButtonSystemItemUndo
		UIBarButtonSystemItemPlay
		UIBarButtonSystemItemRedo
		UIBarButtonSystemItemTrash
		*/

//		UIBarButtonItem* item_Delete = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemTrash target:self action:@selector(handleDelete)] autorelease];
		UIBarButtonItem* item_More = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemOrganize target:self action:@selector(handleMore)] autorelease];
		UIBarButtonItem* item_Replay = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemPlay target:self action:@selector(handleReplay)] autorelease];
		UIBarButtonItem* item_Share = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_share")] style:UIBarButtonItemStylePlain target:self action:@selector(handleShare)] autorelease];
		UIBarButtonItem* item_Space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];
		UIBarButtonItem* item_DbgvalidateCommandStream = nil;
#ifdef DEBUG
		item_DbgvalidateCommandStream = [[[UIBarButtonItem alloc] initWithTitle:@"V" style:UIBarButtonItemStylePlain target:self action:@selector(handleValidateCommandStream)] autorelease];
#endif
//		[self setToolbarItems:[NSArray arrayWithObjects:item_Delete, item_Space, item_Replay, item_Space, item_Share, item_DbgvalidateCommandStream, nil]];
		[self setToolbarItems:[NSArray arrayWithObjects:item_More, item_Space, item_Replay, item_Space, item_Share, item_DbgvalidateCommandStream, nil]];
		
		UIBarButtonItem* item_Edit = [[[UIBarButtonItem alloc] initWithTitle:@"Edit" style:UIBarButtonItemStylePlain target:self action:@selector(handleEdit)] autorelease];
		self.navigationItem.rightBarButtonItem = item_Edit;
			
		asMore = [[UIActionSheet alloc] initWithTitle:@"Organize" delegate:self cancelButtonTitle:cancelTitle destructiveButtonTitle:nil otherButtonTitles:nil];
		[asMore setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
		asMoreDuplicate = [asMore addButtonWithTitle:@"Duplicate"];
		asMoreDelete = [asMore addButtonWithTitle:@"Delete"];
		
		asShare = [[UIActionSheet alloc] initWithTitle:@"Share" delegate:self cancelButtonTitle:cancelTitle destructiveButtonTitle:nil otherButtonTitles:nil];
		[asShare setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
		asSharePhotoAlbum = [asShare addButtonWithTitle:@"Photo album"];
		asShareEmail = [asShare addButtonWithTitle:@"E-Mail (PNG)"];
		asShareEmailPsd = [asShare addButtonWithTitle:@"E-Mail (PSD)"];
		asShareFlickr = [asShare addButtonWithTitle:@"Flickr"];
		asShareFacebook = [asShare addButtonWithTitle:@"Facebook"];
		
		deleteAlert = [[UIAlertView alloc] initWithTitle:NS(Deployment::PictureDeleteTitle) message:NS(Deployment::PictureDeleteText) delegate:self cancelButtonTitle:@"No" otherButtonTitles:@"Yes", nil];
		flickrLoginAlert = [[UIAlertView alloc] initWithTitle:NS(Deployment::FlickrLoginPromptTitle) message:NS(Deployment::FlickrLoginPromptText) delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"OK", nil];
		flickrUploadAlert = [[UIAlertView alloc] initWithTitle:NS(Deployment::FlickrUploadPromptTitle) message:NS(Deployment::FlickrUploadPromptText) delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"OK", nil];
		facebookUploadAlert = [[UIAlertView alloc] initWithTitle:NS(Deployment::FacebookUploadPromptTitle) message:NS(Deployment::FacebookUploadPromptText) delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"OK", nil];
		
		loginAndUpload = false;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setImageId:(ImageId)_imageId
{
	imageId = _imageId;
}

-(void)handleEdit
{
	HandleExceptionObjcBegin();
	
	[app newApplication];
	
	// load image
	
	app.mApplication->LoadImage(imageId);
	app.mApplication->ImageActivate(true);
	app.mApplication->UndoEnabled_set(true);
	
	[app.vcEditing.view setNeedsDisplay];
	
	// transition to editing view
	
	[app.vcEditing newPainting];
	
	[app show:app.vcEditing animated:FALSE];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleMore
{
	HandleExceptionObjcBegin();
	
	[asMore showInView:self.view];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleDuplicate
{
	HandleExceptionObjcBegin();
	
	[app newApplication];
	
	app.mApplication->ImageDuplicate(imageId, app.mApplication->AllocateImageId());
	
	[app hide];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleDelete
{
	HandleExceptionObjcBegin();
	
	[deleteAlert show];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleReplay
{
	HandleExceptionObjcBegin();
	
	View_ReplayMgr* subController = [[[View_ReplayMgr alloc] initWithApp:app imageId:imageId] autorelease];
	[app.rootController pushViewController:subController animated:FALSE];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleShare
{
	HandleExceptionObjcBegin();
	
	[asShare showInView:self.view];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleValidateCommandStream
{
	HandleExceptionObjcBegin();
	
	try
	{
		Application::DBG_ValidateCommandStream(imageId);
	}
	catch (std::exception& e)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Error" message:[NSString stringWithCString:e.what() encoding:NSASCIIStringEncoding] delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil, nil] autorelease] show];
	}
	
	HandleExceptionObjcEnd(false);
}

-(NSData*)getImageData:(ImageEncoding)encoding
{
	if (encoding == ImageEncoding_Png || encoding == ImageEncoding_Jpg)
	{
		MacImage imagex;
		
		Application::LoadImagePreview(imageId, imagex);
		
		if (gSystem.IsLiteVersion())
			[AppDelegate applyWatermark:&imagex];
		
		// Flip image
		
		MacImage* image0 = imagex.FlipY();
		
		// Get CG image
		
		CGImageRef cgImage = image0->Image_get();
		
		// Convert to UIImage
		
		UIImage* image = [[UIImage alloc] initWithCGImage:cgImage];
		
		CGImageRelease(cgImage);
		
		// Get encoded data
		
		NSData* data = nil;
		
		if (encoding == ImageEncoding_Png)
			data = UIImagePNGRepresentation(image);
		else if (encoding == ImageEncoding_Jpg)
			data = UIImageJPEGRepresentation(image, 1.0f);
		else
			throw ExceptionVA("unknown image encoding: %d", (int)encoding);
		
		[image release];
		
		delete image0;
		
		return data;
	}
	else if (encoding == ImageEncoding_Psd)
	{
		DBG_PrintAllocState();
		
		// load image description
		
		kdImageDescription description = Application::LoadImageDescription(imageId);
		
		// load image data
		
		Image** images = new Image*[description.layerCount];
		
		for (int i = 0; i < description.layerCount; ++i)
		{
			const int index = description.layerOrder[i];
			
			const std::string fileName = description.dataLayerFile[index];
			
			MacImage temp;
			
			Application::LoadImageDataLayer(fileName, &temp);
			
			if (gSystem.IsLiteVersion())
				[AppDelegate applyWatermark:&temp];
			
			// convert MacImage to Image
			
			Image* image = new Image();
			
			image->SetSize(temp.Sx_get(), temp.Sy_get());
			
			for (int y = 0; y < temp.Sy_get(); ++y)
			{
				const MacRgba* src = temp.Line_get(y);
				ImagePixel* dst = image->GetLine(y);
				
				for (int x = 0; x < temp.Sx_get(); ++x)
				{
					for (int j = 0; j < 4; ++j)
						dst[x].rgba[j] = src[x].rgba[j];
				}
			}
			
			images[i] = image;
		}
		
		// create PSD file
		
		std::string fileName = gSystem.GetDocumentPath("export.psd");
		
		ImageLoader_Photoshop loader;
		
		loader.SaveMultiLayer(images, description.layerCount, fileName);
		
		// free
		
		for (int i = 0; i < description.layerCount; ++i)
			delete images[i];
		delete[] images;
		images = 0;
		
		//
		
		DBG_PrintAllocState();
		
		// create NSData from stream
		
		NSData* result = [NSData dataWithContentsOfFile:[NSString stringWithCString:fileName.c_str() encoding:NSASCIIStringEncoding]];
		
		return result;
	}
	else
	{
		throw ExceptionVA("unknown image encoding: %d", (int)encoding);
	}
}

-(void)sharePhotoAlbum
{
	HandleExceptionObjcBegin();
	
	// save to photo album
	
	NSData* data = [self getImageData:ImageEncoding_Png];
	
	UIImage* image = [[UIImage alloc] initWithData:data];
	
	UIImageWriteToSavedPhotosAlbum(image, nil, nil, 0);
	
	[image release];
	
	[[[[UIAlertView alloc] initWithTitle:NS(Deployment::AlbumSavedTitle) message:NS(Deployment::AlbumSavedText) delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	
	HandleExceptionObjcEnd(false);
}

-(void)shareEmail
{
	HandleExceptionObjcBegin();
	
	if ([MFMailComposeViewController canSendMail])
	{
		MFMailComposeViewController* mailController = [[MFMailComposeViewController alloc] init];
		mailController.mailComposeDelegate = self;
		[mailController setSubject:NS(Deployment::EmailMessageSubject)];
		[mailController setMessageBody:NS(Deployment::EmailMessageBody) isHTML:NO]; 
		[mailController addAttachmentData:[self getImageData:ImageEncoding_Png] mimeType:@"image/png" fileName:NS(Deployment::EmailAttachmentName)];
		[self presentViewController:mailController animated:YES completion:NULL];
		[mailController release];
	}
	else
	{
		[[[[UIAlertView alloc] initWithTitle:NS(Deployment::EmailUnavailableTitle) message:NS(Deployment::EmailUnavailableText) delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)shareEmailPsd
{
	HandleExceptionObjcBegin();
	
#ifdef DEBUG
	[self getImageData:ImageEncoding_Psd];
#endif
	
	if ([MFMailComposeViewController canSendMail])
	{
		MFMailComposeViewController* mailController = [[MFMailComposeViewController alloc] init];
		mailController.mailComposeDelegate = self;
		[mailController setSubject:NS(Deployment::EmailMessageSubject)];
		[mailController setMessageBody:NS(Deployment::EmailMessageBody) isHTML:NO]; 
		[mailController addAttachmentData:[self getImageData:ImageEncoding_Psd] mimeType:@"image/psd" fileName:@"picture.psd"];
		[self presentViewController:mailController animated:YES completion:NULL];
		[mailController release];
	}
	else
	{
		[[[[UIAlertView alloc] initWithTitle:NS(Deployment::EmailUnavailableTitle) message:NS(Deployment::EmailUnavailableText) delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)shareFacebook
{
	HandleExceptionObjcBegin();
	
#if BUILD_FACEBOOK
	// check if logged in to Facebook. if not, present login view
	
	if (![app.facebookState loggedIntoFacebook])
	{
		loginAndUpload = true;
		
		[app.facebookState logIntoFacebookWithDelegate:self action:@selector(handleFacebookLogin)];
	}
	else
	{
		[facebookUploadAlert show];
	}
#endif
	
	HandleExceptionObjcEnd(false);
}

-(void)shareFlickr
{
	HandleExceptionObjcBegin();
	
#if BUILD_FLICKR
	// check if logged in to Flickr. if not, present login view
	
	if (![app.flickrState loggedIntoFlickr])
	{
		[self logIntoFlickrPrompt];
	}
	else
	{
		[flickrUploadAlert show];
	}
#endif
	
	HandleExceptionObjcEnd(false);
}

-(void)logIntoFlickrPrompt
{
	[flickrLoginAlert show];
}

-(void)handleFacebookLogin
{
	if (loginAndUpload)
	{
		[self shareFacebook];
	}
}

#if BUILD_FLICKR

-(void)uploadToFlickr
{
	HandleExceptionObjcBegin();

	// convert drawing to image
	
	NSData* data = [self getImageData:ImageEncoding_Jpg];
	
	// perform upload request
	
	OFFlickrAPIRequest* request = [[OFFlickrAPIRequest alloc] initWithAPIContext:app.flickrState.flickrContext];
	
	[request setDelegate:app.flickrState];
	
	NSDictionary* arguments = [NSDictionary dictionaryWithObjectsAndKeys:@"0", @"is_public", NS(Deployment::FlickrUploadDescription), @"description", nil];
	
	[request uploadImageStream:[NSInputStream inputStreamWithData:data] suggestedFilename:NS(Deployment::FlickrUploadFilename) MIMEType:@"image/jpeg" arguments:arguments];
	
	HandleExceptionObjcEnd(false);
}

#endif

#if BUILD_FACEBOOK

-(void)uploadToFacebook
{
	HandleExceptionObjcBegin();
	
	// convert drawing to image
	
	NSData* data = [self getImageData:ImageEncoding_Png];
	
	// perform upload request
  
	NSDictionary* params = nil;
	
	params = [NSDictionary dictionaryWithObjectsAndKeys:NS(Deployment::FacebookUploadDescription), @"caption", nil];
	
	[[FBRequest requestWithDelegate:app.facebookState] call:@"facebook.photos.upload" params:params dataParam:data];
	
	HandleExceptionObjcEnd(false);
}

#endif

-(void)loadView 
{
	HandleExceptionObjcBegin();
	
	self.view = [[[View_PictureDetail alloc] initWithFrame:[UIScreen mainScreen].applicationFrame controller:self imageId:imageId] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	HandleExceptionObjcBegin();
	
	if (alertView == deleteAlert)
	{
		if (buttonIndex == 0)
		{
			LOG_DBG("delete picture: cancelled", 0);
			
			// nop: cancel button pressed
		}
		
		if (buttonIndex == 1)
		{
			LOG_DBG("delete picture", 0);
			
			// actually delete the picture
			
			app.mApplication->ImageDelete(imageId);
			
			[app hideWithAnimation:TRUE];
		}
	}
	if (alertView == flickrLoginAlert)
	{
		if (buttonIndex == 0)
		{
			LOG_DBG("flickr login: cancelled", 0);
			
			// nop: cancel button pressed
		}
		
		if (buttonIndex == 1)
		{
			LOG_DBG("flickr login", 0);
			
        #if BUILD_FLICKR
			[app.flickrState logIntoFlickr];
        #endif
		}
	}
	if (alertView == flickrUploadAlert)
	{
		if (buttonIndex == 0)
		{
			LOG_DBG("flickr upload: cancelled", 0);
		}
		
		if (buttonIndex == 1)
		{
			LOG_DBG("flickr upload", 0);
			
        #if BUILD_FLICKR
			[self uploadToFlickr];
        #endif
		}
	}
	if (alertView == facebookUploadAlert)
	{
		if (buttonIndex == 0)
		{
			LOG_DBG("facebook upload: cancelled", 0);
		}
		
		if (buttonIndex == 1)
		{
			LOG_DBG("facebook upload", 0);
			
        #if BUILD_FACEBOOK
			[self uploadToFacebook];
        #endif
		}
	}
	
	HandleExceptionObjcEnd(false);
}

// ----------
// UIActionSheetDelegate
// ----------

-(void)actionSheet:(UIActionSheet*)actionSheet clickedButtonAtIndex:(NSInteger)index
{
	HandleExceptionObjcBegin();
	
	if (actionSheet == asMore)
	{
		if (index == asMoreDuplicate)
		{
			[self handleDuplicate];
		}
		else if (index == asMoreDelete)
		{
			[self handleDelete];
		}
		else if (index == actionSheet.cancelButtonIndex)
		{
		}
		else
		{
			Assert(false);
			LOG_ERR("unknown action", 0);
		}
	}
	
	if (actionSheet == asShare)
	{
		if (index == asSharePhotoAlbum)
		{
			[self sharePhotoAlbum];
		}
		else if (index == asShareEmail)
		{
			[self shareEmail];
		}
		else if (index == asShareEmailPsd)
		{
			[self shareEmailPsd];
		}
		else if (index == asShareFlickr)
		{
			[self shareFlickr];
		}
		else if (index == asShareFacebook)
		{
			[self shareFacebook];
		}
		else if (index == actionSheet.cancelButtonIndex)
		{
		}
		else
		{
			Assert(false);
			LOG_ERR("unknown action", 0);
		}
	}
	
	HandleExceptionObjcEnd(false);
}

// ----------
// MFMailComposeViewControllerDelegate
// ----------

-(void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
	HandleExceptionObjcBegin();
	
	if (result == MFMailComposeResultSent || result == MFMailComposeResultSaved)
	{
		[[[[UIAlertView alloc] initWithTitle:NS(Deployment::EmailSentTitle) message:NS(Deployment::EmailSentText) delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	}
	else if (result == MFMailComposeResultCancelled)
	{
		LOG_DBG("mail compose cancelled", 0);
	}
	else
	{
		[[[[UIAlertView alloc] initWithTitle:NS(Deployment::EmailFailedTitle) message:NS(Deployment::EmailFailedText) delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	}
	
	[self dismissViewControllerAnimated:YES completion:NULL];
	
	HandleExceptionObjcEnd(false);
}

// ----------
// FBDialogDelegate
// ----------

-(void)dialog:(FBDialog*)dialog didFailWithError:(NSError*)error 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("FB dialog fail", 0);
	
	NSString* text = [NSString stringWithFormat:@"Error: %@ (%ld)", error.localizedDescription, (long)error.code];
	
	LOG_ERR("reason: %s", [text cStringUsingEncoding:NSASCIIStringEncoding]);
	
	[[[[UIAlertView alloc] initWithTitle:@"Error" message:text delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
//	Assert(!app.mApplication->ImageIsActive_get());
	
	[self setMenuTransparent];
	
	// recreate view
	
	[self loadView];
	
	// todo: update UI
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[super viewWillDisappear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_PictureDetailMgr", 0);
	
	[flickrUploadAlert release];
	[flickrLoginAlert release];
	[deleteAlert release];
	
	[asShare release];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end

#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "Deployment.h"
#import "ExceptionLoggerObjC.h"
//#import "FlickrLoginDialog.h"
#import "KlodderSystem.h"
#import "Log.h"
#import "StreamReader.h"
#import "StringEx.h"
#import "View_AboutMgr.h"
#import "View_EditingMgr.h"
#import "View_HelpMgr.h"
#import "View_HttpServerMgr.h"
#import "View_PictureDetailMgr.h"
#import "View_PictureGallery.h"
#import "View_PictureGalleryImage.h"
#import "View_PictureGalleryMgr.h"

#import "FileStream.h" // debug

@implementation View_PictureGalleryMgr

-(id)initWithApp:(AppDelegate*)_app
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithApp:_app])
	{
		[self setWantsFullScreenLayout:TRUE];
		
		self.title = @"Gallery";
		
		UIBarButtonItem* item_Serve = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_serve")] style:UIBarButtonItemStylePlain target:self action:@selector(handleServeHttp)] autorelease];
		UIBarButtonItem* item_About = [[[UIBarButtonItem alloc] initWithTitle:@"A" style:UIBarButtonItemStylePlain target:self action:@selector(handleAbout)] autorelease];
		UIBarButtonItem* item_Debug = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCompose target:self action:@selector(handleDebug)] autorelease];
		UIBarButtonItem* item_Space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];
		[self setToolbarItems:[NSArray arrayWithObjects:item_Serve, item_Space, item_About, item_Debug, nil]];
		
		UIBarButtonItem* item_Help = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_help")] style:UIBarButtonItemStylePlain target:self action:@selector(handleHelp)] autorelease];
		self.navigationItem.leftBarButtonItem = item_Help;
		UIBarButtonItem* item_New = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_new")] style:UIBarButtonItemStylePlain target:self action:@selector(handleNew)] autorelease];
		self.navigationItem.rightBarButtonItem = item_New;
		
		overwritePictureAlert = [[UIAlertView alloc] initWithTitle:NS(Deployment::PictureOverwriteTitle) message:NS(Deployment::PictureOverwriteText) delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"OK", nil];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleNew
{
	HandleExceptionObjcBegin();
	
	[app newApplication];
	
	ImageId imageId = app.mApplication->AllocateImageId();
	
	if (app.mApplication->ImageExists(imageId))
	{
		overwriteId = imageId;
		
		[overwritePictureAlert show];
	}
	else
	{
		[self beginNew:imageId];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)beginNew:(ImageId)imageId
{
	lastImageId = imageId;
	
	const float scale = [AppDelegate displayScale];
	CGSize screenSize = [UIScreen mainScreen].applicationFrame.size;
	screenSize.width *= scale;
	screenSize.height *= scale;
	
	app.mApplication->ImageReset(imageId);
	app.mApplication->ImageActivate(true);
	app.mApplication->UndoEnabled_set(true);
	app.mApplication->ImageInitialize(3, screenSize.width, screenSize.height);
	app.mApplication->SaveImage();
	
	View_PictureGallery* galleryView = (View_PictureGallery*)self.view;
	[galleryView addImage:imageId];
	
	[app.vcEditing newPainting];
	[app show:app.vcEditing];
}

-(void)pictureSelected:(id)object
{
	HandleExceptionObjcBegin();
	
	View_PictureGalleryImage* image = (View_PictureGalleryImage*)object;
	
	LOG_DBG("picture: %s", image.imageId->mName.c_str());
	
	lastImageId = *image.imageId;
	
	View_PictureDetailMgr* subController = [[[View_PictureDetailMgr alloc] initWithApp:app] autorelease];
	[subController setImageId:*image.imageId];
//	[app show:app.vcPictureDetail];
	[app show:subController];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleServeHttp
{
	HandleExceptionObjcBegin();
	
	View_HttpServerMgr* controller = [[[View_HttpServerMgr alloc] initWithApp:app] autorelease];
	[self presentModalViewController:controller animated:YES];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleAbout
{
	HandleExceptionObjcBegin();
	
	View_AboutMgr* controller = [[[View_AboutMgr alloc] initWithApp:app] autorelease];
	[self presentModalViewController:controller animated:YES];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleHelp
{
	HandleExceptionObjcBegin();
	
	View_HelpMgr* controller = [[[View_HelpMgr alloc] initWithApp:app] autorelease];
	[self presentModalViewController:controller animated:YES];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleDebug
{
	HandleExceptionObjcBegin();
	
	UITextView* textView = [[[UITextView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
	FileStream stream;
	stream.Open(gSystem.GetDocumentPath("exception.log").c_str(), OpenMode_Read);
	StreamReader reader(&stream, false);
	std::string text = reader.ReadAllText();
	[textView setText:[NSString stringWithCString:text.c_str() encoding:NSASCIIStringEncoding]];
	[self.view addSubview:textView];
	
	HandleExceptionObjcEnd(false);
}

// --------------------
// UIAlertViewDelegate
// --------------------
-(void)alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	if (alertView == overwritePictureAlert)
	{
		if (buttonIndex == 0)
		{
			LOG_DBG("picture overwrite: cancelled", 0);
		}
		
		if (buttonIndex == 1)
		{
			LOG_DBG("picture overwrite", 0);
			
			[self beginNew:overwriteId];
		}
	}
}

//

-(void)loadView
{
	HandleExceptionObjcBegin();
	
	self.view = [[[View_PictureGallery alloc] initWithFrame:[UIScreen mainScreen].applicationFrame app:app controller:self] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_PictureGalleryMgr: viewWillAppear", 0);
	
	[self setMenuOpaque];
	
	View_PictureGallery* vw = (View_PictureGallery*)self.view;
	
	[vw scan];
	
	if (lastImageId.IsSet_get())
		[vw scrollIntoView:lastImageId];
	else
		[vw scrollToBottom];
		
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_PictureGalleryMgr", 0);
	
	[overwritePictureAlert release];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end

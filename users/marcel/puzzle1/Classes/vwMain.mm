#import "AppDelegate.h"
#import "OpenFeint.h"
#import "SHKItem.h"
#import "SHKActionSheet.h"
#import "State.h"
#import "vwAlbum.h"
#import "vwMain.h"
#import "vwPuzzle.h"

// note: direct promo code link: https://phobos.apple.com/WebObjects/MZFinance.woa/wa/freeProductCodeWizard?code=RWAPJ7XLTHN7

// todo: add openfeint enabled setting
// todo: create enabled openfeint/open dashboard, deactivate openfeint alert

@implementation vwMain

@synthesize difficulty;
@synthesize banner;

-(id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil 
{
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) 
	{
//		[self setWantsFullScreenLayout:YES];
		
		asPictureSource = [[UIActionSheet alloc] initWithTitle:@"Get Picture From" delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil otherButtonTitles:nil];
		asPictureSource_Preset = [asPictureSource addButtonWithTitle:@"Built-In Collection"];
		asPictureSource_Camera = [asPictureSource addButtonWithTitle:@"Photo Camera"];
		asPictureSource_PhotoAlbum = [asPictureSource addButtonWithTitle:@"Photo Album"];
		infoAlert = [[UIAlertView alloc] initWithTitle:@"About" message:@"Puzzle Time © 2010 grannies games" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:@"More Games!", nil];
		shareAlert = [[UIAlertView alloc] initWithTitle:@"Share Game" message:@"Do you want to share this (free) app with your friends?" delegate:self cancelButtonTitle:@"Not now" otherButtonTitles:@"Share!", nil];
		shareInfoAlert = [[UIAlertView alloc] initWithTitle:@"Share Game" message:@"The share option enables you to post or mail a direct download link to this free app to one of your friends or on your Facebook or Twitter account." delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
		rateAlert = [[UIAlertView alloc] initWithTitle:@"Like This App?" message:@"If you like this app, please give it a 5 star rating on iTunes! :)" delegate:self cancelButtonTitle:@"Remind me later" otherButtonTitles:@"Yes, rate it!", @"Don't ask again", nil];
		feintAlertEnable = [[UIAlertView alloc] initWithTitle:@"Enable OpenFeint?" message:@"Do you want to enable OpenFeint and participate in online leaderboards and achievements?" delegate:self cancelButtonTitle:@"No thanks" otherButtonTitles:@"Yes!", nil];
		feintAlertOpen = [[UIAlertView alloc] initWithTitle:@"OpenFeint" message:nil delegate:self cancelButtonTitle:@"Disable" otherButtonTitles:@"Dashboard", nil];
		feintInfoAlert = [[UIAlertView alloc] initWithTitle:@"OpenFeint" message:@"To enable OpenFeint, please click on the OpenFeint button and sign in." delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
    }
	
    return self;
}

-(IBAction)handleBeginFree:(id)sender
{
	mode = PuzzleMode_Free;
	
	[asPictureSource showInView:self.view];
}

-(IBAction)handleBeginSwitch:(id)sender
{
	mode = PuzzleMode_Switch;
	
	[asPictureSource showInView:self.view];
}

-(IBAction)handleShare:(id)sender
{
	[shareInfoAlert show];
}

-(void)handleShare2:(id)sender
{
	NSString* text = [NSString stringWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"applink" ofType:@"txt"] encoding:NSASCIIStringEncoding error:nil];
	
	SHKItem* item = [SHKItem text:text];
	SHKActionSheet* actionSheet = [SHKActionSheet actionSheetForItem:item];
	
	[actionSheet showInView:self.view];
}

-(IBAction)handleInfo:(id)sender
{
	[infoAlert show];
}

-(IBAction)handleOpenfeint:(id)sender
{
	if (GetInt(@"feint_enabled"))
	{
		[feintAlertOpen show];
	}
	else
	{
		[feintAlertEnable show];
	}
}

-(IBAction)handleRate:(id)sender
{
	[rateAlert show];
}

-(void)handleAcquirePhotoAlbum
{
	imagePickerAlbum = [[[UIImagePickerController alloc] init] autorelease];
	[imagePickerAlbum setDelegate:self];
	[imagePickerAlbum setSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
	
	[self presentModalViewController:imagePickerAlbum animated:TRUE];
}

-(void)handleAcquirePhotoCamera
{
	BOOL supported = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
	
	if (!supported)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Not supported" message:@"The device has no photo camera support" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
		return;
	}
	
	imagePickerCamera = [[[UIImagePickerController alloc] init] autorelease];
	[imagePickerCamera setDelegate:self];
	[imagePickerCamera setSourceType:UIImagePickerControllerSourceTypeCamera];
	
	[self presentModalViewController:imagePickerCamera animated:TRUE];
}

+(UIImage*)processImage:(UIImage*)image size:(Vec2I)size
{
	CGImageRef cgImage = image.CGImage;
	
	float sx = CGImageGetWidth(cgImage);
	float sy = CGImageGetHeight(cgImage);
	
	float scale = MAX(size[0] / sx, size[1] / sy);
	
	sx *= scale;
	sy *= scale;
	
	float x = (size[0] - sx) / 2.0f;
	float y = (size[1] - sy) / 2.0f;
	
	UIGraphicsBeginImageContextWithOptions(CGSizeMake(size[0], size[1]), true, [UIScreen mainScreen].scale);
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
	CGContextTranslateCTM(ctx, 0.0f, size[1]);
	CGContextScaleCTM(ctx, 1.0f, -1.0);
	CGContextDrawImage(ctx, CGRectMake(x, y, sx, sy), cgImage);
	UIImage* result = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	
	LOG_DBG("processed image scale: %f, size: %fx%f", result.scale, result.size.width, result.size.height);
	
	return result;
}

-(void)selectImage:(UIImage *)image
{
	int diff = [difficulty selectedSegmentIndex];
	
	const int diff2size[3] = { 3, 4, 5 };
	
	int size = diff2size[diff];
	
	vwPuzzle* puzzle = [[[vwPuzzle alloc] initWithNibName:@"vwPuzzle" bundle:nil mode:mode image:image size:size] autorelease];
	[self presentModalViewController:puzzle animated:YES];
}

-(void)handleAcquirePreset
{
	imagePickerPreset = [[[vwAlbum alloc] initWithNibName:@"vwAlbum" bundle:nil delegate:self] autorelease];
//	[imagePickerPreset setDelegate:self];
	
	[self presentModalViewController:imagePickerPreset animated:TRUE];
}

// ---------------------
// UIActionSheetDelegate
// ---------------------

-(void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
	if (actionSheet == asPictureSource)
	{
		if (buttonIndex == asPictureSource_PhotoAlbum)
		{
			[self handleAcquirePhotoAlbum];
		}
		if (buttonIndex == asPictureSource_Camera)
		{
			[self handleAcquirePhotoCamera];
		}
		if (buttonIndex == asPictureSource_Preset)
		{
			[self handleAcquirePreset];
		}
	}
}

// -------------------------------
// UIAlertViewDelegate
// -------------------------------

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	if (alertView == infoAlert)
	{
		if (buttonIndex == 1)
		{
//			NSString* url = @"http://itunes.apple.com/nl/artist/grannies-games/id347602547";
			NSString* url = @"http://itunes.com/apps/granniesgames";
	
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
		}
	}
	if (alertView == shareAlert)
	{
		if (buttonIndex == 1)
		{
			[shareAlert dismissWithClickedButtonIndex:-1 animated:NO];
			
//			[NSTimer scheduledTimerWithTimeInterval:1.5f target:self selector:@selector(handleShare:) userInfo:nil repeats:NO];
			[self handleShare:nil];
			
			SetInt(@"share_disabled", 1);
		}
	}
	if (alertView == shareInfoAlert)
	{
		[shareInfoAlert dismissWithClickedButtonIndex:-1 animated:NO];
		
		[self handleShare2:nil];
	}
	if (alertView == rateAlert)
	{
		if (buttonIndex == 0)
		{
			// remind me
		}
		if (buttonIndex == 1)
		{
			// rate it!
			
			//NSString* url = @"http://itunes.com/apps/granniesgames/puzzletime";
			//NSString* url = @"http://itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?id=386963293";
			//NSString* url = @"https://userpub.itunes.apple.com/WebObjects/MZUserPublishing.woa/wa/addUserReview?id=386963293&type=Purple+Software";
			//NSString* url = @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?id=386963293";
			NSString* url = @"itms-apps://ax.itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?type=Purple+Software&id=386963293";
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
			
			SetInt(@"rating_disabled", 1);
		}
		if (buttonIndex == 2)
		{
			// don't ask again
			
			SetInt(@"rating_disabled", 1);
		}
	}
	if (alertView == feintAlertEnable)
	{
		if (buttonIndex == 1)
		{
			[[AppDelegate mainApp] feintEnable];
		}
	}
	if (alertView == feintAlertOpen)
	{
		if (buttonIndex == 0)
		{
			[[AppDelegate mainApp] feintDisable];
		}
		if (buttonIndex == 1)
		{
			[feintAlertOpen dismissWithClickedButtonIndex:-1 animated:NO];
			
			[OpenFeint launchDashboard];
		}
	}
}

// -------------------------------
// UIImagePickerControllerDelegate
// -------------------------------

-(void)imagePickerController:(UIImagePickerController*)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
	[self dismissModalViewControllerAnimated:NO];
	
	UIImage* image = [info valueForKey:UIImagePickerControllerOriginalImage];
	
	if (image == nil)
	{
		return;
	}
	
	Vec2I imageSize = Vec2I(self.view.bounds.size.width, self.view.bounds.size.width);// * (int)[UIScreen mainScreen].scale;
	
	image = [vwMain processImage:image size:imageSize];
	
	[self selectImage:image];
}

-(void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	[self dismissModalViewControllerAnimated:TRUE];
}

// -------------
// AlbumDelegate
// -------------

-(void)albumImageSelected:(UIImage*)image
{
	[self dismissModalViewControllerAnimated:NO];
	
	Vec2I imageSize(self.view.bounds.size.width, self.view.bounds.size.width);
	
	image = [vwMain processImage:image size:imageSize];
	
	[self selectImage:image];
}

-(void)albumImageDismissed
{
	[self dismissModalViewControllerAnimated:TRUE];
}

// --------------------
// ADBannerViewDelegate
// --------------------

-(void)bannerViewDidLoadAd:(ADBannerView *)_banner
{
	LOG_INF("bannerViewDidLoadAd", 0);
	
	if (_banner.hidden)
		[_banner setAlpha:0.0f];
	
	[UIView beginAnimations:@"iAd" context:nil];
	
	[_banner setHidden:NO];
	[_banner setAlpha:1.0f];
	
	[UIView commitAnimations];
}

-(void)bannerView:(ADBannerView *)_banner didFailToReceiveAdWithError:(NSError *)error
{
	LOG_INF("bannerViewDidFailToReceiveAdWithError", 0);
	NSLog(@"error: %@", error);
	
	[_banner setHidden:YES];
}

-(BOOL)bannerViewActionShouldBegin:(ADBannerView *)banner willLeaveApplication:(BOOL)willLeave
{
	LOG_INF("bannerViewActionShouldBegin", 0);
	
//	if (willLeave)
//		return NO;
	
	return YES;
}

-(void)didReceiveMemoryWarning 
{
    [super didReceiveMemoryWarning];
}

-(void)viewDidLoad
{
	bool prompted = false;
	
	if (!prompted)
	{
		// prompt OpenFeint info
		
		int playCount = GetInt(@"app_count");

		if (playCount == 1)
		{
			[feintInfoAlert show];
		
			prompted = true;
		}
	}
	
	if (!prompted)
	{
		// prompt rate question
		
		NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

		if (![defaults objectForKey:@"last_rated"]) 
		{
			[defaults setObject:[NSDate date] forKey:@"last_rated"];
		}

		NSInteger daysSinceInstall = [[NSDate date] timeIntervalSinceDate:[defaults objectForKey:@"last_rated"]] / 86400;
		
/*#ifdef DEBUG
		daysSinceInstall = 100;
#endif*/
		
		if (daysSinceInstall > 10 && !GetInt(@"rating_disabled"))
		{
			[rateAlert show];
			
			[defaults setObject:[NSDate date] forKey:@"last_rated"];
			
			prompted = true;
		}

		[defaults synchronize];
	}
	
	if (!prompted)
	{
		if (!GetInt(@"share_disabled"))
		{
			// prompt share question
			
			int playCount = GetInt(@"app_count");

			if (playCount % 3 == 1)
			{
				[shareAlert show];
			
				prompted = true;
			}
		}
	}
	
	CGRect rect = self.view.bounds;
	
	rect.origin.y = 20.0f;
	
	[self.view setFrame:rect];
}

-(void)dealloc 
{
	self.banner.delegate = nil;
	self.banner = nil;
	
	[infoAlert release];
	[shareAlert release];
	[shareInfoAlert release];
	[rateAlert release];
	[feintAlertEnable release];
	[feintAlertOpen release];
	[feintInfoAlert release];
	
    [super dealloc];
}

@end

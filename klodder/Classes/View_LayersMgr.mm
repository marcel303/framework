#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "Events.h"
#import "ExceptionLoggerObjC.h"
#import "KlodderSystem.h"
#import "LayerMgr.h"
#import "Log.h"
#import "UIImageEx.h"
#import "View_LayerClearMgr.h"
#import "View_LayerManager.h"
#import "View_LayerManagerLayer.h"
#import "View_Layers.h"
#import "View_LayersMgr.h"
#import "View_PictureGalleryMgr.h"

static bool IsLastLayer(int index, std::vector<int> layerOrder);

@implementation View_LayersMgr

@synthesize focusLayerIndex;

-(id)initWithApp:(AppDelegate*)_app
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithApp:_app]))
	{
		self.title = @"Layers";
		[self setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
		//[self setModalInPopover:YES];
		[self setPreferredContentSize:CGSizeMake(320.0f, 480.0f)];
		
		focusLayerIndex = -1;
		
		// note: creating image picker controllers here adds ~3.4 sec additional boot time
		
		BOOL hasCamera = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
		
		asAcquire = [[UIActionSheet alloc] initWithTitle:@"Acquire" delegate:self cancelButtonTitle:cancelTitle destructiveButtonTitle:nil otherButtonTitles:nil];
		[asAcquire setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
		asAcquirePhotoAlbum = [asAcquire addButtonWithTitle:@"Photo album"];
		if (hasCamera)
			asAcquirePhotoCamera = [asAcquire addButtonWithTitle:@"Photo camera"];
		else
			asAcquirePhotoCamera = -10;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)loadView 
{
	HandleExceptionObjcBegin();
	
#ifdef IPAD
	CGRect rect = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f);
#else
	CGRect rect = [UIScreen mainScreen].applicationFrame;
#endif
	
	self.view = [[[View_Layers alloc] initWithFrame:rect andApp:app controller:self] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_LayersMgr:: viewWillAppear", 0);
	
	[self setMenuOpaque];
	
	focusLayerIndex = app.mApplication->LayerMgr_get()->EditingDataLayer_get();

	layerOrder = app.mApplication->LayerMgr_get()->LayerOrder_get();
	
	View_Layers* vw = (View_Layers*)self.view;
	
	[vw handleFocus];
	
	[self updateUi];
	
	[self.navigationController setToolbarHidden:FALSE animated:FALSE];
	[self.navigationController setNavigationBarHidden:FALSE animated:FALSE];
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self applyChanges];
	
	HandleExceptionObjcEnd(false);
}

// UI handlers

-(void)handleBack
{
	[self dismissModalViewControllerAnimated:YES];
}

-(void)handleAcquire
{
	HandleExceptionObjcBegin();
	
	[asAcquire showInView:self.view];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleToggleVisibility
{
	HandleExceptionObjcBegin();
	
	const int index = [self selectedDataLayer];
	
	if (index < 0)
		return;
	
	bool visibility = app.mApplication->LayerMgr_get()->DataLayerVisibility_get(index);
	
	app.mApplication->DataLayerVisibility(index, !visibility);
	
	[self updateUi];
	
	[Events post:EVT_LAYERS_CHANGED];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayerMergeDown
{
	HandleExceptionObjcBegin();
	
	int layer = [self selectedLayer];
	
	if (layer < 0)
		return;
	
	if (layer == 0)
		return;
	
	int layer1 = layer;
	int layer2 = layer - 1;
	
	int index1 = app.mApplication->LayerMgr_get()->LayerOrder_get(layer1);
	int index2 = app.mApplication->LayerMgr_get()->LayerOrder_get(layer2);
	
	// horrible solution to get preview opacity finalized before merge..
	
	View_Layers* vw = (View_Layers*)self.view;
	
	View_LayerManagerLayer* layerView1 = [vw.layerMgr layerByIndex:index1];
	View_LayerManagerLayer* layerView2 = [vw.layerMgr layerByIndex:index1];
	
	if (layerView1.previewOpacity.HasValue_get())
		app.mApplication->DataLayerOpacity(index1, layerView1.previewOpacity.Value_get());
	if (layerView2.previewOpacity.HasValue_get())
		app.mApplication->DataLayerOpacity(index2, layerView2.previewOpacity.Value_get());
	
	[vw handleFocus];
	
	//
	
	app.mApplication->DataLayerMerge(index1, index2);
	
	[self updatePreviewPictures];
	
	[self updateUi];
	
	[Events post:EVT_LAYERS_CHANGED];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayerClear
{
	HandleExceptionObjcBegin();
	
	int index = [self selectedDataLayer];
	
	if (index < 0)
		return;
	
	View_LayerClearMgr* controller = [[[View_LayerClearMgr alloc] initWithApp:app index:index] autorelease];
	//[app show:controller];
	[self presentModalViewController:controller animated:YES];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleUndo
{
	HandleExceptionObjcBegin();
	
	app.mApplication->Undo();
	
	app.mApplication->LayerMgr_get()->Validate();
	
	[self updateLayerOrder];
	[self updatePreviewPictures];
	[self updateUi];

	HandleExceptionObjcEnd(false);
}

-(void)handleRedo
{
	HandleExceptionObjcBegin();
	
	app.mApplication->Redo();
	
	app.mApplication->LayerMgr_get()->Validate();
	
	[self updateLayerOrder];
	[self updatePreviewPictures];
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

/*-(void)setToolbarItems:(NSArray*)items
{
	View_Layers* vw = (View_Layers*)self.view;
	[vw.toolBar setItems:items];
}*/

-(void)updateUi
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_LayersMgr: updateUi", 0);
	
	UIBarButtonItem* item_MergeDown = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_layer_merge")] style:UIBarButtonItemStylePlain target:self action:@selector(handleLayerMergeDown)] autorelease];
	UIBarButtonItem* item_Clear = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_layer_fill")] style:UIBarButtonItemStylePlain target:self action:@selector(handleLayerClear)] autorelease];
	UIBarButtonItem* item_Undo = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_undo")] style:UIBarButtonItemStylePlain target:self action:@selector(handleUndo)] autorelease];
	UIBarButtonItem* item_Redo = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_redo")] style:UIBarButtonItemStylePlain target:self action:@selector(handleRedo)] autorelease];
	UIBarButtonItem* item_Acquire = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCamera target:self action:@selector(handleAcquire)] autorelease];
//	UIBarButtonItem* item_ToggleVisibility = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAction target:self action:@selector(handleToggleVisibility)] autorelease];
	UIBarButtonItem* item_ToggleVisibility = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_visibility")] style:UIBarButtonItemStylePlain target:self action:@selector(handleToggleVisibility)] autorelease];
	UIBarButtonItem* item_Space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];
	[item_MergeDown setEnabled:focusLayerIndex >= 0 && !IsLastLayer(focusLayerIndex, app.mApplication->LayerMgr_get()->LayerOrder_get())];
	[item_Clear setEnabled:focusLayerIndex >= 0];
	[item_Acquire setEnabled:focusLayerIndex >= 0];
	[item_ToggleVisibility setEnabled:focusLayerIndex >= 0];
	[item_Undo setEnabled:app.mApplication->HasUndo_get()];
	[item_Redo setEnabled:app.mApplication->HasRedo_get()];
	
	[self setToolbarItems:[NSArray arrayWithObjects:item_MergeDown, item_Space, item_Clear, item_Space, item_Undo, item_Space, item_Redo, item_Space, item_ToggleVisibility, item_Space, item_Acquire, nil]];
	
	View_Layers* vw = (View_Layers*)self.view;
	[vw updateUi];
	
	HandleExceptionObjcEnd(false);
}

// implementation

-(void)applyChanges
{
	View_Layers* vw = (View_Layers*)self.view;
	
	std::map<int, float> layerOpacity = [vw.layerMgr layerOpacity];
	
	for (std::map<int, float>::iterator i = layerOpacity.begin(); i != layerOpacity.end(); ++i)
	{
		app.mApplication->DataLayerOpacity(i->first, i->second);
	}
	
//	int index = __layer.index;
	int index = [self selectedDataLayer];
	
	if (index >= 0)
	{
		app.mApplication->DataLayerSelect(index);
	}
}

-(void)acquirePhotoAlbum
{
	HandleExceptionObjcBegin();
	
	int index = [self selectedDataLayer];
	
	if (index < 0)
		return;
	
	BOOL supported = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary];
	
	if (!supported)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Not supported" message:@"The device has no photo album support" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
		return;
	}
	
	LOG_DBG("show image picker (album)", 0);
	
	acquireLayerIndex = index;
	acquireType = AcquireType_Album;
	
	imagePickerAlbum = [[[UIImagePickerController alloc] init] autorelease];
	[imagePickerAlbum setDelegate:self];
	[imagePickerAlbum setSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
	
	[self presentModalViewController:imagePickerAlbum animated:TRUE];
	
	HandleExceptionObjcEnd(false);
}

-(void)acquirePhotoCamera
{
	HandleExceptionObjcBegin();
	
	int index = [self selectedDataLayer];
	
	if (index < 0)
		return;
	
	BOOL supported = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
	
	if (!supported)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Not supported" message:@"The device has no photo camera support" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
		return;
	}
	
	LOG_DBG("show image picker (camera)", 0);
	
	acquireLayerIndex = index;
	acquireType = AcquireType_Camera;
	
	imagePickerCamera = [[[UIImagePickerController alloc] init] autorelease];
	[imagePickerCamera setDelegate:self];
	[imagePickerCamera setSourceType:UIImagePickerControllerSourceTypeCamera];
	
	[self presentModalViewController:imagePickerCamera animated:TRUE];

	HandleExceptionObjcEnd(false);
}

-(void)handleLayerFocus:(View_LayerManagerLayer*)layer
{	
	HandleExceptionObjcBegin();
	
	LOG_DBG("handleLayerFocus", 0);
	
	focusLayerIndex = layer.index;
	
	[self updateUi];
	
	LOG_DBG("focus layer: data_idx=%d, layer_idx=%d", layer.index, [self selectedLayer]);
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayerSelect:(View_LayerManagerLayer*)__layer
{
	HandleExceptionObjcBegin();
	
	[app hideWithAnimation:TRUE];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayerOrderChanged:(std::vector<int>)_layerOrder
{
	HandleExceptionObjcBegin();
	
	std::vector<int> oldOrder = app.mApplication->LayerMgr_get()->LayerOrder_get();
	
	bool changed = false;
	
	Assert(oldOrder.size() == _layerOrder.size());
	
	for (size_t i = 0; i < _layerOrder.size(); ++i)
		if (_layerOrder[i] != oldOrder[i])
			changed = true;
	
	if (changed)
	{
		app.mApplication->LayerOrder(_layerOrder);
		
		[Events post:EVT_LAYERS_CHANGED];
	}
	
	HandleExceptionObjcEnd(false);
}

#ifdef DEBUG
static float ToFloat(const Rgba* rgba)
{
	return (rgba->rgb[0] + rgba->rgb[1] + rgba->rgb[2]) / 3.0f;
}

static void ApplySobel(Bitmap* bmp)
{
	Bitmap temp;
	
	temp.Size_set(bmp->Sx_get(), bmp->Sy_get(), false);
	
//	const float coeff[3] = { 1, 2, 1 };
	
	for (int y = 0; y < bmp->Sy_get(); ++y)
	{
		for (int x = 0 ; x < bmp->Sx_get(); ++x)
		{
#if 0
			const Rgba* x1[3] = {
				bmp->Sample_Clamped_Ref(x - 1, y - 1),
				bmp->Sample_Clamped_Ref(x + 0, y - 1),
				bmp->Sample_Clamped_Ref(x + 1, y - 1)
			};
			
			const Rgba* x2[3] = {
				bmp->Sample_Clamped_Ref(x - 1, y + 1),
				bmp->Sample_Clamped_Ref(x + 0, y + 1),
				bmp->Sample_Clamped_Ref(x + 1, y + 1)
			};
			
			Rgba c = { 0.0f, 0.0f, 0.0f, 0.0f };
			
			for (int i = 0; i < 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					c.rgb[j] -= x1[i]->rgb[j] * coeff[i];
					c.rgb[j] += x2[i]->rgb[j] * coeff[i];
				}
			}
			
			for (int i = 0; i < 3; ++i)
				c.rgb[i] = Calc::Saturate(Calc::Abs(c.rgb[i]));
			
			for (int i = 0; i < 4; ++i)
			{
				temp.Line_get(y)[x] = c;
			}
#else
			const float x1[3] = {
				ToFloat(bmp->Sample_Clamped_Ref(x - 1, y - 1)),
				ToFloat(bmp->Sample_Clamped_Ref(x + 0, y - 1)),
				ToFloat(bmp->Sample_Clamped_Ref(x + 1, y - 1))
			};
			
			const float x2[3] = {
				ToFloat(bmp->Sample_Clamped_Ref(x - 1, y + 1)),
				ToFloat(bmp->Sample_Clamped_Ref(x + 0, y + 1)),
				ToFloat(bmp->Sample_Clamped_Ref(x + 1, y + 1))
			};
			
			const float y1[3] = {
				ToFloat(bmp->Sample_Clamped_Ref(x - 1, y - 1)),
				ToFloat(bmp->Sample_Clamped_Ref(x - 1, y + 0)),
				ToFloat(bmp->Sample_Clamped_Ref(x - 1, y + 1))
			};
			
			const float y2[3] = {
				ToFloat(bmp->Sample_Clamped_Ref(x + 1, y - 1)),
				ToFloat(bmp->Sample_Clamped_Ref(x + 1, y + 0)),
				ToFloat(bmp->Sample_Clamped_Ref(x + 1, y + 1))
			};
			
			const float vx = -x1[0] - x1[1] * 2.0f - x1[2] + x2[0] + x2[1] * 2.0f + x2[2];
			const float vy = -y1[0] - y1[1] * 2.0f - y1[2] + y2[0] + y2[1] * 2.0f + y2[2];
			
			const float v = Calc::Saturate(Calc::Abs(vx) + Calc::Abs(vy));
			
			for (int i = 0; i < 4; ++i)
				temp.Line_get(y)[x].rgb[i] = bmp->Line_get(y)[x].rgb[i] * v;
#endif
		}
	}
	
	temp.Blit(bmp);
}
#endif

-(void)handleLayerOpacityChanged:(float)opacity
{
	HandleExceptionObjcBegin();
	
#ifdef DEBUG
	app.mApplication->LayerMgr_get()->EditingBegin(false);
	ApplySobel(app.mApplication->LayerMgr_get()->EditingBuffer_get());
	app.mApplication->LayerMgr_get()->EditingEnd();
#endif
	
	View_Layers* vw = (View_Layers*)self.view;
	
	if (focusLayerIndex >= 0)
	{
		View_LayerManagerLayer* layer = [vw.layerMgr layerByIndex:focusLayerIndex];
	
		[layer setPreviewOpacity:opacity];
		
		//
		
		const int index = [self selectedDataLayer];
		
		app.mApplication->DataLayerOpacity(index, opacity);
		
		[Events post:EVT_LAYERS_CHANGED];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
/*	[imagePickerAlbum release];
	imagePickerAlbum = nil;
	[imagePickerCamera release];
	imagePickerCamera = nil;*/
	[asAcquire release];
	asAcquire = nil;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false)
}

-(UIImage*)processCameraImage2:(UIImage*)image
{
	CGImageRef cgImage = image.CGImage;
	
	float sx = CGImageGetWidth(cgImage);
	float sy = CGImageGetHeight(cgImage);
	
	// downscale image to at most MaxByteCount bytes
	
	const int MaxByteCount = 1 << 22;
	bool downsize = false;
	
	while (sx * sy * 4 > MaxByteCount)
	{
		sx /= 2.0f;
		sy /= 2.0f;
		downsize = true;
	}
	
	UIImage* result = nil;
	
	if (downsize || true)
	{
		UIGraphicsBeginImageContext(CGSizeMake(sx, sy));
		CGContextRef ctx = UIGraphicsGetCurrentContext();
		CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
		CGContextTranslateCTM(ctx, 0.0f, sy);
		CGContextScaleCTM(ctx, 1.0f, -1.0);
		CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, sx, sy), cgImage);
		result = UIGraphicsGetImageFromCurrentImageContext();
		UIGraphicsEndImageContext();
	}
	else
	{
		result = [UIImage imageWithCGImage:cgImage];
	}
	
	return result;
}

-(UIImage*)processCameraImage:(UIImage*)image
{
	image = [self processCameraImage2:image];
	
/*	if (scale < 1.0f - 0.001f)
	{
		const float sx = image.size.width * scale;
		const float sy = image.size.height * scale;
	
		image = [image resizedImage:CGSizeMake(sx, sy) interpolationQuality:kCGInterpolationHigh];
	}*/
	
	return image;
}

-(void)updateLayerOrder
{
	View_Layers* vw = (View_Layers*)self.view;
	
	[vw updateLayerOrder];
}

-(void)updatePreviewPictures
{
	View_Layers* vw = (View_Layers*)self.view;
	
	[vw updatePreviewPictures];
}

-(int)selectedLayer
{
	if (focusLayerIndex < 0)
		return -1;
	
	std::vector<int> __layerOrder = app.mApplication->LayerMgr_get()->LayerOrder_get();
	
	int layer = 0;
	
	for (size_t i = 0; i < __layerOrder.size(); ++i)
		if (__layerOrder[i] == focusLayerIndex)
			layer = (int)i;
	
	return layer;
}

-(int)selectedDataLayer
{
	return focusLayerIndex;
}

// ---------------------
// UIActionSheetDelegate
// ---------------------

-(void)actionSheet:(UIActionSheet*)actionSheet clickedButtonAtIndex:(NSInteger)index
{
	HandleExceptionObjcBegin();
	
	if (actionSheet == asAcquire)
	{
		if (focusLayerIndex < 0)
		{
			[[[[UIAlertView alloc] initWithTitle:@"Error" message:@"No destination layer selected." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
			return;
		}
		
		if (index == asAcquirePhotoAlbum)
		{
			[self acquirePhotoAlbum];
		}
		else if (index == asAcquirePhotoCamera)
		{
			[self acquirePhotoCamera];
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

// -------------------------------
// UIImagePickerControllerDelegate
// -------------------------------

-(void)imagePickerController:(UIImagePickerController*)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("image picker: finish", 0);
	
	[self dismissModalViewControllerAnimated:TRUE];
	
	const int index = [self selectedDataLayer];
	
	if (index < 0)
	{
		LOG_DBG("no data layer selected", 0);
		
		return;
	}
	
	UIImage* image = [info valueForKey:UIImagePickerControllerOriginalImage];
	
	if (image == nil)
	{
		LOG_DBG("image picker: image is nil", 0);
		
		return;
	}
	
	if (acquireType == AcquireType_Album)
	{
		image = [self processCameraImage:image];
	}
	if (acquireType == AcquireType_Camera)
	{
		image = [self processCameraImage:image];
	}
	
	const Vec2I size = app.mApplication->LayerMgr_get()->Size_get();
	
	View_ImagePlacementMgr* vc = [[[View_ImagePlacementMgr alloc] initWithImage:image size:size dataLayer:index app:app delegate:self] autorelease];
	[app show:vc];
//	[self presentModalViewController:vc animated:FALSE];

	HandleExceptionObjcEnd(false);
}

-(void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("image picker: canceled", 0);
	
	[self dismissModalViewControllerAnimated:TRUE];
	
	HandleExceptionObjcEnd(false);
}

// ----------------------
// ImagePlacementDelegate
// ----------------------

-(void)placementFinished:(UIImage*)image dataLayer:(int)dataLayer transform:(BlitTransform*)transform controller:(View_ImagePlacementMgr*)controller
{
	HandleExceptionObjcBegin();
	
	// blit
	
	int index = acquireLayerIndex;
	
	MacImage* macImage = [AppDelegate uiImageToMacImage:image];
	app.mApplication->DataLayerBlit(index, macImage, *transform);
	delete macImage;
	
	[app hideWithAnimation:TRUE];
	
	[self updatePreviewPictures];
	
	HandleExceptionObjcEnd(false);
}

@end

static bool IsLastLayer(int index, std::vector<int> layerOrder)
{
	Assert(layerOrder.size() > 0);
	
	return layerOrder[0] == index;
}

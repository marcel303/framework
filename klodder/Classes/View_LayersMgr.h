#import <vector>
#import "klodder_forward_objc.h"
#import "ViewControllerBase.h"
#import "View_ImagePlacementMgr.h"

@interface View_LayersMgr : ViewControllerBase <UIImagePickerControllerDelegate, UIActionSheetDelegate, ImagePlacementDelegate, UINavigationControllerDelegate>
{
	int focusLayerIndex;
	int acquireLayerIndex;
	
	UIActionSheet* asAcquire;
	NSInteger asAcquirePhotoAlbum;
	NSInteger asAcquirePhotoCamera;
	UIImagePickerController* imagePickerAlbum;
	UIImagePickerController* imagePickerCamera;
	
	std::vector<int> layerOrder;
	
	enum AcquireType
	{
		AcquireType_Album,
		AcquireType_Camera
	};
	
	AcquireType acquireType;
}

//@property (nonatomic, retain) View_LayerManagerLayer* focusLayer;
@property (nonatomic, assign) int focusLayerIndex;

// UI handlers

-(void)handleBack;
-(void)handleAcquire;
-(void)handleToggleVisibility;
-(void)handleLayerMergeDown;
-(void)handleLayerClear;
-(void)handleUndo;
-(void)handleRedo;
-(void)updateUi;

// implementation

-(void)applyChanges;
-(void)acquirePhotoAlbum;
-(void)acquirePhotoCamera;

-(void)handleLayerFocus:(View_LayerManagerLayer*)layer;
-(void)handleLayerSelect:(View_LayerManagerLayer*)layer;
-(void)handleLayerOrderChanged:(std::vector<int>)layerOrder;
-(void)handleLayerOpacityChanged:(float)opacity;

// helpers

-(UIImage*)processCameraImage:(UIImage*)image;
-(void)updateLayerOrder;
-(void)updatePreviewPictures;
-(int)selectedLayer;
-(int)selectedDataLayer;

// UIActionSheetDelegate
-(void)actionSheet:(UIActionSheet*)actionSheet clickedButtonAtIndex:(NSInteger)index;

// UIImagePickerControllerDelegate
-(void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info;
-(void)imagePickerControllerDidCancel:(UIImagePickerController *)picker;

// ImagePlacementDelegate
-(void)placementFinished:(UIImage*)image dataLayer:(int)dataLayer transform:(BlitTransform*)transform controller:(View_ImagePlacementMgr*)controller;

@end

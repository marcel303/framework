#import <vector>
#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "LayerMgr.h"
#import "Log.h"
#import "View_LayerManager.h"
#import "View_LayerManagerLayer.h"
#import "View_LayersMgr.h"

// todo: variable number of layers

@implementation View_LayerManager

static bool Compare(const View_LayerManagerLayer* l1, const View_LayerManagerLayer* l2);

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app controller:(View_LayersMgr*)_controller
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setMultipleTouchEnabled:FALSE];
		[self setUserInteractionEnabled:TRUE];
		
		app = _app;
		controller = _controller;
		
		for (int i = 0; i < 3; ++i)
		{
			layer[i] = [[[View_LayerManagerLayer alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 100.0f) app:app controller:controller parent:self index:i layerIndex:i] autorelease];
			[self addSubview:layer[i]];
		}
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleFocus
{
	LOG_DBG("updating layer previews", 0);
		
	[self alignLayers:TRUE animated:FALSE];
	[self updateDepthOrder];
	[self updatePreviewPictures];
	
	for (int i = 0; i < 3; ++i)
		[layer[i] handleFocus];
}

// --------------------
// visual
// --------------------

-(void)updatePreviewPictures
{
	for (int i = 0; i < 3; ++i)
		[layer[i] updatePreview];
}

-(void)updateUi
{
	for (int i = 0; i < 3; ++i)
		[layer[i] updateUi];
}

-(Vec2F)desiredLayerPosition:(int)__layer
{
	return Vec2F(self.frame.size.width / 2.0f, 230 - __layer * 60.0f);
}

-(void)updateDepthOrder
{
	std::vector<View_LayerManagerLayer*> layers = [self sortedLayers];
	
	for (int i = 0; i < 3; ++i)
		layers.push_back(layer[i]);
	
	std::sort(layers.begin(), layers.end(), Compare);
	
	for (size_t i = 0; i < layers.size(); ++i)
		[self sendSubviewToBack:layers[i]];
}

// --------------------
// interaction
// --------------------

-(void)alignLayers:(BOOL)alignToState animated:(BOOL)animated
{
	if (alignToState)
	{
		LOG_DBG("updating layer positions (visually)", 0);
		
		// position layers based on current order
		
		std::vector<int> layerOrder = app.mApplication->LayerMgr_get()->LayerOrder_get();
		
		for (int i = 0; i < (int)layerOrder.size(); ++i)
		{
//			float y = -i;
			
			View_LayerManagerLayer* __layer = [self layerByIndex:layerOrder[i]];
			
			Assert(__layer != nil);
			
//			__layer.animationLocation = Vec2F(0.0f, y);
			Vec2F location = [self desiredLayerPosition:i];
			__layer.targetLocation = location;

			if (animated)
			{
				[__layer setMoveMode:LayerMoveMode_TargetAnim];
			}
			else
			{
				[__layer setMoveMode:LayerMoveMode_TargetInstant];
			}
		}
	}
	else
	{
		std::vector<View_LayerManagerLayer*> layers = [self sortedLayers];
		
		for (size_t i = 0; i < layers.size(); ++i)
		{
			int index = (int)(layers.size() - 1 - i);
			
			Vec2F location = [self desiredLayerPosition:index];
			
	//		[layers[i] setAnimationLocation:location];
			[layers[i] setTargetLocation:location];
		
			if (layers[i].moveMode != LayerMoveMode_Manual)
			{
				if (animated)
				{
					[layers[i] setMoveMode:LayerMoveMode_TargetAnim];
				}
				else
				{
					[layers[i] setMoveMode:LayerMoveMode_TargetInstant];
				}
			}
		}
	}
}

// --------------------
// helpers
// --------------------

-(void)updateLayerOrder:(View_LayerManagerLayer*)layer
{
	[self alignLayers:FALSE animated:TRUE];
	
	std::vector<int> layerOrder = [self layerOrder];
	
	[controller handleLayerOrderChanged:layerOrder];
}
										 
-(View_LayerManagerLayer*)layerByIndex:(int)index
{
	View_LayerManagerLayer* result = nil;
	
	for (int i = 0; i < 3; ++i)
		if (layer[i].index == index)
			result = layer[i];
	
	return result;
}

static bool Compare(const View_LayerManagerLayer* l1, const View_LayerManagerLayer* l2)
{
	return l1->animationLocation[1] < l2->animationLocation[1];
}

// --------------------
// state
// --------------------

-(std::vector<View_LayerManagerLayer*>)sortedLayers
{
	std::vector<View_LayerManagerLayer*> layers;
	
	for (int i = 0; i < 3; ++i)
		layers.push_back(layer[i]);
	
	std::sort(layers.begin(), layers.end(), Compare);
	
	return layers;
}

-(std::vector<int>)layerOrder
{
	std::vector<int> layerOrder;
	
	std::vector<View_LayerManagerLayer*> layers = [self sortedLayers];
	
	for (size_t i = 0; i < layers.size(); ++i)
		layerOrder.push_back(layers[i].index);
	
	std::reverse(layerOrder.begin(), layerOrder.end());
	
	return layerOrder;
}

-(std::map<int, float>)layerOpacity
{
	std::map<int, float> layerOpacity;
	
	std::vector<View_LayerManagerLayer*> layers = [self sortedLayers];
	
	for (size_t i = 0; i < layers.size(); ++i)
	{
		if (!layers[i].previewOpacity.HasValue_get())
			continue;
		
		const int index = layers[i].index;
		const float opacity = layers[i].previewOpacity.Value_get();
		
		layerOpacity[index]  = opacity;
	}
	
	return layerOpacity;
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_LayerManager", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end

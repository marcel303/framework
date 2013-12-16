#import <map>
#import <UIKit/UIView.h>
#import <vector>
#import "klodder_forward_objc.h"

@interface View_LayerManager : UIView 
{
	AppDelegate* app;
	View_LayersMgr* controller;
	View_LayerManagerLayer* layer[3];
}

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app controller:(View_LayersMgr*)controller;
-(void)handleFocus;

// --------------------
// visual
// --------------------

-(void)updatePreviewPictures;
-(void)updateUi;
-(Vec2F)desiredLayerPosition:(int)index; // Returns the desired view position based on layer index
-(void)updateDepthOrder; // Makes sure layers are drawn in the proper order

// --------------------
// interaction
// --------------------

-(void)alignLayers:(BOOL)alignToState animated:(BOOL)animated; // Align layers based on layer order. If animated, layers will slowly move into their desired position. Elsewise, the update is instant.

// --------------------
// helpers
// --------------------

-(void)updateLayerOrder:(View_LayerManagerLayer*)layer; // Finalizes layer order after a move operation
-(View_LayerManagerLayer*)layerByIndex:(int)index;

// --------------------
// state
// --------------------

-(std::vector<View_LayerManagerLayer*>)sortedLayers; // Returns layers sorted by position
-(std::vector<int>)layerOrder;
-(std::map<int, float>)layerOpacity;

@end

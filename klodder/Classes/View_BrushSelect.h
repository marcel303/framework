#import <UIKit/UIView.h>
#import <vector>
#import "klodder_forward.h"
#import "klodder_forward_objc.h"

// brush selection view
// allows selection of a brush using touch/swipe gestures

#define BRUSH_PATTERN_SOFT -1

@protocol BrushSelectDelegate

-(std::vector<Brush_Pattern*>) getPatternList;
-(void)handleBrushPatternSelect:(int)patternId;
-(void)handleBrushSoftSelect;

@end

class BrushItem
{
public:
	BrushItem();
	BrushItem(ToolType type, Brush_Pattern* pattern);
	
	ToolType mToolType;
	uint32_t mPatternId;
	Brush_Pattern* mPattern;
};

typedef std::vector<BrushItem> BrushItemList;

@interface View_BrushSelect : UIView 
{
	@private
	
	View_ToolSelectMgr* controller;
	id<BrushSelectDelegate> delegate;
	BrushItemList itemList;
	bool touchActive;
	bool touchHasMoved;
	NSTimer* animationTimer;
	bool isAnimating;
	float scrollPosition;
	float scrollTargetPosition;
	int selectionIndex;
	View_BrushSelect_Strip* strip;
	UIImageView* tick;
}

@property (nonatomic, assign) NSTimer* animationTimer;
@property (nonatomic, assign) float scrollPosition;

-(id)initWithFrame:(CGRect)frame controller:(View_ToolSelectMgr*)controller delegate:(id<BrushSelectDelegate>)delegate;
-(void)loadBrushes;
-(void)beginScroll:(bool)animated;
-(int)locationToIndex:(float)x;
-(int)scrollToIndex;
-(float)indexToScroll:(int)index;
-(void)brushSettingsChanged;
-(void)setScrollPosition:(float)position;
-(void)setSelectionIndex:(int)index;

-(void)animationBegin;
-(void)animationEnd;
-(bool)animationTargetReached;
-(void)animationUpdate;

@end

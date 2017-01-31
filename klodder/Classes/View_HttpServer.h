#if BUILD_HTTPSERVER

#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

@interface View_HttpServer : UIView 
{
	IBOutlet View_HttpServerMgr* controller;
	IBOutlet UILabel* lblAddressWifi;
	IBOutlet UILabel* lblAddressWww;
}

@property (nonatomic, assign) UILabel* lblAddressWifi;
@property (nonatomic, assign) UILabel* lblAddressWww;

-(void)updateUi;

@end

#endif

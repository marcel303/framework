#if BUILD_FLICKR

#import <Foundation/Foundation.h>
#import "ObjectiveFlickr.h"

@interface FlickrState : NSObject <OFFlickrAPIRequestDelegate>
{
	OFFlickrAPIContext* flickrContext;
	OFFlickrAPIRequest* flickrRequest;
	bool flickrIsConnected;
}

@property (nonatomic, retain) OFFlickrAPIRequest* flickrRequest;
@property (nonatomic, retain) OFFlickrAPIContext* flickrContext;

-(void)resume;
-(bool)loggedIntoFlickr;
-(void)logIntoFlickr;

-(void)setAndSaveFlickrAuthToken:(NSString*)authToken;
-(void)handleOpenUrl:(NSURL*)url;
-(void)flickrCheckLogin;
-(void)flickrResume;

@end

#endif

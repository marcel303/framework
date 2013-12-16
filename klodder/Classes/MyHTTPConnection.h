#pragma once
#import <Foundation/Foundation.h>
#import "HTTPConnection.h"

@interface MyHTTPConnection : HTTPConnection
{	
}

- (BOOL)isBrowseable:(NSString *)path;
- (NSString *)createBrowseableIndex:(NSString *)path;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2010 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>

@interface OFCompressableData : NSObject {
    size_t uncompressedSize;
    NSData *data;
}
@property (nonatomic) size_t uncompressedSize;  //0 means that it is not compressed
@property (nonatomic, retain) NSData* data;

-(void) compress;
-(void) uncompress;
-(NSData*) uncompressedData;
-(NSData*) serializedData;
-(size_t) length;

//configuration
+(void)setDisableCompression:(BOOL)disable;
+(void)setVerbose:(BOOL)verbose;

//compressed data factories
+(OFCompressableData*) dataWithNSData:(NSData*)_data compress:(BOOL)_compressData;
+(OFCompressableData*) dataWithBytes:(const void*)_data length:(size_t)length compress:(BOOL)_compressData;
+(OFCompressableData*) dataWithSerializedData:(NSData*)_data;

//simple pass-through converters that convert from a blob to a possibly compressed blob and back again
+(NSData*) uncompressedDataFromSerializedData:(NSData*)_serializedData;
+(NSData*) serializedDataFromData:(NSData*)_data;


+(void) runTests;

@end

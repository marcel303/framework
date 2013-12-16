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

#import "OFCompressableData.h"
#import "OFDependencies.h"
#import "OFLog.h"
#ifndef OF_EXCLUDE_ZLIB
#include "zlib.h"
#endif

namespace {
    //if the data begins with this value, then it is assumed that the data is compressed
    //we can use different magics for versioning if that becomes an issue later
    class HeaderMagic {
    public:
        const static size_t size = 8;
        const static size_t headerSize = size + sizeof(size_t);
        HeaderMagic(const char*buffer) : value(buffer) {
        }
        const char *chars() const { return value; }
    private:
        const char *value;
    };
    
    const HeaderMagic magic("OFZLHDR0");
    
    BOOL sVerbose = NO;
    BOOL sDisableCompression = YES;
}

@implementation OFCompressableData
@synthesize uncompressedSize, data;

-(void) compress {
#ifndef OF_EXCLUDE_ZLIB
    if(sDisableCompression) return;
    size_t oldSize = [self.data length];
    if(self.data and [self.data length]) {
        const size_t bufferSize = compressBound([self.data length]);
        unsigned char *destBuffer = new unsigned char[bufferSize];
        size_t destSize = bufferSize;
        int error = compress(destBuffer, &destSize, (const Bytef*) [self.data bytes], oldSize);
        if(error == Z_OK) {
            if(destSize + magic.headerSize < oldSize) {  //TODO: make this more strict?
                self.data = [NSData dataWithBytes:destBuffer length:destSize];
                self.uncompressedSize = oldSize;
                if(sVerbose) {
                    OFLog(@"ZLIB: Compression ratio for %d/%d is %d%%", (destSize + magic.headerSize), oldSize, (int)((destSize + magic.headerSize)*100.f / self.uncompressedSize + 0.5f));
                }
            }
            else {
                if(sVerbose)
                    OFLog(@"ZLIB: Rejecting compression %d/%d", destSize + magic.headerSize, oldSize);
            }
        }
        else {
            OFLog(@"ZLIB: Failed to compress data, error code %d", error);
        }
        delete destBuffer;
    }
#endif
}

-(NSData*) internalUncompressedData {
#ifndef OF_EXCLUDE_ZLIB
    unsigned char *destBuffer = new unsigned char[self.uncompressedSize];
    size_t destSize = self.uncompressedSize;
    int error = uncompress(destBuffer, &destSize, (const Bytef*) [self.data bytes], [self.data length]);
    if(error == Z_OK) {        
        return [NSData dataWithBytes:destBuffer length:destSize];
    }
    else {
        OFLog(@"ZLIB: Failed to uncompress data, error code %d", error);
        return nil;
    }
    
#endif    
    return nil;
}

-(void) uncompress {
#ifndef OF_EXCLUDE_ZLIB
    if(self.uncompressedSize) {
        NSData *buffer = [self internalUncompressedData];
        if(buffer) {
            self.data = buffer;
            self.uncompressedSize = 0;
        }
    }
#endif
}

-(NSData*) uncompressedData {
    if(self.uncompressedSize == 0)
        return [[self.data copy] autorelease];
    else {
        return [self internalUncompressedData];
    }
}

-(NSData*) serializedData {
#ifdef OF_EXCLUDE_ZLIB
    return self.data;
#else
    //make a new NSData with length and data
    int size = self.uncompressedSize;
    if(size > 0) {    
        NSMutableData *retData = [NSMutableData dataWithBytes:magic.chars() length:magic.size];
        [retData appendBytes:&size length:sizeof(size_t)];
        [retData appendData:self.data];
        return retData;
    }
    else {
        return self.data;
    }
#endif
}

-(size_t) length {
    return self.uncompressedSize ? self.uncompressedSize : [self.data length];
}

#pragma mark configuration
+(void)setDisableCompression:(BOOL)disable {
    sDisableCompression = disable;
}
+(void)setVerbose:(BOOL)verbose {
    sVerbose = verbose;
}


#pragma mark Factories
+(OFCompressableData*) dataWithBytes:(const void*)_data length:(size_t)_length compress:(BOOL)_compressData {
    OFCompressableData *newData = [[OFCompressableData new] autorelease];
    newData.data = [NSData dataWithBytes:_data length:_length];
    newData.uncompressedSize = 0;
    if(_compressData)
        [newData compress];
    return newData;
}

+(OFCompressableData*) dataWithNSData:(NSData*)_data compress:(BOOL) _compressData {
    return [OFCompressableData dataWithBytes:[_data bytes] length:[_data length] compress:_compressData];
}

+(OFCompressableData*) dataWithSerializedData:(NSData*)_data {
#ifdef OF_EXCLUDE_ZLIB
    return [OFCompressableData dataWithNSData:_data compress:NO];
#else
    OFCompressableData *decompressor =[[OFCompressableData new] autorelease];
    char header[magic.size];    
    header[magic.size - 1] = 'x';  //if the data doesn't have 8 characters in it, then the header won't have anything copied into it, putting this here to avoid false positives
    [_data getBytes:&header length:magic.size];
    if(!memcmp(header, magic.chars(), magic.size)) {
        size_t size;
        [_data getBytes:&size range:NSMakeRange(magic.size,sizeof(size_t))];
        decompressor.uncompressedSize = size;
        decompressor.data = [NSData dataWithBytes:(char*)[_data bytes] + magic.headerSize length:[_data length]-magic.headerSize];
        
    }
    else {  //no header, so not compressed
        decompressor.uncompressedSize = 0;
        decompressor.data = [[_data copy] autorelease];
    }
    return decompressor;
#endif
}

#pragma mark Pass-Through filters
+(NSData*) uncompressedDataFromSerializedData:(NSData*)_serializedData {
#ifdef OF_EXCLUDE_ZLIB
    return _serializedData;
#else
    if(sDisableCompression) return _serializedData;
    OFCompressableData *compressor = [OFCompressableData dataWithSerializedData:_serializedData];
    return [compressor uncompressedData];
#endif
}

+(NSData*) serializedDataFromData:(NSData*)_data {
#ifdef OF_EXCLUDE_ZLIB
    return _data;
#else
    if(sDisableCompression) return _data;
    return [[OFCompressableData dataWithNSData:_data compress:YES] serializedData];
#endif
}

#pragma mark Testing
void testCheck(const char*str, NSData*data) {
    NSString *first = [NSString stringWithCString:str encoding:NSASCIIStringEncoding];
    NSString *second = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
    if(![first isEqualToString:second]) {
        OFLog(@"Test failed!");
    }
}

+(void)runTests {
    const char *testingText = "This is a bit of English test, since that seems like a generally useful"
    " bit of stuff that we can use to determine if compression is working properly.  We can't really use"
    " real random data, because that doesn't compress and I'm not sure of what files can be guaranteed"
    " to be on the device.  Why are you still reading this?  Go away!";
//    const char *testingText = "ABC";
    NSData *testData = [NSData dataWithBytes:testingText length:strlen(testingText)];
    OFCompressableData *testCompressor = [OFCompressableData dataWithNSData:testData compress:YES];
    NSData *testCopy = [testCompressor uncompressedData];
    testCheck(testingText, testCopy);
    [testCompressor uncompress];
    testCheck(testingText, testCompressor.data);
    OFCompressableData *testCompressor2 = [OFCompressableData dataWithBytes:testingText length:strlen(testingText) compress:YES];
    testCopy = [testCompressor2 uncompressedData];
    testCheck(testingText, testCopy);
    [testCompressor2 uncompress];
    testCheck(testingText, testCompressor2.data);
    
    //test the passthrough versions
    NSData *passthru = [OFCompressableData serializedDataFromData:testData];
    NSData *passReturn = [OFCompressableData uncompressedDataFromSerializedData:passthru];
    testCheck(testingText, passReturn);
    
    //test that non compressed data is handled properly
    NSData *notcomp = [testData copy];
    NSData *fromNC = [OFCompressableData uncompressedDataFromSerializedData:notcomp];
    testCheck(testingText, fromNC);

    OFCompressableData *checkNC2 = [OFCompressableData dataWithBytes:testingText length:strlen(testingText) compress:NO];
    NSData *checkNC2Serial = [checkNC2 serializedData];
    NSData *checkNC2final = [OFCompressableData uncompressedDataFromSerializedData:checkNC2Serial];
    testCheck(testingText, checkNC2final);
    
    //make sure it works with very small data sets, too
    const char *tinyText="ABC";
    NSData *toosmall = [NSData dataWithBytes:tinyText length:strlen(tinyText)];
    NSData *serializeTooSmall = [OFCompressableData serializedDataFromData:toosmall];
    NSData *fromTS = [OFCompressableData uncompressedDataFromSerializedData:serializeTooSmall];
    testCheck(tinyText, fromTS);
}


@end

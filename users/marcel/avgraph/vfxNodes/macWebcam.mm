#if 1

#import "Debugging.h"
#import "Log.h"
#import "macWebcam.h"
#import "Timer.h"

#import <AVFoundation/AVFoundation.h>
#include <SDL2/SDL.h>

@interface MacWebcamImpl : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

@end

@implementation MacWebcamImpl

AVCaptureSession * session;
AVCaptureVideoDataOutput * videoOutput;
dispatch_queue_t queue;
MacWebcam * webcam = nullptr;
int nextImageIndex = 0;

- (id)init
{
	return self;
}

- (bool)initWebcam:(MacWebcam*)webcam
{
	if ([self doInitWebcam:webcam] == false)
	{
		[self shut];
		
		return false;
	}
	else
	{
		return true;
	}
}

- (void)configureSession
{
	NSError * error = nullptr;
	
	uint64_t ts1 = g_TimerRT.TimeUS_get();
	
	session = [[AVCaptureSession alloc] init];
	
	uint64_t ts2 = g_TimerRT.TimeUS_get();
	
	uint64_t tc1 = g_TimerRT.TimeUS_get();
	
#if 0
	[session beginConfiguration];
	
	if (true && [session canSetSessionPreset:AVCaptureSessionPreset1280x720])
	{
		session.sessionPreset = AVCaptureSessionPreset1280x720;
	}
	else if ([session canSetSessionPreset:AVCaptureSessionPreset640x480])
	{
		session.sessionPreset = AVCaptureSessionPreset640x480;
	}
	else
	{
		return;
	}
	
	[session commitConfiguration];
#endif

	uint64_t tc2 = g_TimerRT.TimeUS_get();
	
	uint64_t td1 = g_TimerRT.TimeUS_get();
	
	AVCaptureDevice * device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	
	if (device == nullptr)
	{
		return;
	}
	
	uint64_t td2 = g_TimerRT.TimeUS_get();
	
	uint64_t ti1 = g_TimerRT.TimeUS_get();
	
	AVCaptureDeviceInput * input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
	
	if (input == nullptr)
	{
		return;
	}
	
	[session addInput:input];
	
	uint64_t ti2 = g_TimerRT.TimeUS_get();
	
	uint64_t tq1 = g_TimerRT.TimeUS_get();
	
	//queue = dispatch_queue_create("VideoOutputQueue", DISPATCH_QUEUE_SERIAL);
	
	uint64_t tq2 = g_TimerRT.TimeUS_get();
	
	uint64_t to1 = g_TimerRT.TimeUS_get();
	
	videoOutput = [[AVCaptureVideoDataOutput alloc] init];
	//kCVPixelFormatType_32ABGR
	// todo : setting videoSettings here overrides session preset params like resolution
	// todo : figure out how to set resolution on output
	// todo : figure out how to set desired framerate (60 fps)
	// todo : figure out what's the most efficient output format. by default it doesn't output 32 bit BGRA
	
#if 1
NSDictionary * newSettings =
		@{
			(NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
		};
#else
	NSDictionary * newSettings =
		@{
			(NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
			(NSString *)kCVPixelBufferWidthKey : @(640),
			(NSString *)kCVPixelBufferHeightKey : @(480)
		};
#endif
	
	/*
	NSDictionary * oldSettings = videoOutput.videoSettings;
	
	NSArray * pixelFormats = [videoOutput availableVideoCVPixelFormatTypes];
	printf("%s\n", [[oldSettings description] cStringUsingEncoding:NSASCIIStringEncoding]);
	printf("%s\n", [[newSettings description] cStringUsingEncoding:NSASCIIStringEncoding]);
	printf("%s\n", [[pixelFormats description] cStringUsingEncoding:NSASCIIStringEncoding]);
	
	for (int i = 0; i < pixelFormats.count; ++i)
	{
		const uint32_t value = [[pixelFormats objectAtIndex:i] integerValue];
		
		const char c1 = (value & (0xff << 0)) >> 0;
		const char c2 = (value & (0xff << 8)) >> 8;
		const char c3 = (value & (0xff << 16)) >> 16;
		const char c4 = (value & (0xff << 24)) >> 24;
		
		printf("%u: %c%c%c%c\n", value, c4, c3, c2, c1);
	}
	*/
	
	videoOutput.videoSettings = newSettings;
	[videoOutput setAlwaysDiscardsLateVideoFrames:YES];

	[videoOutput setSampleBufferDelegate:self queue:queue];

	if ([session canAddOutput:videoOutput])
	{
		[session addOutput:videoOutput];
	}
	else
	{
		return;
	}
	
	uint64_t to2 = g_TimerRT.TimeUS_get();
	
	uint64_t tr1 = g_TimerRT.TimeUS_get();
	
	[session startRunning];
	
	uint64_t tr2 = g_TimerRT.TimeUS_get();
	
	LOG_DBG("ts: %.2fms, td: %.2fms, ti: %.2fms, to: %.2fms, tq: %.2fms, tr: %.2fms",
		(ts2 - ts1) / 1000.0,
		(td2 - td1) / 1000.0,
		(ti2 - ti1) / 1000.0,
		(to2 - ti1) / 1000.0,
		(tq2 - tq1) / 1000.0,
		(tr2 - tr1) / 1000.0);
	
	NSArray * supportedFrameRateRanges = device.activeFormat.videoSupportedFrameRateRanges;
	
	for (AVFrameRateRange * frr in supportedFrameRateRanges)
	{
		LOG_DBG("frateRateRange: %.2f - %.2f", frr.minFrameRate, frr.maxFrameRate);
	}
}

- (bool)doInitWebcam:(MacWebcam*)_webcam
{
	webcam = _webcam;
	
	queue = dispatch_queue_create("VideoOutputQueue", DISPATCH_QUEUE_SERIAL);
	
	dispatch_async( queue, ^{
		[self configureSession];
    });
	
	return true;
}

- (void)shut
{
	if (session != nullptr)
	{
		[session stopRunning];
	}
	
	if (videoOutput != nullptr)
	{
		[videoOutput release];
		videoOutput = nullptr;
	}

    if (queue != 0)
    {
		dispatch_sync(queue, ^{ });
		
        dispatch_release(queue);
        queue = 0;
    }
	
	if (videoOutput != nullptr)
	{
		[videoOutput release];
		videoOutput = nullptr;
	}
	
	if (session != nullptr)
	{
		[session release];
		session = nullptr;
	}
	
	webcam = nullptr;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
     didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
     fromConnection:(AVCaptureConnection *)connection
{
	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	CVPixelBufferLockBaseAddress(imageBuffer, 0);
	
	const size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    const size_t sx = CVPixelBufferGetWidth(imageBuffer);
    const size_t sy = CVPixelBufferGetHeight(imageBuffer);

	uint8_t * baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(imageBuffer);
	
	MacWebcamImage * image = new MacWebcamImage(sx, sy);
	
	image->pitch = (sx * 4 + 15) & (~15);
	image->index = nextImageIndex++;
	
	// todo : benchmark SSE optimized version
	
#if 1
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcPtr = baseAddress + y * bytesPerRow;
			  uint8_t * __restrict dstPtr = image->data + y * image->pitch;
		
		const int sx8 = sx/8;
		
		const __m128i * __restrict srcItr = (__m128i*)srcPtr;
			  __m128i * __restrict dstItr = (__m128i*)dstPtr;
		
		for (int x = 0; x < sx8; ++x)
		{
			const __m128i src1 = _mm_load_si128(srcItr++);
			const __m128i src2 = _mm_load_si128(srcItr++);
			
			//
			
			__m128i srcL1 = _mm_unpacklo_epi8(src1, _mm_setzero_si128());
			__m128i srcH1 = _mm_unpackhi_epi8(src1, _mm_setzero_si128());
			srcL1 = _mm_shufflelo_epi16(srcL1, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			srcL1 = _mm_shufflehi_epi16(srcL1, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			srcH1 = _mm_shufflelo_epi16(srcH1, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			srcH1 = _mm_shufflehi_epi16(srcH1, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			const __m128i dst1 = _mm_packus_epi16(srcL1, srcH1);
			
			//
			
			__m128i srcL2 = _mm_unpacklo_epi8(src2, _mm_setzero_si128());
			__m128i srcH2 = _mm_unpackhi_epi8(src2, _mm_setzero_si128());
			srcL2 = _mm_shufflelo_epi16(srcL2, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			srcL2 = _mm_shufflehi_epi16(srcL2, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			srcH2 = _mm_shufflelo_epi16(srcH2, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			srcH2 = _mm_shufflehi_epi16(srcH2, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
			const __m128i dst2 = _mm_packus_epi16(srcL2, srcH2);
			
			//
			
			_mm_store_si128(&dstItr[x * 2 + 0], dst1);
			_mm_store_si128(&dstItr[x * 2 + 1], dst2);
		}
		
		for (int x = sx8 * 8; x < sx; ++x)
		{
			dstPtr[x * 4 + 0] = srcPtr[x * 4 + 2];
			dstPtr[x * 4 + 1] = srcPtr[x * 4 + 1];
			dstPtr[x * 4 + 2] = srcPtr[x * 4 + 0];
			dstPtr[x * 4 + 3] = srcPtr[x * 4 + 3];
		}
	}
#else
	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcPtr = baseAddress + y * bytesPerRow;
			  uint8_t * __restrict dstPtr = image->data + y * image->pitch;
		
		for (int x = 0; x < sx; ++x)
		{
			dstPtr[x * 4 + 0] = srcPtr[x * 4 + 2];
			dstPtr[x * 4 + 1] = srcPtr[x * 4 + 1];
			dstPtr[x * 4 + 2] = srcPtr[x * 4 + 0];
			dstPtr[x * 4 + 3] = srcPtr[x * 4 + 3];
		}
	}
#endif
	
	CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
	
	MacWebcamImage * oldImage1 = nullptr;
	MacWebcamImage * oldImage2 = nullptr;
	
	SDL_LockMutex(webcam->mutex);
	{
		oldImage1 = webcam->newImage;
		webcam->newImage = nullptr;
		
		oldImage2 = webcam->oldImage;
		webcam->oldImage = nullptr;
		
		Assert(webcam->newImage == nullptr);
		webcam->newImage = image;
	}
	SDL_UnlockMutex(webcam->mutex);
	
	delete oldImage1;
	oldImage1 = nullptr;
	
	delete oldImage2;
	oldImage2 = nullptr;
}

@end

MacWebcamImage::MacWebcamImage(const int _sx, const int _sy)
	: index(-1)
	, sx(_sx)
	, sy(_sy)
	, pitch(0)
	, data(nullptr)
{
	data = (uint8_t*)_mm_malloc(sx * sy * 4, 16);
}

MacWebcamImage::~MacWebcamImage()
{
	_mm_free(data);
	data = nullptr;
}

MacWebcam::MacWebcam()
	: webcamImpl(nullptr)
	, image(nullptr)
	, newImage(nullptr)
	, oldImage(nullptr)
	, mutex(nullptr)
{
}

MacWebcam::~MacWebcam()
{
	shut();
}

bool MacWebcam::init()
{
	shut();
	
	//
	
	mutex = SDL_CreateMutex();
	
	MacWebcamImpl * impl = [[MacWebcamImpl alloc] init];

	if ([impl initWebcam:this] == false)
	{
		[impl release];
		impl = nullptr;
	}

	webcamImpl = impl;
	
	return impl != nullptr;
}

void MacWebcam::shut()
{
	MacWebcamImpl * impl = (MacWebcamImpl*)webcamImpl;

	if (impl != nullptr)
	{
		[impl shut];
		
		[impl release];
		impl = nullptr;
	}

	webcamImpl = impl;
	
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
	
	delete image;
	image = nullptr;
	
	delete newImage;
	newImage = nullptr;
	
	delete oldImage;
	oldImage = nullptr;
}

void MacWebcam::tick()
{
	SDL_LockMutex(mutex);
	{
		if (newImage != nullptr)
		{
			Assert(oldImage == nullptr);
			oldImage = image;
			
			image = newImage;
			newImage = nullptr;
		}
	}
	SDL_UnlockMutex(mutex);
}

#endif

#ifdef MACOS

#import "Debugging.h"
#import "Log.h"
#import "macWebcam.h"
#import "Timer.h"

#import <AVFoundation/AVFoundation.h>
#include <SDL2/SDL.h>

@interface MacWebcamImpl : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
	@property (assign) AVCaptureSession * session;
	@property (assign) AVCaptureDevice * device;
	@property (assign) AVCaptureDeviceInput * deviceInput;
	@property (assign) AVCaptureVideoDataOutput * videoOutput;
	@property (assign) dispatch_queue_t queue;
	@property MacWebcamContext * webcamContext;
	@property int nextImageIndex;
@end

@implementation MacWebcamImpl
	@synthesize session;
	@synthesize device;
	@synthesize deviceInput;
	@synthesize videoOutput;
	@synthesize queue;
	@synthesize webcamContext;
	@synthesize nextImageIndex;

- (id)init
{
	[super init];
	
	return self;
}

- (bool)initWebcam:(MacWebcamContext*)_webcamContext
{
	if ([self initContext:_webcamContext] == false || [self initSession] == false)
	{
		[self shut];
		
		return false;
	}
	else
	{
		return true;
	}
}

- (bool)initContext:(MacWebcamContext*)_webcamContext
{
	Assert(webcamContext == nullptr);
	webcamContext = _webcamContext;
	
	Assert(queue == 0);
	queue = dispatch_queue_create("VideoOutputQueue", DISPATCH_QUEUE_SERIAL);
	
	return true;
}

- (bool)initSession
{
	NSError * error = nullptr;
	
	// allocate a new AV capture session
	
	Assert(session == nullptr);
	session = [[AVCaptureSession alloc] init];
	
	[session beginConfiguration];
	
	// get the default capture device
	// todo : select the correct capture device
	
	device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	
	if (device == nullptr)
	{
		return false;
	}
	
	// get the input associated with the device
	
	deviceInput = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
	
	if (deviceInput == nullptr)
	{
		return false;
	}
	
	[session addInput:deviceInput];
	
	// add a video data output. this will let us capture the video feed as pixel data
	
	videoOutput = [[AVCaptureVideoDataOutput alloc] init];
	
	// todo : figure out what's the most efficient output format. by default it doesn't output 32 bit BGRA. perhaps we should capture RGBA when available. or YUV when not. what is most efficient for the OS/driver ?
	
	NSMutableDictionary * newSettings = [[[NSMutableDictionary alloc] init] autorelease];
	
	[newSettings setValue:@(kCVPixelFormatType_32BGRA) forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];
	
	if ([session canSetSessionPreset:AVCaptureSessionPreset640x480])
	{
		[newSettings setValue:@640 forKey:(NSString *)kCVPixelBufferWidthKey];
		[newSettings setValue:@480 forKey:(NSString *)kCVPixelBufferHeightKey];
	}
	else if ([session canSetSessionPreset:AVCaptureSessionPreset1280x720])
	{
		[newSettings setValue:@1280 forKey:(NSString *)kCVPixelBufferWidthKey];
		[newSettings setValue:@720 forKey:(NSString *)kCVPixelBufferHeightKey];
	}
	
#if 0
	// print the current settings and list the available pixel formats
	
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
#endif
	
	videoOutput.videoSettings = newSettings;
	[videoOutput setAlwaysDiscardsLateVideoFrames:YES];
	[videoOutput setSampleBufferDelegate:self queue:queue];

	if ([session canAddOutput:videoOutput])
	{
		[session addOutput:videoOutput];
	}
	else
	{
		return false;
	}
	
	[session commitConfiguration];
	
	// begin capturing !
	
	[session startRunning];
	
	return true;
}

- (void)shut
{
	LOG_DBG("shut", 0);
	
	[self shutSession];
	
	if (queue != 0)
	{
		dispatch_barrier_sync(queue, ^{});
	}
	
	[self shutContext];
}

- (void)shutSession
{
	LOG_DBG("shutSession", 0);
	
	if (session != nullptr)
	{
		[session stopRunning];
	}
	
	if (videoOutput != nullptr)
	{
		[videoOutput release];
		videoOutput = nullptr;
	}
	
	if (deviceInput != nullptr)
	{
		deviceInput = nullptr;
	}
	
	if (device != nullptr)
	{
		device = nullptr;
	}
	
	if (session != nullptr)
	{
		[session release];
		session = nullptr;
	}
}

- (void)shutContext
{
	LOG_DBG("shutContext", 0);
	
    if (queue != 0)
    {
        dispatch_release(queue);
        queue = 0;
    }
}

static __m128i swizzle(const __m128i src)
{
	__m128i srcL = _mm_unpacklo_epi8(src, _mm_setzero_si128());
	__m128i srcH = _mm_unpackhi_epi8(src, _mm_setzero_si128());
	
	srcL = _mm_shufflelo_epi16(srcL, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
	srcL = _mm_shufflehi_epi16(srcL, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
	
	srcH = _mm_shufflelo_epi16(srcH, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
	srcH = _mm_shufflehi_epi16(srcH, (2 << 0) | (1 << 2) | (0 << 4) | (3 << 6));
	
	return _mm_packus_epi16(srcL, srcH);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
     didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
     fromConnection:(AVCaptureConnection *)connection
{
	//LOG_DBG("captureOutput", 0);
	
	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	
	const size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    const size_t sx = CVPixelBufferGetWidth(imageBuffer);
    const size_t sy = CVPixelBufferGetHeight(imageBuffer);
	
	//
	
	MacWebcamImage * image = new MacWebcamImage(sx, sy);
	
	image->pitch = (sx * 4 + 15) & (~15);
	image->index = nextImageIndex++;
	
	//

	CVPixelBufferLockBaseAddress(imageBuffer, 0);
	uint8_t * baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(imageBuffer);
	
	//
	
	uint64_t t1 = g_TimerRT.TimeUS_get();
	
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
			const __m128i src1 = _mm_load_si128(srcItr + 0);
			const __m128i src2 = _mm_load_si128(srcItr + 1);
			
			srcItr += 2;
			
			//
			
			const __m128i dst1 = swizzle(src1);
			const __m128i dst2 = swizzle(src2);
			
			//
			
			_mm_store_si128(dstItr + 0, dst1);
			_mm_store_si128(dstItr + 1, dst2);
			
			dstItr += 2;
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
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	const uint64_t dt = t2 - t1;
	webcamContext->conversionTimeUsAvg = (webcamContext->conversionTimeUsAvg * 90 + dt * 10) / 100;
	
	//
	
	CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
	
	//
	
	MacWebcamImage * oldImage1 = nullptr;
	MacWebcamImage * oldImage2 = nullptr;
	
	SDL_LockMutex(webcamContext->mutex);
	{
		oldImage1 = webcamContext->newImage;
		webcamContext->newImage = nullptr;
		
		oldImage2 = webcamContext->oldImage;
		webcamContext->oldImage = nullptr;
		
		Assert(webcamContext->newImage == nullptr);
		webcamContext->newImage = image;
	}
	SDL_UnlockMutex(webcamContext->mutex);
	
	//
	
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

//

MacWebcamContext::MacWebcamContext()
	: newImage(nullptr)
	, oldImage(nullptr)
	, cond(nullptr)
	, mutex(nullptr)
	, stop(false)
	, conversionTimeUsAvg(0)

{
	cond = SDL_CreateCond();
	
	mutex = SDL_CreateMutex();
}

MacWebcamContext::~MacWebcamContext()
{
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
	
	SDL_DestroyCond(cond);
	cond = nullptr;
	
	delete newImage;
	newImage = nullptr;
	
	delete oldImage;
	oldImage = nullptr;
}

//

static int MacWebcamThreadMain(void * obj)
{
	MacWebcamContext * context = (MacWebcamContext*)obj;
	
	MacWebcamImpl * webcamImpl = [[MacWebcamImpl alloc] init];

	if ([webcamImpl initWebcam:context] == false)
	{
		LOG_DBG("failed to init webcam", 0);
		
		[webcamImpl release];
		webcamImpl = nullptr;
		
		SDL_LockMutex(context->mutex);
		{
			if (context->stop == false)
			{
				SDL_CondWait(context->cond, context->mutex);
			}
		}
		SDL_UnlockMutex(context->mutex);
	}
	else
	{
		SDL_LockMutex(context->mutex);
		{
			if (context->stop == false)
			{
				SDL_CondWait(context->cond, context->mutex);
			}
		}
		SDL_UnlockMutex(context->mutex);
		
		[webcamImpl shut];
		
		[webcamImpl release];
		webcamImpl = nullptr;
	}
	
	delete context;
	context = nullptr;
	
	return 0;
}

//

MacWebcam::MacWebcam(const bool _threaded)
	: context(nullptr)
	, image(nullptr)
	, threaded(_threaded)
	, nonThreadedWebcamImpl(nullptr)
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
	
	Assert(context == nullptr);
	context = new MacWebcamContext();
	
	if (threaded)
	{
		SDL_Thread * thread = SDL_CreateThread(MacWebcamThreadMain, "MacWebcam", context);
		SDL_DetachThread(thread);
	}
	else
	{
		MacWebcamImpl * webcamImpl = [[MacWebcamImpl alloc] init];

		if ([webcamImpl initWebcam:context] == false)
		{
			LOG_DBG("failed to init webcam", 0);
			
			[webcamImpl release];
			webcamImpl = nullptr;
		}
		
		nonThreadedWebcamImpl = webcamImpl;
	}
	
	return true;
}

void MacWebcam::shut()
{
	if (context != nullptr)
	{
		if (threaded)
		{
			// tell webcam thread to clean up after itself
			
			SDL_LockMutex(context->mutex);
			{
				context->stop = true;
				
				SDL_CondSignal(context->cond);
			}
			SDL_UnlockMutex(context->mutex);
			
			context = nullptr;
		}
		else
		{
			MacWebcamImpl * webcamImpl = (MacWebcamImpl*)nonThreadedWebcamImpl;
			
			[webcamImpl shut];
			
			[webcamImpl release];
			webcamImpl = nullptr;
			
			nonThreadedWebcamImpl = nullptr;
			
			delete context;
			context = nullptr;
		}
	}
	
	// clean up our own stuff
	
	delete image;
	image = nullptr;
}

void MacWebcam::tick()
{
	if (context == nullptr)
		return;
	
	SDL_LockMutex(context->mutex);
	{
		if (context->newImage != nullptr)
		{
			Assert(context->oldImage == nullptr);
			context->oldImage = image;
			
			image = context->newImage;
			context->newImage = nullptr;
		}
	}
	SDL_UnlockMutex(context->mutex);
}

#endif

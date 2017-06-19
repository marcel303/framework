#if 1

#import "macWebcam.h"

#import <AVFoundation/AVFoundation.h>

@interface MacWebcamImpl : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

@end

@implementation MacWebcamImpl

AVCaptureSession * session;
AVCaptureVideoDataOutput * videoOutput;
dispatch_queue_t queue;
MacWebcamImage * image = nullptr;
int nextImageIndex = 0;

- (id)init
{
	return self;
}

- (bool)initWebcam:(MacWebcamImage*)image
{
	if ([self doInitWebcam:image] == false)
	{
		[self shut];
		
		return false;
	}
	else
	{
		return true;
	}
}

- (bool)doInitWebcam:(MacWebcamImage*)_image
{
	NSError * error = nullptr;

	image = _image;

	session = [[AVCaptureSession alloc] init];

	if ([session canSetSessionPreset:AVCaptureSessionPreset1280x720])
	{
		session.sessionPreset = AVCaptureSessionPreset1280x720;
	}
	else if ([session canSetSessionPreset:AVCaptureSessionPreset640x480])
	{
		session.sessionPreset = AVCaptureSessionPreset640x480;
	}
	else
	{
		return false;
	}

	AVCaptureDevice * device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	
	if (device == nullptr)
	{
		return false;
	}

	AVCaptureDeviceInput * input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
	
	if (input == nullptr)
	{
		return false;
	}
	
	[session addInput:input];

	videoOutput = [[AVCaptureVideoDataOutput alloc] init];

	NSDictionary * settings = @{ (NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA) };
	videoOutput.videoSettings = settings;
	[videoOutput setAlwaysDiscardsLateVideoFrames:YES];

	queue = dispatch_queue_create("VideoOutputQueue", DISPATCH_QUEUE_SERIAL);

	[videoOutput setSampleBufferDelegate:self queue:queue];

	dispatch_release(queue);

	if ([session canAddOutput:videoOutput])
	{
		[session addOutput:videoOutput];
	}
	else
	{
		return false;
	}

	[session startRunning];
	
	return true;
}

- (void)shut
{
	[session stopRunning];
	
	[videoOutput release];
	videoOutput = nullptr;

    if (queue)
    {
        dispatch_release(queue);
        queue = 0;
    }

    [videoOutput release];
    videoOutput = nullptr;

    [session release];
    session = nullptr;
	
	image = nullptr;
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
	
	const int numBytes = bytesPerRow * sy;
	
	if (numBytes <= sizeof(image->data))
	{
		image->sx = sx;
		image->sy = sy;
		image->pitch = sx * 4;
		image->index = nextImageIndex++;
		
		// todo : write SSE optimized version
		// todo : see if we can let webcam output to RGBA directly
		
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
	}
	
	CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
}

@end

MacWebcam::MacWebcam()
	: webcamImpl(nullptr)
	, image()
	, gotImage(false)
{
}

bool MacWebcam::init()
{
	shut();

	//

	MacWebcamImpl * impl = [[MacWebcamImpl alloc] init];

	if ([impl initWebcam:&image] == false)
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
}

#endif

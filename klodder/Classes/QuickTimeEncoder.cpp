#if USE_QUICKTIME

QtmlClient.lib
CVClient.lib

#include <wtypes.h>
#include "Exception.h"
#include "QuickTimeEncoder.h"

#define MAKEFOURCC(a, b, c, d) ( ((unsigned long)a) | (((unsigned long)b)<< 8) | (((unsigned long)c)<<16) | (((unsigned long)d)<<24) ) 

QuickTimeEncoder::QuickTimeEncoder()
{
	mIsInitialized = false;
	mVideoCodec = QtVideoCodec_Undefined;
	mVideoQuality = QtVideoQuality_Undefined;
	mImage = 0;
	mQtCodecType = 0;
	mQtCodecQuality = 0;

	// Movie stuff
	mMovie = 0;
	mTrack = 0;
	mMedia = 0;
	mDataHandler = 0;

	// Encoder stuff
	mTime = 0;
	mSession = 0;
	mPixelBuffer = 0;
	mPixelDataPtr = 0;

	// Errors
	mError = noErr;
	mStatus = 0;
	mCvReturn = kCVReturnSuccess;
}

OSStatus QuickTimeEncoder::OutputCallback(void* obj, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef frame, void* reserved)
{
	QuickTimeEncoder* self = (QuickTimeEncoder*)obj;

	try
	{
		self->MovieAppend(frame);

		return noErr;
	}
	catch (std::exception&)
	{
		return -1000;
	}
}

void QuickTimeEncoder::Initialize(const char* fileName, QtVideoCodec videoCodec, QtVideoQuality videoQuality, MacImage* image)
{
	mIsInitialized = true;

	mFileName = fileName;
	mVideoCodec = videoCodec;
	mVideoQuality = videoQuality;
	mImage = image;

	mRect.left = 0;
	mRect.right = image->Sx_get();
	mRect.top = 0;
	mRect.bottom = image->Sy_get();

	// translate codec/quality enum to QuickTime values

	switch (mVideoCodec)
	{
	case QtVideoCodec_H264:
		mQtCodecType = kH264CodecType;
		break;
	case QtVideoCodec_Jpeg:
		mQtCodecType = kJPEGCodecType;
		break;
	case QtVideoCodec_Jpeg2000:
		mQtCodecType = kJPEG2000CodecType;
		break;
	default:
		throw ExceptionVA("unknown video codec: %d", (int)mVideoCodec);
	}

	switch (mVideoQuality)
	{
	case QtVideoQuality_High:
		mQtCodecQuality = codecHighQuality;
		break;
	case QtVideoQuality_Low:
		mQtCodecQuality = codecLowQuality;
		break;
	case QtVideoQuality_Medium:
		mQtCodecQuality = codecNormalQuality;
		break;
	default:
		throw ExceptionVA("unknown video quality: %d", (int)mVideoQuality);
	}

	MovieBegin();
	EncoderBegin();
}

void QuickTimeEncoder::Shutdown()
{
	mIsInitialized = false;

	EncoderEnd();
	MovieEnd();
}

void QuickTimeEncoder::CommitVideoFrame(int durationMS)
{
	for (int y = 0; y < mImage->Sy_get(); ++y)
	{
		const MacRgba* srcLine = mImage->Line_get(mImage->Sy_get() - 1 - y);
		uint8_t* dstLine = mPixelDataPtr + y * mImage->Sx_get() * 3;

		for (int x = 0; x < mImage->Sx_get(); ++x)
		{
			dstLine[0] = srcLine->rgba[0];
			dstLine[1] = srcLine->rgba[1];
			dstLine[2] = srcLine->rgba[2];

			srcLine++;
			dstLine += 3;
		}
	}

	mStatus = ICMCompressionSessionEncodeFrame(
			mSession,
			mPixelBuffer,
			mTime,
			durationMS,
			kICMValidTime_DisplayDurationIsValid | kICMValidTime_DisplayTimeStampIsValid,
			0,
			0,
			0);
	CheckErrors("unable to encode video frame");

	mTime += durationMS;
}

bool QuickTimeEncoder::IsInitialized_get() const
{
	return mIsInitialized;
}

void QuickTimeEncoder::MovieBegin()
{
	// initialize QuickTime

	mError = InitializeQTML(0);
	CheckErrors("unable to initialize QuickTime");

	mError = EnterMovies();
	CheckErrors("unable to initialize QuickTime");

	// calculate absolute filename

	char fileNameAbs[MAX_PATH];

	if(!_fullpath(fileNameAbs, mFileName.c_str(), MAX_PATH))
		throw ExceptionVA("unable to determine absolute path");

	c2pstrcpy(mFileNameAbs255, fileNameAbs);

	// open movie file

	FSSpec mFileSpec;
	
	//mError = FSMakeFSSpec(0, 0, mFileNameAbs255, &mFileSpec);
	FSMakeFSSpec(0, 0, mFileNameAbs255, &mFileSpec);
	//CheckErrors("unable to create FS spec");

	// setup movie storage

	Handle data = 0;
	OSType dataType;
	
	mError = QTNewDataReferenceFromFSSpec(&mFileSpec, 0, &data, &dataType);
	CheckErrors("unable to create new data from FS spec");

	mError = CreateMovieStorage(
		data,
		dataType,
		MAKEFOURCC('T', 'V', 'O', 'D'),// 'TVOD', 
		smAllScripts, 
		createMovieFileDeleteCurFile | createMovieFileDontCreateResFile | newMovieActive,
		&mDataHandler, &mMovie);
	CheckErrors("unable to create movie storage");

	DisposeHandle(data);

	// create new movie track

	mTrack = NewMovieTrack(
		mMovie,
		FixRatio(mImage->Sx_get(), 1),
		FixRatio(mImage->Sy_get(), 1), 0);
	mError = GetMoviesError();
	CheckErrors("unable to create movie track");

	if (mTrack == 0)
		throw ExceptionVA("unable to create movie track");

	// create track media

	mMedia = NewTrackMedia(
		mTrack,
		VideoMediaType,
		1000, 0, 0);
	mError = GetMoviesError();
	CheckErrors("unable to create track media");

	if (mMedia == 0)
		throw ExceptionVA("unable to create track media");

	// begin editing

	mError = BeginMediaEdits(mMedia);
	CheckErrors("unable to begin media edits");
}

void QuickTimeEncoder::MovieEnd()
{
	/// finish creating movie

	short resId = movieInDataForkResID;

	LOG_DBG("ending media edits", 0);

	if (mMedia) 
	{
		mError = EndMediaEdits(mMedia);
		CheckErrors("unable to end media edits");

		mError = ExtendMediaDecodeDurationToDisplayEndTime(mMedia, 0);
		CheckErrors("unable to extend decode duration to display time");
	}

	LOG_DBG("adding media to track", 0);

	if (mTrack)
	{
		TimeValue duration = GetMediaDuration(mMedia);

		mError = InsertMediaIntoTrack(
			mTrack,
			0,
			0,
			duration,
			fixed1);
		CheckErrors("unable to insert media into track");
	}

	LOG_DBG("adding movie storage", 0);

	if (mDataHandler)
	{
		mError = AddMovieToStorage(mMovie, mDataHandler);
		CheckErrors("unable to add movie to storage");

		mError = CloseMovieStorage(mDataHandler);
		CheckErrors("unable to close movie storage");
		mDataHandler = 0;
	}

	// free memory

	if (mMovie)
	{
		DisposeMovie(mMovie);
		mMovie = 0;
	}

	// shutdown QuickTime

	ExitMovies();

	TerminateQTML();
}

void QuickTimeEncoder::MovieAppend(ICMEncodedFrameRef frame)
{
	OSErr err = 0;

	TimeValue64 duration = ICMEncodedFrameGetDecodeDuration(frame);

	if (duration > 0)
	{
		if (ICMEncodedFrameGetDataSize(frame) > 0)
		{
			mError = AddMediaSampleFromEncodedFrame(mMedia, frame, 0); 
			CheckErrors("unable to add encoded frame to media");
		}
		else
		{
			LOG_DBG("frame data size is 0", 0);
		}
	}
	else
	{
		LOG_DBG("frame duration is 0", 0);
	}
}

void QuickTimeEncoder::EncoderBegin()
{
	// create compression session options

	ICMCompressionSessionOptionsRef options = 0;

	mStatus = ICMCompressionSessionOptionsCreate(
		kCFAllocatorDefault,
		&options);
	CheckErrors("unable to creation compression session options");

	mStatus = ICMCompressionSessionOptionsSetAllowTemporalCompression(options, true); // P+B frames
	CheckErrors("unable to set temporal compression option");
	mStatus = ICMCompressionSessionOptionsSetAllowFrameReordering(options, true); // B frames
	CheckErrors("unable to set frame reordering option");
	mStatus = ICMCompressionSessionOptionsSetMaxKeyFrameInterval(options, 30);
	CheckErrors("unable to set max key frame interval option");
	mStatus = ICMCompressionSessionOptionsSetAllowFrameTimeChanges(options, true); // allow ICM to drop/coalesc frames
	CheckErrors("unable to set time changes option");
	mStatus = ICMCompressionSessionOptionsSetDurationsNeeded(options, true);
	CheckErrors("unable to set duration needed option");

	mStatus = ICMCompressionSessionOptionsSetProperty(options, 
		kQTPropertyClass_ICMCompressionSessionOptions,
		kICMCompressionSessionOptionsPropertyID_Quality,
		sizeof(mQtCodecQuality),
		&mQtCodecQuality);
	CheckErrors("unable to set compression quality option");

	ICMMultiPassStorageRef storage = 0;
	mStatus = ICMCompressionSessionOptionsSetProperty(options, kQTPropertyClass_ICMCompressionSessionOptions, kICMCompressionSessionOptionsPropertyID_MultiPassStorage, sizeof(ICMMultiPassStorageRef), &storage);
	CheckErrors("unable to set multi pass storage option");

	Boolean async = FALSE;
	mStatus = ICMCompressionSessionOptionsSetProperty(options, kQTPropertyClass_ICMCompressionSessionOptions, kICMCompressionSessionOptionsPropertyID_AllowAsyncCompletion, sizeof(Boolean), &async);
	CheckErrors("unable to set asynchronous completion option");

	// create compression session

	ICMEncodedFrameOutputRecord record = { 0 };

	record.encodedFrameOutputCallback = OutputCallback;
    record.encodedFrameOutputRefCon = this;
    record.frameDataAllocator = 0;

	mStatus = ICMCompressionSessionCreate(
		kCFAllocatorDefault,
		mImage->Sx_get(),
		mImage->Sy_get(),
		mQtCodecType,
		1000,
		options,
		NULL,
		&record,
		&mSession);
	CheckErrors("unable to creation compression session");

	ICMCompressionSessionOptionsRelease(options);

	// create pixel buffer

	mCvReturn = CVPixelBufferCreate(
		kCFAllocatorDefault,
		mImage->Sx_get(),
		mImage->Sy_get(),
		k24RGBPixelFormat,
		//k32RGBAPixelFormat, // fails :(
		0,
		&mPixelBuffer);
	CheckErrors("unable to create CV pixel buffer");

	mCvReturn = CVPixelBufferLockBaseAddress(mPixelBuffer, 0);
	CheckErrors("unable to lock CV pixel buffer base address");
	mPixelDataPtr = (uint8_t*)CVPixelBufferGetBaseAddress(mPixelBuffer);
	if (!mPixelDataPtr)
		throw ExceptionVA("unable to get pixel buffer base address");
}

void QuickTimeEncoder::EncoderEnd()
{
	CVPixelBufferUnlockBaseAddress(mPixelBuffer, 0);

	// complete frames

	mStatus = ICMCompressionSessionCompleteFrames(
		mSession,
		true,
		0,
		0);
	CheckErrors("unable to complete frames");
}

void QuickTimeEncoder::CheckErrors(const char* desc)
{
	if (mError != noErr)
		throw ExceptionVA("error code: %s: %d", desc, (int)mError);
	if (mStatus != 0)
		throw ExceptionVA("error status: %s: %d", desc, (int)mStatus);
	if (mCvReturn != kCVReturnSuccess)
		throw ExceptionVA("error CV return: %s: %d", desc, (int)mCvReturn);
}

#endif

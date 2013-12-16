#include <exception>
#include <movies.h>
#include <stdio.h>
#include <string>
#include <qtml.h>
#include <wtypes.h>

class ExceptionVA : std::exception
{
public:
	ExceptionVA(const char* text, ...)
	{
		mWhat = text;
	}

	virtual const char* what()
	{
		return mWhat.c_str();
	}

private:
	std::string mWhat;
};

static Movie mMovie = 0;
static Track mTrack = 0;
static Media mMedia = 0;
static Str255 fileNameAbs255;
static Handle mData = 0;
static DataHandler mDataHandler = 0;
static short mMovieRefNum = 0;
static GWorldPtr world = 0;
static Handle data = 0;

const static int sx = 320;
const static int sy = 240;

static void Encode();
static void MovieBegin();
static void MovieEnd();
static void MovieAppend(ICMEncodedFrameRef frame);

static void CheckStatus(OSStatus status);
static void CheckError(OSErr err);
static void CheckCvReturn(CVReturn cvReturn);

static void SourceFrameTrackingCallback(void *sourceTrackingRefCon, ICMSourceTrackingFlags sourceTrackingFlags, void *sourceFrameRefCon, void *reserved);
static OSStatus OutputCallback(void *encodedFrameOutputRefCon, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef frame, void *reserved);

int main(int argc, char* argv[])
{
	try
	{
		MovieBegin();

		Encode();

		MovieEnd();

		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s", e.what());

		return -1;
	}
}

static void Encode()
{
	OSStatus status = 0;
	OSErr err = 0;
	CVReturn cvret = 0;

	uint32_t* colors = new uint32_t[sx * sy];
	
	Rect rect;

	rect.left = 0;
	rect.right = sx;
	rect.top = 0;
	rect.bottom = sy;

	CodecType codecType = kH264CodecType;
	//CodecQ codecQuality = codecHighQuality;
	CodecQ codecQuality = codecNormalQuality;

	printf("creating GWorld\n");

	err = NewGWorldFromPtr(
		&world, 
		k24RGBPixelFormat,
		&rect,
		0, 0, 0,
		(Ptr)colors,
		sizeof(uint32_t) * sx);
	CheckError(err);

	SetGWorld(world, 0);

	printf("determining max compression size\n");

	long maxSize = 0;

	err = GetMaxCompressionSize(
		world->portPixMap,
		&rect,
		0, // let ICM decide color depth
		codecQuality,
		codecType,
		(CompressorComponent)anyCodec,
		&maxSize);
	CheckError(err);

	printf("allocating image description\n");

	ImageDescriptionHandle imageDesc;

	imageDesc = (ImageDescriptionHandle)NewHandle(sizeof(ImageDescriptionHandle));

	if (!imageDesc)
		throw ExceptionVA("unable to allocate image desc handle");

	printf("creating compression session options\n");

	ICMCompressionSessionOptionsRef options = 0;

	status = ICMCompressionSessionOptionsCreate(
		kCFAllocatorDefault,
		&options);
	CheckStatus(status);

	status = ICMCompressionSessionOptionsSetAllowTemporalCompression(options, true); // P+B frames
	CheckStatus(status);
	status = ICMCompressionSessionOptionsSetAllowFrameReordering(options, true); // B frames
	CheckStatus(status);
	status = ICMCompressionSessionOptionsSetMaxKeyFrameInterval(options, 30);
	CheckStatus(status);
	status = ICMCompressionSessionOptionsSetAllowFrameTimeChanges(options, true); // allow ICM to drop/coalesc frames
	CheckStatus(status);
	status = ICMCompressionSessionOptionsSetDurationsNeeded(options, true);
	CheckStatus(status);

	status = ICMCompressionSessionOptionsSetProperty(options, 
		kQTPropertyClass_ICMCompressionSessionOptions,
		kICMCompressionSessionOptionsPropertyID_Quality,
		sizeof(codecQuality),
		&codecQuality);
	CheckStatus(status);

	ICMMultiPassStorageRef storage = 0;
	status = ICMCompressionSessionOptionsSetProperty(options, kQTPropertyClass_ICMCompressionSessionOptions, kICMCompressionSessionOptionsPropertyID_MultiPassStorage, sizeof(ICMMultiPassStorageRef), &storage);
	CheckStatus(status);

	Boolean async = FALSE;
	status = ICMCompressionSessionOptionsSetProperty(options, kQTPropertyClass_ICMCompressionSessionOptions, kICMCompressionSessionOptionsPropertyID_AllowAsyncCompletion, sizeof(Boolean), &async);
	CheckStatus(status);

	printf("creating compression session\n");

	ICMEncodedFrameOutputRecord record = { 0 };

	record.encodedFrameOutputCallback = OutputCallback;
    record.encodedFrameOutputRefCon = 0;
    record.frameDataAllocator = 0;

	ICMCompressionSessionRef session;

	status = ICMCompressionSessionCreate(
		kCFAllocatorDefault,
		sx,
		sy,
		codecType,
		1000,
		options,
		NULL,
		&record,
		&session);
	CheckStatus(status);

	ICMCompressionSessionOptionsRelease(options);

	TimeValue64 time = 0;
	TimeValue64 duration = 1000/50;

	printf("creating pixel buffer\n");

	CVPixelBufferRef pixelBuffer = 0;

	cvret = CVPixelBufferCreate(
		kCFAllocatorDefault,
		sx,
		sy,
		k24RGBPixelFormat,
		0,
		&pixelBuffer);
	CheckCvReturn(cvret);

	cvret = CVPixelBufferLockBaseAddress(pixelBuffer, 0);
	CheckCvReturn(cvret);
	uint8_t* pixelData = (uint8_t*)CVPixelBufferGetBaseAddress(pixelBuffer);

	for (int i = 0; i < 1000; ++i)
	{
		printf("encoding frame\n");

		int sampleCount = sx * sy * 3;

		for (int y = 0; y < sy; ++y)
		{
			uint8_t* dstLine = pixelData + y * sx * 3;

			for (int x = 0; x < sx; ++x)
			{
				const int v = (x + i) * ((y - i) >> 3) + (((x + y) * i) >> 3);

				*dstLine++ = v;
				*dstLine++ = v >> 1;
				*dstLine++ = v >> 2;
			}
		}

		status = ICMCompressionSessionEncodeFrame(
			session,
			pixelBuffer,
			time,
			duration,
			kICMValidTime_DisplayDurationIsValid | kICMValidTime_DisplayTimeStampIsValid,
			0,
			0,
			0);
		CheckStatus(status);

		time += duration;
	}

	CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

	//ICMCompressionFrameOptionsRelease(frameOptions);

	printf("completing frames\n");

	status = ICMCompressionSessionCompleteFrames(
		session,
		true,
		0,
		0);
	CheckStatus(status);
}

static void MovieBegin()
{
	OSErr err = 0;

	const char* fileName = "test.mov";

	// initialize QuickTime

	printf("initializing QTML");

	err = InitializeQTML(0);
	CheckError(err);

	// enter da movies~

	printf("enter movies\n");

	err = EnterMovies();
	CheckError(err);

	// calculate absolute filename

	printf("creating filename\n");

	char fileNameAbs[MAX_PATH];
	if(!_fullpath(fileNameAbs, fileName, MAX_PATH))
		throw ExceptionVA("unable to determine absolute path");
	c2pstrcpy(fileNameAbs255, fileNameAbs);

	// open movie file
	
	FSSpec mFileSpec;

	// note: FSMakeFSSpec doesn't return noErr.. yet movie renders OK..
	FSMakeFSSpec(0, 0, fileNameAbs255, &mFileSpec);

	// setup QuickTime encoder

	printf("creating movie storage\n");

	Handle data = 0;
	OSType dataType;
	
	err = QTNewDataReferenceFromFSSpec(&mFileSpec, 0, &data, &dataType);
	CheckError(err);

	//smCurrentScript
	err = CreateMovieStorage(data, dataType, 'TVOD', smAllScripts, createMovieFileDeleteCurFile | newMovieActive, &mDataHandler, &mMovie);
	CheckError(err);

	DisposeHandle(data);

	printf("creating new movie track\n");

	mTrack = NewMovieTrack(
		mMovie,
		FixRatio(sx, 1),
		FixRatio(sy, 1), 0);
	CheckError(GetMoviesError());

	if (mTrack == 0)
		throw ExceptionVA("unable to create movie track");

	printf("creating new track media\n");

	mMedia = NewTrackMedia(
		mTrack,
		VideoMediaType,
		1000, 0, 0);
	CheckError(GetMoviesError());

	if (mMedia == 0)
		throw ExceptionVA("unable to create track media");

	err = BeginMediaEdits(mMedia);
	CheckError(err);
}

static void MovieAppend(ICMEncodedFrameRef frame)
{
	OSErr err =0;

	TimeValue64 duration = ICMEncodedFrameGetDecodeDuration(frame);

	printf("duration: %d\n", (int)duration);

	if (duration > 0)
	{
		printf("adding media sample from encoded frame\n");

		if (ICMEncodedFrameGetDataSize(frame) > 0)
		{
			err = AddMediaSampleFromEncodedFrame(mMedia, frame, 0); 
			CheckError(err);
		}
	}
}

static void MovieEnd()
{
	OSErr err = 0;

	printf("ending media edits\n");

	short resId = movieInDataForkResID;

	if (mMedia)
	{
		err = EndMediaEdits(mMedia);
		CheckError(err);

		err = ExtendMediaDecodeDurationToDisplayEndTime(mMedia, 0);
		CheckError(err);
	}

	printf("adding media to track\n");

	if (mTrack)
	{
		err = InsertMediaIntoTrack(
			mTrack,
			0,
			0,
			GetMediaDuration(mMedia),
			fixed1);
		CheckError(err);
	}

	printf("adding movie storage\n");

	if (mDataHandler)
	{
		AddMovieToStorage(mMovie, mDataHandler);
		CloseMovieStorage(mDataHandler);
		mDataHandler = 0;
	}

	printf("closing movie file\n");

	if (mMovieRefNum)
	{
		err = CloseMovieFile(mMovieRefNum);
		mMovieRefNum = 0;
		CheckError(err);
	}

	printf("disposing movie\n");

	if (mMovie)
	{
		DisposeMovie(mMovie);
		mMovie = 0;
	}

	printf("disposing GWorld\n");

	if (world)
	{
		DisposeGWorld(world);
		world = 0;
	}

	printf("exiting the movies\n");

	ExitMovies();
	CheckError(err);

	printf("shutting down QTML\n");

	TerminateQTML();
}

static void CheckStatus(OSStatus status)
{
	if (status != 0)
		throw ExceptionVA("error: status: %d", (int)status);
}

static void CheckError(OSErr err)
{
	if (err != noErr)
		throw ExceptionVA("error: code: %d", (int)err);
}

static void CheckCvReturn(CVReturn cvReturn)
{
	if (cvReturn != kCVReturnSuccess)
		throw ExceptionVA("error: cv error: %d", (int)cvReturn);
}

static void SourceFrameTrackingCallback(void *sourceTrackingRefCon, ICMSourceTrackingFlags sourceTrackingFlags, void *sourceFrameRefCon, void *reserved)
{
	printf("SourceFrameTrackingCallback\n");
}

static OSStatus OutputCallback(void *encodedFrameOutputRefCon, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef frame, void *reserved)
{
	CheckStatus(error);

	OSErr err = 0;

	printf("OutputCallback\n");

	MovieAppend(frame);

	return noErr;
}

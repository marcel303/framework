#pragma once

#include <movies.h>
#include <qtml.h>
#include <string>
#include "MacImage.h"

enum QtVideoCodec
{
	QtVideoCodec_Undefined,
	QtVideoCodec_H264,
	QtVideoCodec_Jpeg,
	QtVideoCodec_Jpeg2000
};

enum QtVideoQuality
{
	QtVideoQuality_Undefined,
	QtVideoQuality_Low,
	QtVideoQuality_Medium,
	QtVideoQuality_High
};

class QuickTimeEncoder
{
public:
	QuickTimeEncoder();

	void Initialize(const char* fileName, QtVideoCodec videoCodec, QtVideoQuality videoQuality, MacImage* image);
	void Shutdown();
	void CommitVideoFrame(int durationMS);

	bool IsInitialized_get() const;

private:
	bool mIsInitialized;
	std::string mFileName;
	QtVideoCodec mVideoCodec;
	QtVideoQuality mVideoQuality;
	MacImage* mImage;

	//

	CodecType mQtCodecType;
	CodecQ mQtCodecQuality;

	// --------------------
	// Movie stuff
	// --------------------
	void MovieBegin();
	void MovieEnd();
	void MovieAppend(ICMEncodedFrameRef frame);

	Rect mRect;
	Movie mMovie;
	Track mTrack;
	Media mMedia;
	Str255 mFileNameAbs255;
	DataHandler mDataHandler;

	// --------------------
	// Encoder stuff
	// --------------------
	void EncoderBegin();
	void EncoderEnd();
	static OSStatus OutputCallback(void* obj, ICMCompressionSessionRef session, OSStatus error, ICMEncodedFrameRef frame, void* reserved);

	TimeValue64 mTime;
	ICMCompressionSessionRef mSession;
	CVPixelBufferRef mPixelBuffer;
	uint8_t* mPixelDataPtr;

	// --------------------
	// helpers: error checking
	// --------------------
	void CheckErrors(const char* desc);

	OSErr mError;
	OSStatus mStatus;
	CVReturn mCvReturn;
};

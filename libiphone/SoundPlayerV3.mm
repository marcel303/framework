/* Copyright (C) 2008 Apple Inc. All Rights Reserved. */

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CFURL.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <map>
#include <vector>
#include <pthread.h>
#include <mach/mach.h>
#include <string>

#include <AVFoundation/AVFoundation.h>
#include "Benchmark.h"
#include "Debugging.h"
#include "ResIO.h"
#include "SoundPlayerV3.h"

SoundPlayerV3::SoundPlayerV3()
{
}

void SoundPlayerV3::Initialize(bool playBackgroundMusic)
{
	if (playBackgroundMusic == false)
	{
		UInt32 sessionCategory = kAudioSessionCategory_SoloAmbientSound; // Allows iPod music playback to be mixed.

		/*OSStatus status = */AudioSessionSetProperty(
			kAudioSessionProperty_AudioCategory,
			sizeof(sessionCategory),
			&sessionCategory);
		
		//Assert(status == 0);
	}
	
	m_Res = 0;
	m_Volume = 1.0f;
	m_Loop = false;
	m_IsEnabled = true;
	m_PlayBackgroundMusic = playBackgroundMusic;
}

void SoundPlayerV3::Shutdown()
{
	Stop();
}

void SoundPlayerV3::Play(Res* res1, Res* res2, Res* res3, Res* res4, bool loop)
{
	Assert(res2 == 0);
	Assert(res3 == 0);
	Assert(res4 == 0);
	
	UsingBegin(Benchmark bm("Starting sound playback"))
	{
		if (res1 == m_Res)
			return;
		
		Stop();
		
		m_Res = res1;
		m_Loop = loop;
		
		if (m_IsEnabled)
		{
			Start();
		}
	}
	UsingEnd()
}

void SoundPlayerV3::Start()
{
	if (m_PlayBackgroundMusic)
		return;
	
	UsingBegin(Benchmark b("starting background music"))
	{
		std::string bundleFileName = ResIO::GetBundleFileName(m_Res->m_FileName);
		SoundEngine_LoadBackgroundMusicTrack(bundleFileName.c_str(), false, false);
		SoundEngine_SetBackgroundMusicVolume(m_Volume);
		SoundEngine_StartBackgroundMusic();
	}
	UsingEnd()
}

void SoundPlayerV3::Stop()
{
	if (m_PlayBackgroundMusic)
		return;
	
	SoundEngine_StopBackgroundMusic(false);
	SoundEngine_UnloadBackgroundMusicTrack();
}

bool SoundPlayerV3::HasFinished_get()
{
	return SoundEngine_HasEnded();
}

void SoundPlayerV3::IsEnabled_set(bool enabled)
{
	if (enabled == m_IsEnabled)
		return;
	
	m_IsEnabled = enabled;
	
	if (m_IsEnabled)
	{
		if (m_Res)
		{
			Start();
		}
	}
	else
	{
		Stop();
	}
}

void SoundPlayerV3::Volume_set(float volume)
{
	if (m_PlayBackgroundMusic)
		return;
	
	m_Volume = volume;
	
	SoundEngine_SetBackgroundMusicVolume(m_Volume);
}

void SoundPlayerV3::ChannelVolume_set(int channelIdx, float volume)
{
	// nop
}

void SoundPlayerV3::Update()
{
	// nop
}

//

#define	AssertNoError(inMessage, inHandler)						\
			if(result != noErr)									\
			{													\
				printf("%s: %d\n", inMessage, (int)result);		\
				goto inHandler;									\
			}
			
#define AssertNoOALError(inMessage, inHandler)					\
			if((result = alGetError()) != AL_NO_ERROR)			\
			{													\
				printf("%s: %x\n", inMessage, (int)result);		\
				goto inHandler;									\
			}

#define kNumberBuffers 3

class BackgroundTrackMgr;

static BackgroundTrackMgr* sBackgroundTrackMgr = 0;

//==================================================================================================
//	Helper functions
//==================================================================================================
OSStatus OpenFile(const char *inFilePath, AudioFileID &outAFID)
{
	
	CFURLRef theURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)inFilePath, strlen(inFilePath), false);
	if (theURL == NULL)
		return kSoundEngineErrFileNotFound;

#if TARGET_OS_IPHONE
	OSStatus result = AudioFileOpenURL(theURL, kAudioFileReadPermission, 0, &outAFID);
#else
	OSStatus result = AudioFileOpenURL(theURL, fsRdPerm, 0, &outAFID);
#endif
	CFRelease(theURL);
		AssertNoError("Error opening file", end);
	
	end:
		return result;
}

OSStatus LoadFileDataInfo(const char *inFilePath, AudioFileID &outAFID, AudioStreamBasicDescription &outFormat, UInt64 &outDataSize)
{
	UInt32 thePropSize = sizeof(outFormat);				
	OSStatus result = OpenFile(inFilePath, outAFID);
		AssertNoError("Error opening file", end);

	result = AudioFileGetProperty(outAFID, kAudioFilePropertyDataFormat, &thePropSize, &outFormat);
		AssertNoError("Error getting file format", end);
	
	thePropSize = sizeof(UInt64);
	result = AudioFileGetProperty(outAFID, kAudioFilePropertyAudioDataByteCount, &thePropSize, &outDataSize);
		AssertNoError("Error getting file data size", end);

end:
	return result;
}

void CalculateBytesForTime (AudioStreamBasicDescription & inDesc, UInt32 inMaxPacketSize, Float64 inSeconds, UInt32 *outBufferSize, UInt32 *outNumPackets)
{
	static const UInt32 maxBufferSize = 0x10000; // limit size to 64K
	static const UInt32 minBufferSize = 0x4000; // limit size to 16K

	if (inDesc.mFramesPerPacket) {
		Float64 numPacketsForTime = inDesc.mSampleRate / inDesc.mFramesPerPacket * inSeconds;
		*outBufferSize = (long unsigned int)numPacketsForTime * inMaxPacketSize;
	} else {
		// if frames per packet is zero, then the codec has no predictable packet == time
		// so we can't tailor this (we don't know how many Packets represent a time period
		// we'll just return a default buffer size
		*outBufferSize = maxBufferSize > inMaxPacketSize ? maxBufferSize : inMaxPacketSize;
	}
	
		// we're going to limit our size to our default
	if (*outBufferSize > maxBufferSize && *outBufferSize > inMaxPacketSize)
		*outBufferSize = maxBufferSize;
	else {
		// also make sure we're not too small - we don't want to go the disk for too small chunks
		if (*outBufferSize < minBufferSize)
			*outBufferSize = minBufferSize;
	}
	*outNumPackets = *outBufferSize / inMaxPacketSize;
}

static Boolean MatchFormatFlags(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	UInt32 xFlags = x.mFormatFlags;
	UInt32 yFlags = y.mFormatFlags;
	
	// match wildcards
	if (x.mFormatID == 0 || y.mFormatID == 0 || xFlags == 0 || yFlags == 0) 
		return true;
	
	if (x.mFormatID == kAudioFormatLinearPCM)
	{		 		
		// knock off the all clear flag
		xFlags = xFlags & ~kAudioFormatFlagsAreAllClear;
		yFlags = yFlags & ~kAudioFormatFlagsAreAllClear;
	
		// if both kAudioFormatFlagIsPacked bits are set, then we don't care about the kAudioFormatFlagIsAlignedHigh bit.
		if (xFlags & yFlags & kAudioFormatFlagIsPacked) {
			xFlags = xFlags & ~kAudioFormatFlagIsAlignedHigh;
			yFlags = yFlags & ~kAudioFormatFlagIsAlignedHigh;
		}
		
		// if both kAudioFormatFlagIsFloat bits are set, then we don't care about the kAudioFormatFlagIsSignedInteger bit.
		if (xFlags & yFlags & kAudioFormatFlagIsFloat) {
			xFlags = xFlags & ~kAudioFormatFlagIsSignedInteger;
			yFlags = yFlags & ~kAudioFormatFlagIsSignedInteger;
		}
		
		//	if the bit depth is 8 bits or less and the format is packed, we don't care about endianness
		if((x.mBitsPerChannel <= 8) && ((xFlags & kAudioFormatFlagIsPacked) == kAudioFormatFlagIsPacked))
		{
			xFlags = xFlags & ~kAudioFormatFlagIsBigEndian;
		}
		if((y.mBitsPerChannel <= 8) && ((yFlags & kAudioFormatFlagIsPacked) == kAudioFormatFlagIsPacked))
		{
			yFlags = yFlags & ~kAudioFormatFlagIsBigEndian;
		}
		
		//	if the number of channels is 0 or 1, we don't care about non-interleavedness
		if (x.mChannelsPerFrame <= 1 && y.mChannelsPerFrame <= 1) {
			xFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
			yFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
		}
	}
	return xFlags == yFlags;
}

Boolean FormatIsEqual(AudioStreamBasicDescription x, AudioStreamBasicDescription y)
{
	//	the semantics for equality are:
	//		1) Values must match exactly
	//		2) wildcard's are ignored in the comparison
	
#define MATCH(name) ((x.name) == 0 || (y.name) == 0 || (x.name) == (y.name))
	
	return 
		((x.mSampleRate==0.) || (y.mSampleRate==0.) || (x.mSampleRate==y.mSampleRate)) 
		&& MATCH(mFormatID)
		&& MatchFormatFlags(x, y)  
		&& MATCH(mBytesPerPacket) 
		&& MATCH(mFramesPerPacket) 
		&& MATCH(mBytesPerFrame) 
		&& MATCH(mChannelsPerFrame) 		
		&& MATCH(mBitsPerChannel) ;
}

#pragma mark ***** BackgroundTrackMgr *****
//==================================================================================================
//	BackgroundTrackMgr class
//==================================================================================================
class BackgroundTrackMgr
	{	
#define CurFileInfo THIS->mBGFileInfo[THIS->mCurrentFileIndex]
	public:
		typedef struct BG_FileInfo {
			std::string						mFilePath;
			AudioFileID						mAFID;
			AudioStreamBasicDescription		mFileFormat;
			UInt64							mFileDataSize;
			//UInt64							mFileNumPackets; // this is only used if loading file to memory
			Boolean							mLoadAtOnce;
			Boolean							mFileDataInQueue;
		} BackgroundMusicFileInfo;
		
		BackgroundTrackMgr();
		~BackgroundTrackMgr();
		
		void Teardown();
		void ClearFileInfo();
		
		AudioStreamPacketDescription *GetPacketDescsPtr();
		
		UInt32 GetNumPacketsToRead(BackgroundTrackMgr::BG_FileInfo *inFileInfo);
		
		static OSStatus AttachNewCookie(AudioQueueRef inQueue, BackgroundTrackMgr::BG_FileInfo *inFileInfo);
		static void QueueStoppedProc( void * inUserData, AudioQueueRef inAQ, AudioQueuePropertyID inID );
		static Boolean DisposeBuffer(AudioQueueRef inAQ, std::vector<AudioQueueBufferRef> inDisposeBufferList, AudioQueueBufferRef inBufferToDispose);
		
		enum {
			kQueueState_DoNothing		= 0,
			kQueueState_ResizeBuffer	= 1,
			kQueueState_NeedNewCookie	= 2,
			kQueueState_NeedNewBuffers	= 3,
			kQueueState_NeedNewQueue	= 4,
		};
		
		static SInt8 GetQueueStateForNextBuffer(BackgroundTrackMgr::BG_FileInfo *inFileInfo, BackgroundTrackMgr::BG_FileInfo *inNextFileInfo);
		static void QueueCallback( void * inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inCompleteAQBuffer);
		
		OSStatus SetupQueue(BG_FileInfo *inFileInfo);
		OSStatus SetupBuffers(BG_FileInfo *inFileInfo);
		OSStatus LoadTrack(const char* inFilePath, Boolean inAddToQueue, Boolean inLoadAtOnce);
		
		OSStatus UpdateGain();
		OSStatus SetVolume(Float32 inVolume);
		Float32 GetVolume() const;
		Boolean HasEnded() const { return mHasEnded; }
		
		OSStatus Start();
		OSStatus Stop(Boolean inStopAtEnd);
		
	private:
		AudioQueueRef						mQueue;
		AudioQueueBufferRef					mBuffers[kNumberBuffers];
		UInt32								mBufferByteSize;
		SInt64								mCurrentPacket;
		UInt32								mNumPacketsToRead;
		Float32								mVolume;
		AudioStreamPacketDescription *		mPacketDescs;
		std::vector<BG_FileInfo*>			mBGFileInfo;
		UInt32								mCurrentFileIndex;
		Boolean								mMakeNewQueueWhenStopped;
		Boolean								mStopAtEnd;
		std::vector<AudioQueueBufferRef>	mBuffersToDispose;
		Boolean								mHasEnded;
	};

BackgroundTrackMgr::BackgroundTrackMgr() 
		:	mQueue(0),
		mBufferByteSize(0),
		mCurrentPacket(0),
		mNumPacketsToRead(0),
		mVolume(1.0f),
		mPacketDescs(NULL),
		mCurrentFileIndex(0),
		mMakeNewQueueWhenStopped(false),
		mStopAtEnd(false),
		mHasEnded(true)
{ }
		
BackgroundTrackMgr::~BackgroundTrackMgr() {
	Teardown();
}
		
void BackgroundTrackMgr::Teardown() {
	if (mQueue) {
		AudioQueueDispose(mQueue, true);
	}
	for (UInt32 i=0; i < mBGFileInfo.size(); i++) {
		if (mBGFileInfo[i]->mAFID) {
			AudioFileClose(mBGFileInfo[i]->mAFID);
		}
	}
	
	if (mPacketDescs) {
		delete mPacketDescs;
	}
	
	ClearFileInfo();
}
		
void BackgroundTrackMgr::ClearFileInfo() {
	std::vector< BG_FileInfo* >::iterator itr = mBGFileInfo.begin();
	std::vector< BG_FileInfo* >::iterator endItr = mBGFileInfo.end();
	for( ; itr != endItr; ++itr ) {
		delete *itr;
		*itr = NULL;
	}
	mBGFileInfo.clear();
}
		
AudioStreamPacketDescription *BackgroundTrackMgr::GetPacketDescsPtr() {
	return mPacketDescs;
}
		
UInt32 BackgroundTrackMgr::GetNumPacketsToRead(BackgroundTrackMgr::BG_FileInfo *inFileInfo) { 
	(void)inFileInfo;
	return mNumPacketsToRead; 
}
		
OSStatus BackgroundTrackMgr::AttachNewCookie(AudioQueueRef inQueue, BackgroundTrackMgr::BG_FileInfo *inFileInfo)	{
	OSStatus result = noErr;
	UInt32 size = sizeof(UInt32);
	result = AudioFileGetPropertyInfo (inFileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, NULL);
	if (!result && size) {
		char* cookie = new char [size];		
		result = AudioFileGetProperty (inFileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, cookie);
		AssertNoError("Error getting cookie data", end);
		result = AudioQueueSetProperty(inQueue, kAudioQueueProperty_MagicCookie, cookie, size);
		delete [] cookie;
		AssertNoError("Error setting cookie data for queue", end);
	}
	return noErr;
			
	end:
	return noErr;
}
		
void BackgroundTrackMgr::QueueStoppedProc( void * inUserData, AudioQueueRef inAQ, AudioQueuePropertyID inID ) {
	(void)inID;
	UInt32 isRunning;
	UInt32 propSize = sizeof(isRunning);
			
	BackgroundTrackMgr *THIS = (BackgroundTrackMgr*)inUserData;
	OSStatus result = AudioQueueGetProperty(inAQ, kAudioQueueProperty_IsRunning, &isRunning, &propSize);
			
	if ((!isRunning) && (THIS->mMakeNewQueueWhenStopped)) {
		result = AudioQueueDispose(inAQ, true);
		AssertNoError("Error disposing queue", end);
		result = THIS->SetupQueue(CurFileInfo);
		AssertNoError("Error setting up new queue", end);
		result = THIS->SetupBuffers(CurFileInfo);
		AssertNoError("Error setting up new queue buffers", end);
		result = THIS->Start();
		AssertNoError("Error starting queue", end);
	}
	end:
	return;
}
		
Boolean BackgroundTrackMgr::DisposeBuffer(AudioQueueRef inAQ, std::vector<AudioQueueBufferRef> inDisposeBufferList, AudioQueueBufferRef inBufferToDispose) {
	for (unsigned int i=0; i < inDisposeBufferList.size(); i++)	{
		if (inBufferToDispose == inDisposeBufferList[i]) {
			OSStatus result = AudioQueueFreeBuffer(inAQ, inBufferToDispose);
			if (result == noErr) {
				inDisposeBufferList.pop_back();
			}
			return true;
		}
	}
	return false;
}
		
SInt8 BackgroundTrackMgr::GetQueueStateForNextBuffer(BackgroundTrackMgr::BG_FileInfo *inFileInfo, BackgroundTrackMgr::BG_FileInfo *inNextFileInfo) {
	inFileInfo->mFileDataInQueue = false;
			
	// unless the data formats are the same, we need a new queue
	if (!FormatIsEqual(inFileInfo->mFileFormat, inNextFileInfo->mFileFormat)) {
		return kQueueState_NeedNewQueue;
	}
			
	// if going from a load-at-once file to streaming or vice versa, we need new buffers
	if (inFileInfo->mLoadAtOnce != inNextFileInfo->mLoadAtOnce) {
		return kQueueState_NeedNewBuffers;
	}
			
	// if the next file is smaller than the current, we just need to resize
	if (inNextFileInfo->mLoadAtOnce) {
		return (inFileInfo->mFileDataSize >= inNextFileInfo->mFileDataSize) ? kQueueState_ResizeBuffer : kQueueState_NeedNewBuffers;
	}
			
	return kQueueState_NeedNewCookie;
}
		
void BackgroundTrackMgr::QueueCallback( void * inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inCompleteAQBuffer ) {
	// dispose of the buffer if no longer in use
	OSStatus result = noErr;
	BackgroundTrackMgr *THIS = (BackgroundTrackMgr*)inUserData;
	if (DisposeBuffer(inAQ, THIS->mBuffersToDispose, inCompleteAQBuffer)) {
		return;
	}	

	UInt32 nPackets = 0;
	// loop the current buffer if the following:
	// 1. file was loaded into the buffer previously
	// 2. only one file in the queue
	// 3. we have not been told to stop at playlist completion
	if ((CurFileInfo->mFileDataInQueue) && (THIS->mBGFileInfo.size() == 1) && (!THIS->mStopAtEnd)) {
		nPackets = THIS->GetNumPacketsToRead(CurFileInfo);
	} else {
		UInt32 numBytes;
		while (nPackets == 0) {
			// if loadAtOnce, get all packets in the file, otherwise ~.5 seconds of data
			nPackets = THIS->GetNumPacketsToRead(CurFileInfo);					
			result = AudioFileReadPackets(CurFileInfo->mAFID, false, &numBytes, THIS->mPacketDescs, THIS->mCurrentPacket, &nPackets, 
										  inCompleteAQBuffer->mAudioData);
			AssertNoError("Error reading file data", end);
			
			inCompleteAQBuffer->mAudioDataByteSize = numBytes;	
			
			if (nPackets == 0) { // no packets were read, this file has ended.
				if (CurFileInfo->mLoadAtOnce) {
					CurFileInfo->mFileDataInQueue = true;
				}
						
				THIS->mCurrentPacket = 0;
				UInt32 theNextFileIndex = (THIS->mCurrentFileIndex < THIS->mBGFileInfo.size()-1) ? THIS->mCurrentFileIndex+1 : 0;
						
				// we have gone through the playlist. if mStopAtEnd, stop the queue here
				if (theNextFileIndex == 0 && THIS->mStopAtEnd) {
					result = AudioQueueStop(inAQ, false);
					AssertNoError("Error stopping queue", end);
					THIS->mHasEnded = true;
					return;
				}
						
				SInt8 theQueueState = GetQueueStateForNextBuffer(CurFileInfo, THIS->mBGFileInfo[theNextFileIndex]);
				if (theNextFileIndex != THIS->mCurrentFileIndex) {
					// if were are not looping the same file. Close the old one and open the new
					result = AudioFileClose(CurFileInfo->mAFID);
					AssertNoError("Error closing file", end);
					THIS->mCurrentFileIndex = theNextFileIndex;
							
					result = LoadFileDataInfo(CurFileInfo->mFilePath.c_str(), CurFileInfo->mAFID, CurFileInfo->mFileFormat, CurFileInfo->mFileDataSize);
					AssertNoError("Error opening file", end);
				}
						
				switch (theQueueState) {							
						// if we need to resize the buffer, set the buffer's audio data size to the new file's size
						// we will also need to get the new file cookie
					case kQueueState_ResizeBuffer:
						inCompleteAQBuffer->mAudioDataByteSize = (UInt32)CurFileInfo->mFileDataSize;							
						// if the data format is the same but we just need a new cookie, attach a new cookie
					case kQueueState_NeedNewCookie:
						result = AttachNewCookie(inAQ, CurFileInfo);
						AssertNoError("Error attaching new file cookie data to queue", end);
						break;
						
						// we can keep the same queue, but not the same buffer(s)
					case kQueueState_NeedNewBuffers:
						THIS->mBuffersToDispose.push_back(inCompleteAQBuffer);
						THIS->SetupBuffers(CurFileInfo);
						break;
								
						// if the data formats are not the same, we need to dispose the current queue and create a new one
					case kQueueState_NeedNewQueue:
						THIS->mMakeNewQueueWhenStopped = true;
						result = AudioQueueStop(inAQ, false);
						AssertNoError("Error stopping queue", end);
						return;
								
					default:
						break;
				}
			}
		}
	}
			
	result = AudioQueueEnqueueBuffer(inAQ, inCompleteAQBuffer, (THIS->mPacketDescs ? nPackets : 0), THIS->mPacketDescs);
    if(result != noErr) {
        result = AudioQueueFreeBuffer(inAQ, inCompleteAQBuffer);
		AssertNoError("Error freeing buffers that didn't enqueue", end);
    }
	AssertNoError("Error enqueuing new buffer", end);
	if (CurFileInfo->mLoadAtOnce) {
		CurFileInfo->mFileDataInQueue = true;
	}
	
	THIS->mCurrentPacket += nPackets;
			
	end:
	return;
}
		
OSStatus BackgroundTrackMgr::SetupQueue(BG_FileInfo *inFileInfo) {
	UInt32 size = 0;
	OSStatus err;
	OSStatus result = AudioQueueNewOutput(&inFileInfo->mFileFormat, QueueCallback, this, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &mQueue);
	AssertNoError("Error creating queue", end);
			
	// (2) If the file has a cookie, we should get it and set it on the AQ
	size = sizeof(UInt32);
	result = AudioFileGetPropertyInfo (inFileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, NULL);
			
	if (!result && size) {
		char* cookie = new char [size];		
		result = AudioFileGetProperty (inFileInfo->mAFID, kAudioFilePropertyMagicCookieData, &size, cookie);
		AssertNoError("Error getting magic cookie", end);
		result = AudioQueueSetProperty(mQueue, kAudioQueueProperty_MagicCookie, cookie, size);
		delete [] cookie;
		AssertNoError("Error setting magic cookie", end);
	}
			
	// channel layout
	err = AudioFileGetPropertyInfo(inFileInfo->mAFID, kAudioFilePropertyChannelLayout, &size, NULL);
	if (err == noErr && size > 0) {
		AudioChannelLayout *acl = (AudioChannelLayout *)malloc(size);
		result = AudioFileGetProperty(inFileInfo->mAFID, kAudioFilePropertyChannelLayout, &size, acl);
		AssertNoError("Error getting channel layout from file", end);
		result = AudioQueueSetProperty(mQueue, kAudioQueueProperty_ChannelLayout, acl, size);
		free(acl);
		AssertNoError("Error setting channel layout on queue", end);
	}
			
	// add a notification proc for when the queue stops
	result = AudioQueueAddPropertyListener(mQueue, kAudioQueueProperty_IsRunning, QueueStoppedProc, this);
	AssertNoError("Error adding isRunning property listener to queue", end);
			
	// we need to reset this variable so that if the queue is stopped mid buffer we don't dispose it 
	mMakeNewQueueWhenStopped = false;
		
	// volume
	result = SetVolume(mVolume);
			
	end:
	return result;
}
		
OSStatus BackgroundTrackMgr::SetupBuffers(BG_FileInfo *inFileInfo) {
	int numBuffersToQueue = kNumberBuffers;
	UInt32 maxPacketSize;
	UInt32 size = sizeof(maxPacketSize);
	bool isFormatVBR;
	// we need to calculate how many packets we read at a time, and how big a buffer we need
	// we base this on the size of the packets in the file and an approximate duration for each buffer
			
	// first check to see what the max size of a packet is - if it is bigger
	// than our allocation default size, that needs to become larger
	OSStatus result = AudioFileGetProperty(inFileInfo->mAFID, kAudioFilePropertyPacketSizeUpperBound, &size, &maxPacketSize);
	AssertNoError("Error getting packet upper bound size", end);
	isFormatVBR = (inFileInfo->mFileFormat.mBytesPerPacket == 0 || inFileInfo->mFileFormat.mFramesPerPacket == 0);
			
	CalculateBytesForTime(inFileInfo->mFileFormat, maxPacketSize, 0.5/*seconds*/, &mBufferByteSize, &mNumPacketsToRead);
		
	// if the file is smaller than the capacity of all the buffer queues, always load it at once
	if ((mBufferByteSize * numBuffersToQueue) > inFileInfo->mFileDataSize) {
		inFileInfo->mLoadAtOnce = true;
	}
			
	if (inFileInfo->mLoadAtOnce) {
		UInt64 theFileNumPackets;
		size = sizeof(UInt64);
		result = AudioFileGetProperty(inFileInfo->mAFID, kAudioFilePropertyAudioDataPacketCount, &size, &theFileNumPackets);
		AssertNoError("Error getting packet count for file", end);
				
		mNumPacketsToRead = (UInt32)theFileNumPackets;
		mBufferByteSize = (UInt32)inFileInfo->mFileDataSize;
		numBuffersToQueue = 1;
	} else {
		mNumPacketsToRead = mBufferByteSize / maxPacketSize;
	}
			
	if (isFormatVBR) {
		mPacketDescs = new AudioStreamPacketDescription [mNumPacketsToRead];
	} else {
		mPacketDescs = NULL; // we don't provide packet descriptions for constant bit rate formats (like linear PCM)	
	}
			
	// allocate the queue's buffers
	for (int i = 0; i < numBuffersToQueue; ++i) {
		result = AudioQueueAllocateBuffer(mQueue, mBufferByteSize, &mBuffers[i]);
		AssertNoError("Error allocating buffer for queue", end);
		QueueCallback (this, mQueue, mBuffers[i]);
		if (inFileInfo->mLoadAtOnce) {
			inFileInfo->mFileDataInQueue = true;
		}
	}
			
	end:
	return result;
}
		
OSStatus BackgroundTrackMgr::LoadTrack(const char* inFilePath, Boolean inAddToQueue, Boolean inLoadAtOnce) {
	BG_FileInfo *fileInfo = new BG_FileInfo;
	fileInfo->mFilePath = inFilePath;
	OSStatus result = LoadFileDataInfo(fileInfo->mFilePath.c_str(), fileInfo->mAFID, fileInfo->mFileFormat, fileInfo->mFileDataSize);
	AssertNoError("Error getting file data info", fail);
	fileInfo->mLoadAtOnce = inLoadAtOnce;
	fileInfo->mFileDataInQueue = false;
	// if not adding to the queue, clear the current file vector
	if (!inAddToQueue) {
		ClearFileInfo();
	}
	
	mHasEnded = false;
			
	mBGFileInfo.push_back(fileInfo);
			
	// setup the queue if this is the first (or only) file
	if (mBGFileInfo.size() == 1) {
		result = SetupQueue(fileInfo);
		AssertNoError("Error setting up queue", fail);
		result = SetupBuffers(fileInfo);
		AssertNoError("Error setting up queue buffers", fail);					
	} else { // if this is just part of the playlist, close the file for now
		result = AudioFileClose(fileInfo->mAFID);
		AssertNoError("Error closing file", fail);
	}	
	return result;
			
	fail:
	if (fileInfo) {
		delete fileInfo;
	}
	return result;
}
		
OSStatus BackgroundTrackMgr::UpdateGain() {
	return SetVolume(mVolume);
}
		
OSStatus BackgroundTrackMgr::SetVolume(Float32 inVolume) {
	mVolume = inVolume;
	return AudioQueueSetParameter(mQueue, kAudioQueueParam_Volume, mVolume);
}
		
Float32 BackgroundTrackMgr::GetVolume() const {
	return mVolume;
}
		
OSStatus BackgroundTrackMgr::Start() {	
	OSStatus result = AudioQueuePrime(mQueue, 1, NULL);	
	if (result)	{
		printf("Error priming queue: %d\n", (int)result);
		return result;
	}
	return AudioQueueStart(mQueue, NULL);
}
		
OSStatus BackgroundTrackMgr::Stop(Boolean inStopAtEnd) {
	if (inStopAtEnd) {
		mStopAtEnd = true;
		return noErr;
	} else {
		return AudioQueueStop(mQueue, true);
	}
}

#pragma mark ***** API *****
//==================================================================================================
//	Sound Engine
//==================================================================================================

OSStatus  SoundEngine_LoadBackgroundMusicTrack(const char* inPath, Boolean inAddToQueue, Boolean inLoadAtOnce)
{
	if (sBackgroundTrackMgr == NULL)
		sBackgroundTrackMgr = new BackgroundTrackMgr();
	
	return sBackgroundTrackMgr->LoadTrack(inPath, inAddToQueue, inLoadAtOnce);
}

OSStatus  SoundEngine_UnloadBackgroundMusicTrack()
{
	if (sBackgroundTrackMgr)
	{
		delete sBackgroundTrackMgr;
		sBackgroundTrackMgr = NULL;
	}
		
	return 0;
}

OSStatus  SoundEngine_StartBackgroundMusic()
{
	return (sBackgroundTrackMgr) ? sBackgroundTrackMgr->Start() : kSoundEngineErrUnitialized;
}

OSStatus  SoundEngine_StopBackgroundMusic(Boolean stopAtEnd)
{
	return (sBackgroundTrackMgr) ?  sBackgroundTrackMgr->Stop(stopAtEnd) : kSoundEngineErrUnitialized;
}

OSStatus  SoundEngine_SetBackgroundMusicVolume(Float32 inValue)
{
	return (sBackgroundTrackMgr) ? sBackgroundTrackMgr->SetVolume(inValue) : kSoundEngineErrUnitialized;
}

Float32  SoundEngine_GetBackgroundMusicVolume()
{
	return (sBackgroundTrackMgr) ? sBackgroundTrackMgr->GetVolume() : 0.0f;
}

Boolean SoundEngine_HasEnded()
{
	return (sBackgroundTrackMgr) ? sBackgroundTrackMgr->HasEnded() : true;
}

/*
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef ALvoid	AL_APIENTRY	(*alBufferDataStaticProcPtr) (const ALint bid, ALenum format, ALvoid* data, ALsizei size, ALsizei freq);
ALvoid  alBufferDataStaticProc(const ALint bid, ALenum format, ALvoid* data, ALsizei size, ALsizei freq)
{
	static	alBufferDataStaticProcPtr	proc = NULL;
    
    if (proc == NULL) {
        proc = (alBufferDataStaticProcPtr) alcGetProcAddress(NULL, (const ALCchar*) "alBufferDataStatic");
    }
    
    if (proc)
        proc(bid, format, data, size, freq);

    return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef ALvoid	AL_APIENTRY	(*alcMacOSXMixerOutputRateProcPtr) (const ALdouble value);
ALvoid  alcMacOSXMixerOutputRateProc(const ALdouble value)
{
	static	alcMacOSXMixerOutputRateProcPtr	proc = NULL;
    
    if (proc == NULL) {
        proc = (alcMacOSXMixerOutputRateProcPtr) alcGetProcAddress(NULL, (const ALCchar*) "alcMacOSXMixerOutputRate");
    }
    
    if (proc)
        proc(value);

    return;
}*/

#define DO_JPEGSEQUENCE 1
#define STREAM_ID "b"
#define NUM_JPEG_LOOPS 4

#if DO_JPEGSEQUENCE

#include "Calc.h"
#include "framework.h"
#include "StringEx.h"
#include <list>
#include <string.h>
#include <turbojpeg/turbojpeg.h>

extern const int GFX_SX;
extern const int GFX_SY;

static bool loadFileContents(const char * filename, void * bytes, int & numBytes)
{
	bool result = true;
	
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
	{
		result = false;
	}
	else
	{
		// load source from file
		
		fseek(file, 0, SEEK_END);
		const int fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		if (numBytes < fileSize)
		{
			result = false;
		}
		else
		{
			numBytes = fileSize;
			
			if (fread(bytes, numBytes, 1, file) != (size_t)1)
			{
				result = false;
			}
		}
		
		fclose(file);
	}
	
	if (!result)
	{
		numBytes = 0;
	}
	
	return result;
}

struct JpegLoadData
{
	unsigned char * buffer;
	int bufferSize;
	bool flipY;
	
	int sx;
	int sy;
	
	JpegLoadData()
		: buffer(nullptr)
		, bufferSize(0)
		, flipY(false)
		, sx(0)
		, sy(0)
	{
	}
	
	~JpegLoadData()
	{
		free();
	}
	
	void disown()
	{
		buffer = 0;
		bufferSize = 0;
		sx = 0;
		sy = 0;
	}
	
	void free()
	{
		delete [] buffer;
		buffer = nullptr;
		bufferSize = 0;
	}
};

static bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data)
{
	bool result = true;
	
	tjhandle h = tjInitDecompress();
	
	if (h == nullptr)
	{
		logError("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		int sx = 0;
		int sy = 0;
		
		int jpegSubsamp = 0;
		int jpegColorspace = 0;
		
		if (tjDecompressHeader3(h, (unsigned char*)buffer, (unsigned long)bufferSize, &sx, &sy, &jpegSubsamp, &jpegColorspace) != 0)
		{
			logError("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			data.sx = sx;
			data.sy = sy;
			
			const TJPF pixelFormat = TJPF_RGBX;
			
			const int pitch = sx * tjPixelSize[pixelFormat];
			
			const int flags = TJFLAG_BOTTOMUP * (data.flipY ? 1 : 0);
			
			const int requiredBufferSize = pitch * sy;
			
			if (data.buffer == nullptr || data.bufferSize != requiredBufferSize)
			{
				delete [] data.buffer;
				data.buffer = nullptr;
				data.bufferSize = 0;
				
				//
				
				data.buffer = new unsigned char[requiredBufferSize];
				data.bufferSize = requiredBufferSize;
			}
			
			if (tjDecompress2(h, (unsigned char*)buffer, (unsigned long)bufferSize, (unsigned char*)data.buffer, sx, pitch, data.sy, pixelFormat, flags) != 0)
			{
				logError("turbojpeg: %s", tjGetErrorStr());
				
				result = false;
			}
			else
			{
				//logDebug("decoded jpeg!");
			}
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

static bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, void * fileBuffer, int fileBufferSize)
{
	bool result = true;
	
	if (loadFileContents(filename, fileBuffer, fileBufferSize) == false)
	{
		logDebug("turbojpeg: %s", "failed to load file contents");
		
		result = false;
	}
	else
	{
		result = loadImage_turbojpeg(fileBuffer, fileBufferSize, data);
	}
	
	return result;
}

static bool saveImage_turbojpeg(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, void *& dstBuffer, int & dstBufferSize)
{
	bool result = true;
	
	tjhandle h = tjInitCompress();
	
	if (h == nullptr)
	{
		logError("turbojpeg: %s", tjGetErrorStr());
		
		result = false;
	}
	else
	{
		const TJPF pixelFormat = TJPF_RGBX;
		const TJSAMP subsamp = TJSAMP_422;
		const int quality = 85;
		
		const int xPitch = srcSx * tjPixelSize[pixelFormat];
		
		unsigned long dstBufferSize2 = dstBufferSize;
		
		if (tjCompress2(h, (const unsigned char *)srcBuffer, srcSx, xPitch, srcSy, TJPF_RGBX, (unsigned char**)&dstBuffer, &dstBufferSize2, subsamp, quality, 0) < 0)
		{
			logError("turbojpeg: %s", tjGetErrorStr());
			
			result = false;
		}
		else
		{
			dstBufferSize = dstBufferSize2;
		}
		
		tjDestroy(h);
		h = nullptr;
	}
	
	return result;
}

static bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, void * _saveBuffer, int _saveBufferSize)
{
	bool result = true;
	
	void * saveBuffer = _saveBuffer;
	int saveBufferSize = _saveBufferSize;
	
	if (saveImage_turbojpeg(srcBuffer, srcBufferSize, srcSx, srcSy, saveBuffer, saveBufferSize) == false)
	{
		result = false;
	}
	else
	{
		FILE * file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			result = false;
		}
		else
		{
			if (fwrite(saveBuffer, saveBufferSize, 1, file) != (size_t)1)
			{
				result = false;
			}
			
			fclose(file);
		}
	}
	
	Assert(saveBuffer == _saveBuffer);
	Assert(saveBufferSize <= _saveBufferSize);
	
	return result;
}

static bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy)
{
	bool result = true;
	
	void * saveBuffer = nullptr;
	int saveBufferSize = 0;
	
	if (saveImage_turbojpeg(srcBuffer, srcBufferSize, srcSx, srcSy, saveBuffer, saveBufferSize) == false)
	{
		result = false;
	}
	else
	{
		FILE * file = fopen(filename, "wb");
		
		if (file == nullptr)
		{
			result = false;
		}
		else
		{
			if (fwrite(saveBuffer, saveBufferSize, 1, file) != (size_t)1)
			{
				result = false;
			}
			
			fclose(file);
		}
	}
	
	tjFree((unsigned char*)saveBuffer);
	saveBuffer = nullptr;
	saveBufferSize = 0;
	
	return result;
}

struct JpegStreamer
{
	static const int kFileBufferSize = 1024 * 1024;
	
	struct FileContents
	{
		unsigned char * buffer;
		int bufferSize;
		
		int index;
		
		FileContents()
			: buffer(nullptr)
			, bufferSize(0)
			, index(0)
		{
			 buffer = new unsigned char[kFileBufferSize];
			 bufferSize = kFileBufferSize;
		}
		
		~FileContents()
		{
			delete [] buffer;
			buffer = nullptr;
			bufferSize = 0;
			
			index = 0;
		}
	};
	
	struct ImageContents
	{
		unsigned char * buffer;
		int bufferSize;
		
		int sx;
		int sy;
		
		ImageContents()
			: buffer(nullptr)
			, bufferSize(0)
			, sx(0)
			, sy(0)
		{
		}
		
		~ImageContents()
		{
			delete [] buffer;
			buffer = nullptr;
			bufferSize = 0;
			
			sx = 0;
			sy = 0;
		}
	};
	
	static const int kNumBuffers = 4;
	
	std::string fileBaseName;
	int fileNextIndex;
	
	std::list<FileContents*> fileProduceList;
	std::list<FileContents*> fileConsumeList;
	
	SDL_Thread * fileThread;
	SDL_mutex * fileMutex;
	SDL_sem * fileProduceSema;
	SDL_sem * fileConsumeSema;
	
	//
	
	std::list<ImageContents*> imageProduceList;
	std::list<ImageContents*> imageConsumeList;
	
	SDL_Thread * imageThread;
	SDL_mutex * imageMutex;
	SDL_sem * imageProduceSema;
	SDL_sem * imageConsumeSema;
	
	bool stop;
	
	JpegStreamer(const char * _baseName)
		: fileBaseName(_baseName)
		, fileNextIndex(0)
		, fileProduceList()
		, fileConsumeList()
		, fileThread(nullptr)
		, fileMutex(nullptr)
		, fileProduceSema(nullptr)
		, fileConsumeSema(nullptr)
		, imageProduceList()
		, imageConsumeList()
		, imageThread(nullptr)
		, imageMutex(nullptr)
		, imageProduceSema(nullptr)
		, imageConsumeSema(nullptr)
		, stop(false)
	{
		for (int i = 0; i < kNumBuffers; ++i)
		{
			FileContents * fileContents = new FileContents();
			
			fileProduceList.push_back(fileContents);
		}
		
		fileMutex = SDL_CreateMutex();
		
		fileProduceSema = SDL_CreateSemaphore(kNumBuffers);
		fileConsumeSema = SDL_CreateSemaphore(0);
		
		fileThread = SDL_CreateThread(fileProcess, "JpegStreamer File Process", this);
		
		//
		
		for (int i = 0; i < kNumBuffers; ++i)
		{
			ImageContents * imageContents = new ImageContents();
			
			imageProduceList.push_back(imageContents);
		}
		
		imageMutex = SDL_CreateMutex();
		
		imageProduceSema = SDL_CreateSemaphore(kNumBuffers);
		imageConsumeSema = SDL_CreateSemaphore(0);
		
		imageThread = SDL_CreateThread(imageProcess, "JpegStreamer Image Process", this);
	}
	
	~JpegStreamer()
	{
		SDL_LockMutex(imageMutex);
		{
			stop = true;
		}
		SDL_UnlockMutex(imageMutex);
		
		SDL_SemPost(imageProduceSema);
		
		SDL_WaitThread(imageThread, nullptr);
		imageThread = nullptr;
		
		SDL_DestroySemaphore(imageConsumeSema);
		imageConsumeSema = nullptr;
		
		SDL_DestroySemaphore(imageProduceSema);
		imageProduceSema = nullptr;
		
		SDL_DestroyMutex(imageMutex);
		imageMutex = nullptr;
		
		while (imageProduceList.size() > 0)
		{
			delete imageProduceList.front();
			
			imageProduceList.pop_front();
		}
		
		while (imageConsumeList.size() > 0)
		{
			delete imageConsumeList.front();
			
			imageConsumeList.pop_front();
		}
		
		//
		
		SDL_LockMutex(fileMutex);
		{
			stop = true;
		}
		SDL_UnlockMutex(fileMutex);
		
		SDL_SemPost(fileProduceSema);
		
		SDL_WaitThread(fileThread, nullptr);
		fileThread = nullptr;
		
		SDL_DestroySemaphore(fileConsumeSema);
		fileConsumeSema = nullptr;
		
		SDL_DestroySemaphore(fileProduceSema);
		fileProduceSema = nullptr;
		
		SDL_DestroyMutex(fileMutex);
		fileMutex = nullptr;
		
		while (fileProduceList.size() > 0)
		{
			delete fileProduceList.front();
			
			fileProduceList.pop_front();
		}
		
		while (fileConsumeList.size() > 0)
		{
			delete fileConsumeList.front();
			
			fileConsumeList.pop_front();
		}
	}
	
	static int fileProcess(void * obj)
	{
		JpegStreamer * self = (JpegStreamer*)obj;
		
		self->fileProcess();
		
		return 0;
	}
	
	void fileProcess()
	{
		while (stop == false)
		{
			SDL_SemWait(fileProduceSema);
			
			//
			
			FileContents * fileContents = nullptr;
			
			SDL_LockMutex(fileMutex);
			{
				if (stop == false)
				{
					fileContents = fileProduceList.front();
					
					fileProduceList.pop_front();
				}
			}
			SDL_UnlockMutex(fileMutex);
			
			//
			
			if (fileContents == nullptr)
				break;
			
			//
			
			char filename[256];
			sprintf_s(filename, sizeof(filename), fileBaseName.c_str(), fileNextIndex + 1);
			
			fileContents->bufferSize = kFileBufferSize;
			fileContents->index = fileNextIndex;
	
			const bool hasFile = loadFileContents(filename, fileContents->buffer, fileContents->bufferSize);
			
			fileNextIndex++;
			
			//
			
			if (hasFile)
			{
				SDL_LockMutex(fileMutex);
				{
					fileConsumeList.push_back(fileContents);
				}
				SDL_UnlockMutex(fileMutex);
				
				SDL_SemPost(fileConsumeSema);
			}
			else
			{
				delete fileContents;
				fileContents = nullptr;
			}
		}
	}
	
	static int imageProcess(void * obj)
	{
		JpegStreamer * self = (JpegStreamer*)obj;
		
		self->imageProcess();
		
		return 0;
	}
	
	void imageProcess()
	{
		while (stop == false)
		{
			SDL_SemWait(imageProduceSema);
			
			ImageContents * imageContents = nullptr;
			
			SDL_LockMutex(imageMutex);
			{
				if (stop == false)
				{
					imageContents = imageProduceList.front();
					
					imageProduceList.pop_front();
				}
			}
			SDL_UnlockMutex(imageMutex);
			
			//
			
			if (imageContents == nullptr)
				break;
			
			//
			
			SDL_SemWait(fileConsumeSema);
			
			FileContents * fileContents = nullptr;
			
			SDL_LockMutex(fileMutex);
			{
				if (stop == false)
				{
					fileContents = fileConsumeList.front();
					
					fileConsumeList.pop_front();
				}
			}
			SDL_UnlockMutex(fileMutex);
			
			//
			
			// fixme : stop here is not guaranteed to work (I think)
			
			if (fileContents == nullptr)
				break;
			
			//
			
			JpegLoadData data;
			
			const bool hasImage = loadImage_turbojpeg(
				fileContents->buffer,
				fileContents->bufferSize,
				data);
			
			imageContents->buffer = data.buffer;
			imageContents->bufferSize = data.bufferSize;
			imageContents->sx = data.sx;
			imageContents->sy = data.sy;
			
			if (hasImage)
			{
				SDL_LockMutex(fileMutex);
				{
					fileProduceList.push_back(fileContents);
				}
				SDL_UnlockMutex(fileMutex);
				
				SDL_SemPost(fileProduceSema);
				
				//
				
				SDL_LockMutex(imageMutex);
				{
					imageConsumeList.push_back(imageContents);
				}
				SDL_UnlockMutex(imageMutex);
				
				SDL_SemPost(imageConsumeSema);
			}
			else
			{
				delete imageContents;
				imageContents = nullptr;
				
				delete fileContents;
				fileContents = nullptr;
			}
		}
	}
	
	void consume(ImageContents *& contents)
	{
		SDL_SemWait(imageConsumeSema);
		
		SDL_LockMutex(imageMutex);
		{
			contents = imageConsumeList.front();
			
			imageConsumeList.pop_front();
		}
		SDL_UnlockMutex(imageMutex);
	}
	
	void release(ImageContents *& contents)
	{
		SDL_LockMutex(imageMutex);
		{
			imageProduceList.push_back(contents);
			
			contents = nullptr;
		}
		SDL_UnlockMutex(imageMutex);
		
		SDL_SemPost(imageProduceSema);
	}
};

struct JpegLoop
{
	std::string baseFilename;
	double fps;
	
	unsigned char * fileBuffer;
	int fileBufferSize;
	
	unsigned char ** dstBuffer;
	int * dstBufferSize;
	
	GLuint texture;
	
	JpegLoop(const char * _baseFilename, const double _fps, unsigned char * _fileBuffer, int _fileBufferSize, unsigned char ** _dstBuffer, int * _dstBufferSize)
		: baseFilename(_baseFilename)
		, fps(_fps)
		, fileBuffer(_fileBuffer)
		, fileBufferSize(_fileBufferSize)
		, dstBuffer(_dstBuffer)
		, dstBufferSize(_dstBufferSize)
		, texture(0)
	{
		glGenTextures(1, &texture);
		
		if (texture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			checkErrorGL();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();
		}
	}
	
	~JpegLoop()
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}
	
	void seek(const double time)
	{
		const int frameIndex = std::floor(time * fps);
		
		char filename[256];
		sprintf_s(filename, sizeof(filename), baseFilename.c_str(), frameIndex + 1);
		
		JpegLoadData data;
		data.buffer = *dstBuffer;
		data.bufferSize = *dstBufferSize;
		//data.flipY = true;
		
		const bool hasImage = loadImage_turbojpeg(filename, data, fileBuffer, fileBufferSize);
		
		*dstBuffer = data.buffer;
		*dstBufferSize = data.bufferSize;
		
		if (hasImage)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			checkErrorGL();
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, data.sx, data.sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.buffer);
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();
		}
		
		data.disown();
	}
};

static void speedup(const char * srcBasename, const char * dstBasename, const int numFrames)
{
	const int numDstFrames = numFrames / 2;
	
	int loadBufferSize = 1024 * 1024;
	char * loadBuffer = new char[loadBufferSize];
	
	int saveBufferSize = 1024 * 1024;
	char * saveBuffer = new char[saveBufferSize];
	
	JpegLoadData loadData[2];
	
	for (int i = 0; i < numDstFrames; ++i)
	{
		char srcFilename1[256];
		char srcFilename2[256];
		char dstFilename[256];
		
		sprintf_s(srcFilename1, sizeof(srcFilename1), srcBasename, i*2+0 + 1);
		sprintf_s(srcFilename2, sizeof(srcFilename2), srcBasename, i*2+1 + 1);
		sprintf_s(dstFilename, sizeof(dstFilename), dstBasename, i + 1);
		
		if (loadImage_turbojpeg(srcFilename1, loadData[0], loadBuffer, loadBufferSize) &&
			loadImage_turbojpeg(srcFilename2, loadData[1], loadBuffer, loadBufferSize) &&
			(loadData[0].sx) == (loadData[1].sx) &&
			(loadData[0].sy) == (loadData[1].sy))
		{
			for (int i = 0; i < loadData[0].bufferSize; ++i)
			{
				loadData[0].buffer[i] = (int(loadData[0].buffer[i]) + int(loadData[1].buffer[i])) / 2;
			}
			
			saveImage_turbojpeg(dstFilename, loadData[0].buffer, loadData[0].bufferSize, loadData[0].sx, loadData[0].sy, saveBuffer, saveBufferSize);
		}
		
		if ((i % 100) == 0)
		{
			printf("processing frame %d/%d\n", i, numDstFrames);
		}
	}
	
	for (int i = 0; i < 2; ++i)
		loadData[i].free();
	
	delete [] saveBuffer;
	saveBuffer = nullptr;
	saveBufferSize = 0;
	
	delete [] loadBuffer;
	loadBuffer = nullptr;
	loadBufferSize = 0;
}

void testJpegStreamer()
{
#if 0
	int numFrames = 43200;
	int streamIndex = 0;
	
#if 0
	while (numFrames >= 2 && streamIndex < 12)
	{
		numFrames /= 2;
		++streamIndex;
	}
#endif
	
	while (numFrames >= 2)
	{
		char srcPath[256];
		char dstPath[256];
		
		sprintf_s(srcPath, sizeof(srcPath), "/Users/thecat/videosplitter/slides-" STREAM_ID "-%02d", streamIndex + 0);
		sprintf_s(dstPath, sizeof(dstPath), "/Users/thecat/videosplitter/slides-" STREAM_ID "-%02d", streamIndex + 1);
		
		mkdir(dstPath, S_IRWXU | S_IRGRP | S_IROTH);
		
		char srcBasename[256];
		char dstBasename[256];
		
		sprintf_s(srcBasename, sizeof(srcBasename), "%s/%%06d.jpg", srcPath);
		sprintf_s(dstBasename, sizeof(dstBasename), "%s/%%06d.jpg", dstPath);
		
		speedup(srcBasename, dstBasename, numFrames);
		
		numFrames /= 2;
		++streamIndex;
	}
#endif

#if 0
	{
		JpegStreamer * jpegStreamer = new JpegStreamer("/Users/thecat/videosplitter/slides-" STREAM_ID "-00/%06d.jpg");
		
		logDebug("testJpegStreamer: start");
		
		for (int i = 0; i < 1000; ++i)
		{
			JpegStreamer::ImageContents * contents = nullptr;
			
			jpegStreamer->consume(contents);
			
			jpegStreamer->release(contents);
		}
		
		logDebug("testJpegStreamer: stop");
		
		delete jpegStreamer;
		jpegStreamer = nullptr;
	}
#endif

#if 0
	unsigned char * dstBuffer = nullptr;
	int dstBufferSize = 0;
	
	int fileBufferSize = 1024 * 1024;
	char * fileBuffer = new char[fileBufferSize];
	
	logDebug("testTurbojpeg: start");
	
	for (int i = 0; i < 1000; ++i)
	{
		char filename[256];
		sprintf_s(filename, sizeof(filename), "/Users/thecat/videosplitter/slides-" STREAM_ID "-00/%06d.jpg", i + 1);
		
		int dstSx;
		int dstSy;
		
		loadImage_turbojpeg(filename, dstBuffer, dstBufferSize, dstSx, dstSy, fileBuffer, fileBufferSize);
	}
	
	logDebug("testTurbojpeg: stop");
	
	delete [] fileBuffer;
	fileBuffer = nullptr;
	fileBufferSize = 0;
	
	delete [] dstBuffer;
	dstBuffer = nullptr;
	dstBufferSize = 0;
#endif

	changeDirectory("/Users/thecat/Google Drive/The Grooop - Welcome");
	
	//

#if 1
	framework.init(0, nullptr, GFX_SX, GFX_SY);
	{
		const int fps = 24;
		const double duration = 1800.0;
		
		unsigned char * dstBuffer = nullptr;
		int dstBufferSize = 0;
		
		int fileBufferSize = 1024 * 1024;
		unsigned char * fileBuffer = new unsigned char[fileBufferSize];
		
		const int kNumJpegLoops = NUM_JPEG_LOOPS;
		
		JpegLoop * jpegLoop[kNumJpegLoops] = { };
		
		for (int i = 0; i < kNumJpegLoops; ++i)
		{
			jpegLoop[i] = new JpegLoop("/Users/thecat/videosplitter/slides-" STREAM_ID "-00/%06d.jpg", fps, fileBuffer, fileBufferSize, &dstBuffer, &dstBufferSize);
		}
		
		Surface * surface = new Surface(GFX_SX, GFX_SY, true);
		surface->clear();
		
		double time = 0.0;
		bool isPaused = false;
		bool doMotionBlur = true;
		bool showThumbnails = true;
		
		enum UiState
		{
			UiState_Idle,
			UiState_Seek
		};
		
		UiState uiState = UiState_Idle;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_SPACE))
				isPaused = !isPaused;
			
			if (keyboard.wentDown(SDLK_m))
				doMotionBlur = !doMotionBlur;
			
			if (keyboard.wentDown(SDLK_t))
				showThumbnails = !showThumbnails;
			
			if (uiState == UiState_Idle)
			{
				if (mouse.wentDown(BUTTON_LEFT))
					uiState = UiState_Seek;
			}
			
			if (uiState == UiState_Seek)
			{
				if (mouse.wentUp(BUTTON_LEFT))
					uiState = UiState_Idle;
			}
			
			if (uiState == UiState_Seek)
			{
				time = duration * mouse.x / double(GFX_SX);
			}
			
			const float speedInterp = (mouse.x / float(GFX_SX) - .5f) * 2.f;
			const float speedFactor = Calc::Lerp(0.f, 1000.f, std::powf(std::abs(speedInterp), 2.5f)) * Calc::Sign(speedInterp);
			
			int streamIndex = 0;
			
			if (doMotionBlur)
			{
				for (float i = 1.f; i * 2.f < std::abs(speedFactor) && streamIndex < 14; i *= 2.f)
					streamIndex++;
			}
			
			//logDebug("stream index: %d", streamIndex);
			
			//
			
			float fpsMultiplier = 1.f;
			
			for (int i = 0; i < streamIndex; ++i)
				fpsMultiplier /= 2.f;
			
			for (int i = 0; i < kNumJpegLoops; ++i)
			{
				float seek = 0.f;
				
				if (i == 0)
					seek = time;
				else
					seek = (duration / i) - time;
				
				jpegLoop[i]->fps = fps * fpsMultiplier;
				
				char baseFilename[256];
				sprintf_s(baseFilename, sizeof(baseFilename), "/Users/thecat/videosplitter/slides-" STREAM_ID "-%02d/%%06d.jpg", streamIndex);
				
				jpegLoop[i]->baseFilename = baseFilename;
				
				jpegLoop[i]->seek(seek);
			}
			
			if (uiState == UiState_Idle)
			{
				if (isPaused == false)
				{
					time += framework.timeStep * speedFactor;
				}
			}
			
			if (time > duration)
				time = 0.0;
			if (time < 0.0)
				time = duration;
			
			framework.beginDraw(0, 0, 0, 0);
			{
				pushSurface(surface);
				{
				#if 0
					pushBlend(BLEND_ALPHA);
				#else
					pushBlend(BLEND_OPAQUE);
				#endif
					setColor(255, 255, 255, 15);
					gxSetTexture(jpegLoop[0]->texture);
					drawRect(0, 0, GFX_SX, GFX_SY);
					gxSetTexture(0);
					popBlend();
				}
				popSurface();
				
			#if 1
				const float kernelSize = 127.f * std::pow(std::abs(mouse.y / float(GFX_SY) - .5f) * 2.f, 4.f);
				pushBlend(BLEND_OPAQUE);
				setShader_GaussianBlurH(surface->getTexture(), kernelSize + 1, kernelSize);
				surface->postprocess();
				setShader_GaussianBlurV(surface->getTexture(), kernelSize + 1, kernelSize);
				surface->postprocess();
				clearShader();
				popBlend();
			#endif
			
				pushSurface(surface);
				{
					if (showThumbnails)
					{
						for (int i = 0; i < kNumJpegLoops; ++i)
						{
							const int nx = 3;
							const int sx = 128 * 2;
							const int sy = 72 * 2;
							const int px = 10;
							const int py = 10;
							
							const int ix = i % nx;
							const int iy = i / nx;
							
							const int x = (sx + px) * ix + sx/2;
							const int y = (sy + py) * iy + sy/2;
							
							gxPushMatrix();
							{
								gxTranslatef(x, y, 0);
								pushBlend(BLEND_ALPHA);
								setColor(255, 255, 255, 227);
								gxSetTexture(jpegLoop[i]->texture);
								drawRect(0, 0, sx, sy);
								gxSetTexture(0);
								popBlend();
							}
							gxPopMatrix();
						}
					}
				
				#if 1
					const int padding = 10;
					const int sy = 30;
					setColor(255, 255, 0, 191);
					const int sxf = GFX_SX - padding * 2;
					const double sxt = sxf * time / duration;
					drawRect(padding, GFX_SY - padding - sy, padding + sxt, GFX_SY - padding);
					setColor(colorBlack);
					drawRectLine(padding, GFX_SY - padding - sy, padding + sxf, GFX_SY - padding);
					
					setColor(colorWhite);
					setFont("calibri.ttf");
					int timeInSeconds = int(std::floor(time));
					const int hh = timeInSeconds / 3600;
					timeInSeconds -= hh * 3600;
					const int mm = timeInSeconds / 60;
					timeInSeconds -= mm * 60;
					const int ss = timeInSeconds;
					drawText(GFX_SX/2, GFX_SY - padding - sy/2, 22, 0.f, 0.f, "%02d:%02d:%02d - %gx", hh, mm, ss, speedFactor);
				#endif
				}
				popSurface();
				
				pushBlend(BLEND_OPAQUE);
				setColor(colorWhite);
				gxSetTexture(surface->getTexture());
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				popBlend();
			}
			framework.endDraw();
		}
		
		for (int i = 0; i < kNumJpegLoops; ++i)
		{
			delete jpegLoop[i];
			jpegLoop[i] = nullptr;
		}
		
		delete surface;
		surface = nullptr;
		
		delete [] fileBuffer;
		fileBuffer = nullptr;
		fileBufferSize = 0;
		
		delete [] dstBuffer;
		dstBuffer = nullptr;
		dstBufferSize = 0;
	}
	framework.shutdown();
#endif
}

#else

void testJpegStreamer()
{
}

#endif

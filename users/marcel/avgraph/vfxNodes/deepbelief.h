#pragma once

#include <string>
#include <vector>

struct SDL_cond;
struct SDL_mutex;
struct SDL_Thread;

struct DeepbeliefPrediction
{
	float certainty;
	std::string label;
};

struct DeepbeliefResult
{
	std::vector<DeepbeliefPrediction> predictions;
	
	double bufferCreationTime;
	double classificationTime;
	double sortTime;
	
	DeepbeliefResult()
		: predictions()
		, bufferCreationTime(0.0)
		, classificationTime(0.0)
		, sortTime(0.0)
	{
	}
};

struct Deepbelief
{
	struct Work
	{
		uint8_t * bytes;
		int sx;
		int sy;
		int numChannels;
		int pitch;
		float certaintyTreshold;

		~Work()
		{
			delete[] bytes;
			bytes = nullptr;
		}
	};

	struct State
	{
		bool stop;
		
		bool isDone;

		Work * work;
		
		bool hasResult;
		DeepbeliefResult result;

		State()
			: stop(false)
			, isDone(false)
			, work(nullptr)
			, hasResult(false)
			, result()
		{
		}
	};
	
	bool isInitialized;
	
	void * network;
	
	SDL_Thread * thread;
	SDL_mutex * mutex;
	SDL_cond * workEvent;
	SDL_cond * doneEvent;
	
	State state;

	Deepbelief();
	~Deepbelief();

	void init(const char * networkFilename);
	bool doInit(const char * networkFilename);
	void shut();

	void process(const uint8_t * bytes, const int sx, const int sy, const int numChannels, const int pitch, const float certaintyTreshold);
	void wait(); // optional : wait for all of the processing to complete

	bool getResult(DeepbeliefResult & result);

	static int threadMainProc(void * arg);

	void threadMain();
};

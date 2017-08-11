#pragma once

#include <list>
#include <map>
#include <string>

struct AudioGraph;
struct AudioGraphFileRTC;
struct AudioRealTimeConnection;
struct Graph;
struct GraphEdit;
struct GraphEdit_TypeDefinitionLibrary;

struct SDL_mutex;

struct AudioGraphInstance
{
	AudioGraph * audioGraph;
	AudioRealTimeConnection * realTimeConnection;

	AudioGraphInstance();
	~AudioGraphInstance();
};

struct AudioGraphFile
{
	std::string filename;
	
	std::list<AudioGraphInstance> instanceList;

	const AudioGraphInstance * activeInstance;

	AudioGraphFileRTC * realTimeConnection;
	
	GraphEdit * graphEdit;

	AudioGraphFile();
	~AudioGraphFile();
};

struct AudioControlValue
{
	enum Type
	{
		kType_Vector1d,
		kType_Vector2d,
		kType_Random1d,
		kType_Random2d,
	};
	
	Type type;
	std::string name;
	int refCount = 0;
	
	float min;
	float max;
	float smoothness;
	float defaultX;
	float defaultY;
	
	float desiredX;
	float desiredY;
	float currentX;
	float currentY;
};

struct AudioGraphManager
{
	struct Memf
	{
		float value1 = 0.f;
		float value2 = 0.f;
		float value3 = 0.f;
		float value4 = 0.f;
	};
	
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::vector<AudioControlValue> controlValues;
	
	std::map<std::string, AudioGraphFile*> files;
	
	AudioGraphFile * selectedFile;
	
	std::map<std::string, Memf> memf;
	
	SDL_mutex * audioMutex;
	
	AudioGraphManager();
	~AudioGraphManager();
	
	void init(SDL_mutex * mutex);
	void shut();
	
	void selectFile(const char * filename);
	void selectInstance(const AudioGraphInstance * instance);
	
	AudioGraphInstance * createInstance(const char * filename);
	void free(AudioGraphInstance *& instance);
	
	void registerControlValue(AudioControlValue::Type type, const char * name, const float min, const float max, const float smoothness, const float defaultX, const float defaultY);
	void unregisterControlValue(const char * name);
	bool findControlValue(const char * name, AudioControlValue & result) const;
	void exportControlValues();
	
	void setMemf(const char * name, const float value1, const float value2 = 0.f, const float value3 = 0.f, const float value4 = 0.f);
	Memf getMemf(const char * name);

	void tick(const float dt);
	void updateAudioValues();
	
	bool tickEditor(const float dt, const bool isInputCaptured);
	void drawEditor();
};

extern AudioGraphManager * g_audioGraphMgr;

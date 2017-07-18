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

struct AudioGraphManager
{
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary;
	
	std::map<std::string, AudioGraphFile*> files;
	
	AudioGraphFile * selectedFile;
	
	SDL_mutex * audioMutex;
	
	AudioGraphManager();
	~AudioGraphManager();
	
	void selectFile(const char * filename);
	void selectInstance(const AudioGraphInstance * instance);
	
	AudioGraphInstance * createInstance(const char * filename);
	void free(AudioGraphInstance *& instance);

	void tick(const float dt);
	void draw() const;
	void updateAudioValues();
	
	void tickEditor(const float dt);
	void drawEditor();
};

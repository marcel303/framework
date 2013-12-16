#pragma once

#include <string>
#include "libgg_forward.h"

class LevelPackDescription
{
public:
	LevelPackDescription();
	LevelPackDescription(std::string path);
	
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	std::string path;
	std::string copyright;
	std::string name;
	std::string description;
};

class LevelDescription
{
public:
	LevelDescription();
	LevelDescription(std::string path);
	
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	std::string path;
	std::string author;
	std::string copyright;
	std::string name;
};

class LevelPack
{
public:
	LevelPackDescription mDescription;
};

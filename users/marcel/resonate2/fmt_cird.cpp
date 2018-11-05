#include "fmt_cird.h"
#include <assert.h>
#include <stdio.h> // FILE
#include <string.h> // memset

CIRD::Header::Header()
{
	// Initialize the entire header to zero.
	
	memset(this, 0, sizeof(*this));
}

//

CIRD::~CIRD()
{
	free();
}

bool CIRD::initialize(const int numFrequencies, const int cubeMapSx, const int cubeMapSy, const float * frequencies)
{
	free();
	
	header = Header();
	
	if (numFrequencies > kMaxFrequencies)
		return false;
	
	header.version = 1000;
	header.numFrequencies = numFrequencies;
	header.cubeMapSx = cubeMapSx;
	header.cubeMapSy = cubeMapSy;
	
	memcpy(header.frequencies, frequencies, sizeof(float) * numFrequencies);
	
	allocate(numFrequencies, cubeMapSx, cubeMapSy);
	
	return true;
}

bool CIRD::loadFromFile(const char * filename)
{
	bool result = true;

	FILE * file = fopen(filename, "rb");

	if (file == nullptr)
		result = false;

	if (result)
	{
		if (fread(&header, sizeof(Header), 1, file) != 1)
			result = false;

		// check if the version number is correct
		if (header.version != 1000)
			result = false;

		// check if the number of frequencies is sane
		if (header.numFrequencies > 256)
			result = false;

		// check if the cube map size is sane
		if (header.cubeMapSx > 256 || header.cubeMapSy > 256)
			result = false;
	}

	if (result)
	{
		allocate(header.numFrequencies, header.cubeMapSx, header.cubeMapSy);

		if (fread(allValues, numValues * sizeof(Value), 1, file) != 1)
			result = false;
	}
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}

	return result;
}

bool CIRD::saveToFile(const char * filename)
{
	bool result = true;
	
	FILE * file = fopen(filename, "wb");
	
	if (file == nullptr)
		result = false;
	
	if (result)
	{
		if (fwrite(&header, sizeof(Header), 1, file) != 1)
			result = false;
	}
	
	if (result)
	{
		if (fwrite(allValues, numValues * sizeof(Value), 1, file) != 1)
			result = false;
	}
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
	
	return result;
}

int CIRD::allocate(const int numFrequencies, const int cubeMapSx, const int cubeMapSy)
{
	free();

	//
	
	numValues = 6 * numFrequencies * cubeMapSx * cubeMapSy;

	allValues = new Value[numValues];

	Value * valuePtr = allValues;

	for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
	{
		cube.faces[faceIndex].images.resize(numFrequencies);
	}

	for (int i = 0; i < numFrequencies; ++i)
	{
		for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
		{
			auto & image = cube.faces[faceIndex].images[i];
			
			image.values = valuePtr;

			valuePtr += cubeMapSx * cubeMapSy;
		}
	}

	return numValues;
}

void CIRD::free()
{
	for (int i = 0; i < 6; ++i)
		cube.faces[i].images.clear();

	delete [] allValues;
	allValues = nullptr;
	
	numValues = 0;
}

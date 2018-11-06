#include "fmt_cird.h"
#include <assert.h>
#include <stdio.h> // FILE
#include <string.h> // memset

static void logError(const char * msg)
{
	printf("error: %s\n", msg);
}

CIRD::Header::Header()
{
	assert(sizeof(*this) == 1024*4);
	
	// Initialize the entire header to zero.
	
	memset(this, 0, sizeof(*this));
}

CIRD::FrequencyTable::FrequencyTable()
{
	assert(sizeof(*this) == 1024*4);
	
	// Initialize the entire frequency table to zero.
	
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
	
	frequencyTable = FrequencyTable();
	
	if (numFrequencies > kMaxFrequencies)
		return false;
	
	header.version = 1000;
	header.numFrequencies = numFrequencies;
	header.cubeMapSx = cubeMapSx;
	header.cubeMapSy = cubeMapSy;
	
	memcpy(frequencyTable.frequencies, frequencies, sizeof(float) * numFrequencies);
	
	allocate(numFrequencies, cubeMapSx, cubeMapSy);
	
	return true;
}

bool CIRD::loadFromFile(const char * filename)
{
	bool result = true;

	FILE * file = fopen(filename, "rb");

	if (file == nullptr)
	{
		logError("failed to open file for read");
		result = false;
	}
	
	// read header

	if (result)
	{
		FourCC fourCC;
		
		if (fread(&fourCC, sizeof(FourCC), 1, file) != 1)
		{
			logError("failed to read file format identifier");
			result = false;
		}
		
		// check if the identifier is as expected
		if (memcmp(&fourCC, "cird", 4) != 0)
		{
			logError("file format identifier mismatch");
			result = false;
		}
	}
	
	if (result)
	{
		if (fread(&header, sizeof(Header), 1, file) != 1)
		{
			logError("failed to read header data");
			result = false;
		}

		// check if the version number is correct
		if (header.version != 1000)
		{
			logError("version number mismatch");
			result = false;
		}

		// check if the number of frequencies is sane
		if (header.numFrequencies > kMaxFrequencies)
		{
			logError("frequency table too big");
			result = false;
		}

		// check if the cube map size is sane
		if (header.cubeMapSx > 256 || header.cubeMapSy > 256)
		{
			logError("cube map dimensions too large");
			result = false;
		}
	}
	
	// read the frequency table
	
	if (result)
	{
		FourCC fourCC;
		
		if (fread(&fourCC, sizeof(FourCC), 1, file) != 1)
		{
			logError("failed to read file format identifier");
			result = false;
		}
		
		// check if the identifier is as expected
		if (memcmp(&fourCC, "freq", 4) != 0)
		{
			logError("frequency table identifier mismatch");
			result = false;
		}
	}
	
	if (result)
	{
		if (fread(&frequencyTable, sizeof(FrequencyTable), 1, file) != 1)
		{
			logError("failed to read frequency table");
			result = false;
		}
	}
	
	// read the cube map data
	
	if (result)
	{
		FourCC fourCC;
		
		if (fread(&fourCC, sizeof(FourCC), 1, file) != 1)
		{
			logError("failed to read data identifier");
			result = false;
		}
		
		// check if the identifier is as expected
		if (memcmp(&fourCC, "data", 4) != 0)
		{
			logError("data identifier mismatch");
			result = false;
		}
	}

	if (result)
	{
		allocate(header.numFrequencies, header.cubeMapSx, header.cubeMapSy);

		if (fread(allValues, numValues * sizeof(Value), 1, file) != 1)
		{
			logError("failed to read cube map data");
			result = false;
		}
	}
	
	//
	
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
	{
		logError("failed to open file for write");
		result = false;
	}
	
	// write header
	
	if (result)
	{
		if (fwrite("cird", 4, 1, file) != 1)
		{
			logError("failed to write file format identifier");
			result = false;
		}
	}
	
	if (result)
	{
		if (fwrite(&header, sizeof(Header), 1, file) != 1)
		{
			logError("failed to write header data");
			result = false;
		}
	}
	
	// write frequency table
	
	if (result)
	{
		if (fwrite("freq", 4, 1, file) != 1)
		{
			logError("failed to write file format identifier");
			result = false;
		}
	}
	
	if (result)
	{
		if (fwrite(&frequencyTable, sizeof(FrequencyTable), 1, file) != 1)
		{
			logError("failed to write frequency table");
			result = false;
		}
	}
	
	// write cube map data
	
	if (result)
	{
		if (fwrite("data", 4, 1, file) != 1)
		{
			logError("failed to write data identifier");
			result = false;
		}
	}
	
	if (result)
	{
		if (fwrite(allValues, numValues * sizeof(Value), 1, file) != 1)
		{
			logError("failed to write cube map data");
			result = false;
		}
	}
	
	//
	
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
	
	return result;
}

void CIRD::allocate(const int numFrequencies, const int cubeMapSx, const int cubeMapSy)
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
}

void CIRD::free()
{
	for (int i = 0; i < 6; ++i)
		cube.faces[i].images.clear();

	delete [] allValues;
	allValues = nullptr;
	
	numValues = 0;
}

#pragma once

/*

From: https://learnopengl.com/Advanced-OpenGL/Cubemaps

	GL_TEXTURE_CUBE_MAP_POSITIVE_X	Right
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X	Left
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y	Top
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	Bottom
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z	Back
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	Front
 
	See cubemaps_skybox.png in the documentation folder for reference.

The CIRD file format contains impulse-response data organized as a cube map.

A CIRD file is composed of six image arrays, one for each cube map face.
The cube map faces are organized according to the OpenGL specification.

Each image array contains N unique images, where each image encodes the
impulse-response at location (x, y) on the cube map face for frequency N
in the frequency array.

Preceeding the image data is a header. The header specifies the cube map
dimensions and the number of frequencies measured and stored inside the image
planes.

*/

#include <stdint.h>
#include <vector>

struct CIRD
{
	const static int kMaxFrequencies = 1024;
	
	typedef uint8_t FourCC[4];

	struct Header
	{
		uint32_t version; // Version number of the file format.
		uint32_t cubeMapSx; // Width of the cube map.
		uint32_t cubeMapSy; // Height of the cube map.
		uint32_t numFrequencies; // The number of valid frequencies in the frequencies table and the size of the image arrays.
		
		uint32_t reserved[1024 - 4];
		
		Header();
	};
	
	struct FrequencyTable
	{
		float frequencies[kMaxFrequencies];

		FrequencyTable();
	};

	struct Value
	{
		float real;
		float imag;
	};

	struct Image
	{
		Value * values;
	};

	struct Face
	{
		std::vector<Image> images;
	};

	struct Cube
	{
		Face faces[6];
	};

	Header header;
	
	FrequencyTable frequencyTable;

	Cube cube;

	Value * allValues = nullptr; // Linear array of all of the image data contained in the file.
	size_t numValues = 0;

	~CIRD();
	
	bool initialize(const int numFrequencies, const int cubeMapSx, const int cubeMapSy, const float * frequencies);
	
	bool loadFromFile(const char * filename);
	bool saveToFile(const char * filename);

	void allocate(const int numFrequencies, const int cubeMapSx, const int cubeMapSy);
	void free();
	
	int calcImageValueIndex(const int x, const int y) const
	{
		return x + y * header.cubeMapSx;
	}
};

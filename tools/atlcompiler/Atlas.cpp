#include "Precompiled.h"
#include "Atlas.h"
#include "FileStream.h"
#include "Path.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"

//

enum AtlasChunk
{
	AtlasChunk_Header,
	AtlasChunk_ImageData,
	AtlasChunk_ImageIndex,
	AtlasChunk_End,
	AtlasChunk_Undefined
};

static void WriteChunk(StreamWriter* writer, AtlasChunk chunk);
static void ReadChunk(StreamReader* reader, AtlasChunk* out_Chunk);

//

Atlas::Atlas()
{
	m_Texture = 0;
	m_ImagesByIndex = 0;
}

Atlas::~Atlas()
{
	delete[] m_ImagesByIndex;
	m_ImagesByIndex = 0;

	for (size_t i = 0; i < m_Images.size(); ++i)
		delete m_Images[i];
	m_Images.clear();
}

void Atlas::Setup(const std::vector<Atlas_ImageInfo*>& images, int sx, int sy)
{
	m_Texture = new Image();
	m_Texture->SetSize(sx, sy);
	
	for (size_t i = 0; i < images.size(); ++i)
		AddImage(images[i]);

	Compose();
}

void Atlas::Compose()
{
	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		const Atlas_ImageInfo* image = m_Images[i];

		image->m_Image->Blit(
			m_Texture,
			image->m_Position[0],
			image->m_Position[1]);
		
		#if 0
		ImagePixel c;
		c.r = 0;
		c.g = 255;
		c.b = 0;
		c.a = 255;
		
		m_Texture->SetPixel(image->m_Position[0], image->m_Position[1], c);
		m_Texture->SetPixel(image->m_Position[0] + image->m_Size[0] - 1, image->m_Position[1], c);
		m_Texture->SetPixel(image->m_Position[0] + image->m_Size[0] - 1, image->m_Position[1] + image->m_Size[1] - 1, c);
		m_Texture->SetPixel(image->m_Position[0], image->m_Position[1]+ image->m_Size[1] - 1, c);
		#endif
	}
}

void Atlas::AddImage(Atlas_ImageInfo* image)
{
	if (m_Texture)
	{
		UpdateImageInfo(image);
	}
	
	m_Images.push_back(image);
	m_ImagesByName[image->m_FileName] = image;

	// update image index

	if (image->m_Name != String::Empty)
	{
		ResInfo res(0, String::Empty, "texture", image->m_Name, Path::GetFileName(image->m_FileName));

		m_ImageIndex.Add(res);
	}
}

const Atlas_ImageInfo* Atlas::GetImage(const std::string& name) const
{
	std::map<std::string, Atlas_ImageInfo*>::const_iterator ptr = m_ImagesByName.find(name);
	
	if (ptr == m_ImagesByName.end())
		return 0;
	
	return ptr->second;
}

const Atlas_ImageInfo* Atlas::GetImage(int index) const
{
	if (index < 0 || index >= (int)m_ImageIndex.m_Resources.size())
		return 0;

	return m_ImagesByIndex[index];
}

void Atlas::UpdateImageInfo(Atlas_ImageInfo* image)
{
	const float x = (image->m_Position[0] + image->m_Offset[0]) / (float)m_Texture->m_Sx;
	const float y = (image->m_Position[1] + image->m_Offset[1]) / (float)m_Texture->m_Sy;
	const float sx = image->m_ImageSize[0] / (float)m_Texture->m_Sx;
	const float sy = image->m_ImageSize[1] / (float)m_Texture->m_Sy;
	
	image->SetupAtlas(image->m_ImageSize[0], image->m_ImageSize[1], x, y, sx, sy);
}

#if 1
void Atlas::Shrink()
{
	int sx = 0;
	int sy = 0;
	
	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		PointI max = m_Images[i]->m_Position.Add(m_Images[i]->m_Size);
		
		if (max[0] > sx)
			sx = max[0];
		if (max[1] > sy)
			sy = max[1];
	}
	
	Image* temp = new Image();
	
	temp->SetSize(sx, sy);
	
	m_Texture->Blit(temp, 0, 0);
	
	delete m_Texture;
	m_Texture = 0;
	
	m_Texture = temp;
	
	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		UpdateImageInfo(m_Images[i]);
	}
}
#else
PointI Atlas::CalcSize() const
{
	PointI result(0, 0);
	
	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		PointI max = m_Images[i]->m_Position.Add(m_Images[i]->m_Size);
		
		if (max[0] > result[0])
			result[0] = max[0];
		if (max[1] > result[1])
			result[1] = max[1];
	}
	
	return result;
}
#endif

void Atlas::Save(const std::string& fileName, IImageLoader* loader) const
{
	std::string textureFileName = Path::MakeRelative("", fileName, fileName + ATLAS_EXTENSION);
	
	//
	
	Stream* stream = new FileStream(fileName.c_str(), OpenMode_Write);

	StreamWriter* writer = new StreamWriter(stream, true);
	
	// write header

	WriteChunk(writer, AtlasChunk_Header);

	writer->WriteInt32((int)m_Images.size());
	writer->WriteText_Binary(textureFileName);

	// write image data

	WriteChunk(writer, AtlasChunk_ImageData);
	
	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		const Atlas_ImageInfo* image = m_Images[i];

		// write image file name and name

		//printf("atlas_image: filename: %s\n", Path::GetFileName(image->m_FileName).c_str());
		//printf("atlas_image: name: %s\n", image->m_Name.c_str());

		writer->WriteText_Binary(Path::GetFileName(image->m_FileName));
		writer->WriteText_Binary(image->m_Name);

		// calculate normalized position and size within texture

		const float x = (image->m_Position[0] + image->m_Offset[0]) / (float)m_Texture->m_Sx;
		const float y = (image->m_Position[1] + image->m_Offset[1]) / (float)m_Texture->m_Sy;
		const float sx = image->m_ImageSize[0] / (float)m_Texture->m_Sx;
		const float sy = image->m_ImageSize[1] / (float)m_Texture->m_Sy;

		// write image size in pixels
		
		writer->WriteInt32(image->m_ImageSize[0]);
		writer->WriteInt32(image->m_ImageSize[1]);
		
		// write texture coord

		writer->WriteFloat(x);
		writer->WriteFloat(y);

		// write texture size

		writer->WriteFloat(sx);
		writer->WriteFloat(sy);
	}

	// write image index

	WriteChunk(writer, AtlasChunk_ImageIndex);

	m_ImageIndex.SaveBinary(stream);

	// end

	WriteChunk(writer, AtlasChunk_End);

	delete writer;
	writer = 0;

	// write texture

	if (loader)
	{
		loader->Save(*m_Texture, fileName + ATLAS_EXTENSION);
	}
}

void Atlas::Load(Stream* stream, IImageLoader* loader)
{
	StreamReader reader(stream, false);

	AtlasChunk chunk = AtlasChunk_Undefined;

	int32_t imageCount = 0;
	
	do
	{
		ReadChunk(&reader, &chunk);

		switch (chunk)
		{
		case AtlasChunk_Header:
			{
				imageCount = reader.ReadInt32();
				
				m_TextureFileName = reader.ReadText_Binary();
			}
			break;

		case AtlasChunk_ImageData:
			{
				for (int32_t i = 0; i < imageCount; ++i)
				{
					// read image file name and name

					const std::string fileName = reader.ReadText_Binary();
					const std::string name = reader.ReadText_Binary();

					const int32_t imageSx = reader.ReadInt32();
					const int32_t imageSy = reader.ReadInt32();
					
					// read texture coord

					const float x = reader.ReadFloat();
					const float y = reader.ReadFloat();

					// read texture size

					const float sx = reader.ReadFloat();
					const float sy = reader.ReadFloat();
					
					Atlas_ImageInfo* image = new Atlas_ImageInfo(fileName, name, imageSx, imageSy, x, y, sx, sy);
					
					AddImage(image);
				}
			}
			break;

		case AtlasChunk_ImageIndex:
			{
				m_ImageIndex.LoadBinary(stream);
			}
			break;

		case AtlasChunk_End:
			{
				// nop
			}
			break;
				
		case AtlasChunk_Undefined:
			throw ExceptionVA("undefined atlas chunk");
		}

	} while (chunk != AtlasChunk_End);

	// build image index
	
	if (m_ImageIndex.m_Resources.size() > 0)
	{
		m_ImagesByIndex = new const Atlas_ImageInfo*[m_ImageIndex.m_Resources.size()];
		
		for (size_t i = 0; i < m_ImageIndex.m_Resources.size(); ++i)
		{
			const ResInfo& res = m_ImageIndex.m_Resources[i];
			
			m_ImagesByIndex[i] = GetImage(res.m_FileName);
		}
	}
}

//

static void WriteChunk(StreamWriter* writer, AtlasChunk chunk)
{
	int8_t temp = (int8_t)chunk;

	writer->WriteInt8(temp);
}

static void ReadChunk(StreamReader* reader, AtlasChunk* out_Chunk)
{
	int8_t temp = reader->ReadInt8();

	*out_Chunk = (AtlasChunk)temp;
}

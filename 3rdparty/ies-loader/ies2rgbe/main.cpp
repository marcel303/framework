#include "ies_Loader.h"
#include "rgbe.h"

//#include <io.h>
#include <fstream>
#include <iostream>

struct IESOutputData
{
	std::uint32_t width;
	std::uint32_t height;
	std::uint32_t channel;
	std::vector<float> stream;
};

bool IES2HDR(const std::string& path, const std::string& outpath, IESFileInfo& info)
{
	std::fstream stream(path, std::ios_base::binary | std::ios_base::in);
	if (!stream.is_open())
	{
		std::cout << "IES2HDR Error: Failed to open file :" << path << std::endl;
		return false;
	}

	stream.seekg(0, std::ios_base::end);
	std::streamsize streamSize = stream.tellg();
	stream.seekg(0, std::ios_base::beg);

	std::vector<std::uint8_t> IESBuffer;
	IESBuffer.resize(streamSize + 1);
	stream.read((char*)IESBuffer.data(), IESBuffer.size());

	IESLoadHelper IESLoader;
	if (!IESLoader.load((char*)IESBuffer.data(), streamSize, info))
		return false;

	IESOutputData HDRdata;
	HDRdata.width = 256;
	HDRdata.height = 1;
	HDRdata.channel = 3;
	HDRdata.stream.resize(HDRdata.width * HDRdata.channel);

	if (!IESLoader.saveAs1D(info, HDRdata.stream.data(), HDRdata.width, HDRdata.channel))
		return false;

	FILE* fp = std::fopen(outpath.c_str(), "wb");
	if (!fp)
	{
		std::cout << "IES2HDR Error: Failed to create file : " << outpath << path << std::endl;;
		return false;
	}

	rgbe_header_info hdr;
	hdr.valid = true;
	hdr.gamma = 1.0;
	hdr.exposure = 1.0;
	std::memcpy(hdr.programtype, "RADIANCE", 9);

	RGBE_WriteHeader(fp, HDRdata.width, HDRdata.height, &hdr);
	RGBE_WritePixels_RLE(fp, HDRdata.stream.data(), HDRdata.width, HDRdata.height);
	std::fclose(fp);

	return true;
}

bool IES2HDR(const std::string& path, IESFileInfo& info)
{
	std::size_t i = path.rfind(".");
	std::string out = path.substr(0, i) + ".hdr";

	return IES2HDR(path, out, info);
}

int main(int argc, const char* argv[])
{
	try
	{
		if (argc <= 1)
		{
			std::cout << "IES2HDR Info: ERROR! Please enter a path to a IES file/directory." << std::endl;
			std::system("pause");
			return EXIT_FAILURE;
		}
		
		if (strstr(argv[1], ".ies") != 0)
		{
			IESFileInfo info;
			if (!IES2HDR(argv[1], info))
				std::cout << "IES2HDR Error: " << info.error() << std::endl;
		}
		else
		{
/*
			std::string path = drive;
			path += dir;
			path += fname;
			path += "/*.ies";

			_finddata_t fileinfo;
			std::memset(&fileinfo, 0, sizeof(fileinfo));

			auto handle = _findfirst(path.data(), &fileinfo);
			if (handle != -1)
			{
				do
				{
					if (!(fileinfo.attrib & _A_SUBDIR))
					{
						std::string filename;
						filename = drive;
						filename += dir;
						filename += fname;
						filename += "/";
						filename += fileinfo.name;

						std::cout << "IES2HDR Info: Converting IES to HDR :" << filename << std::endl;

						IESFileInfo info;
						if (!IES2HDR(filename, info))
							std::cout << "IES2HDR Error: " << info.error() << std::endl;
					}
				} while (_findnext(handle, &fileinfo) == 0);

				_findclose(handle);
			}
*/
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	std::system("pause");
	return 0;
}

// makebuild
// - create build script (make file) for processing resources:
//
// input:
// - resource list
// output:
// - build script (make file)
//
// make file actions:
// - copy resources
// - build resources for content scale x1
// - build resources for content scale x2

#include <algorithm>
#include "Arguments.h"
#include "Exception.h"
#include "FileStream.h"
#include "FileStreamExtends.h"
#include "MemoryStream.h"
#include "Parse.h"
#include "Path.h"
#include "ResIndex.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringBuilder.h"
#include "StringEx.h"

static uint32_t DATASET_COUNT = 5;

static void WriteImgFromTga(StreamWriter& writer, std::string tga, std::string img);
static void WriteSfxFromWav(StreamWriter& writer, std::string wav, std::string sfx);
static void WriteVgcFromVg(StreamWriter& writer, std::string vg, std::string vgc, int contentScale);
//static void WriteVgcPngFromVg(StreamWriter& writer, std::string vg, std::string vgcPng);
//static void WriteVgcSilFromVg(StreamWriter& writer, std::string vg, std::string vgcSil);
static void WriteVccFromVc(StreamWriter& writer, std::string vc, std::string vcc);
static void WritePvr4FromPng(StreamWriter& writer, std::string png, std::string pvr4);
//static void WritePvr2FromPng(StreamWriter& writer, std::string png, std::string pvr2);
static void WriteDdsFromPng(StreamWriter& writer, std::string png, std::string dds);
static void WriteDdsAlphaFromPng(StreamWriter& writer, std::string png, std::string dds);
static void WriteFntFromPng(StreamWriter& writer, std::string png, std::string fnt, int contentScale);
static void WriteCopy(StreamWriter& writer, std::string src, std::string dst);
static void WriteCopyBatch(StreamWriter& writer, std::vector<std::string> copyList, std::string dstDirectory);
static void WriteAtlFromImgList(StreamWriter& writer, std::vector<std::string> inputList, std::string atl, int size);
static void WritePkgFromResourceList(StreamWriter& writer, std::vector<std::string> inputList, std::string pkg);
static void WriteImageScaleDown(StreamWriter& writer, std::string src, std::string dst, int scaleDown);
static void WritePostProcess(StreamWriter& writer);

static bool IncludeInPackage(const std::string& fileName, std::map<std::string, int> & excludedFromPackage)
{
	const std::string ext = Path::GetExtension(fileName);

	if (ext == "tga" ||
		ext == "img" ||
		ext == "mp3" ||
		ext == "at3" ||
		ext == "ogg" ||
		ext == "pvr4" ||
		ext == "dds" ||
		ext == "pkg" ||
		ext == "bin" ||
		ext == "gz")
		return false;
	
	if (excludedFromPackage.count(fileName) != 0)
		return false;
	
	return true;
}

class TextureIndexItem
{
public:
	int mDataSetId;
	std::string mName;

	TextureIndexItem()
	{
	}

	TextureIndexItem(int dataSetId, std::string name)
	{
		mDataSetId = dataSetId;
		mName = name;
	}

	bool operator<(const TextureIndexItem& item) const
	{
		return mName < item.mName;
	}
};

static void WriteTextureIndex(StreamWriter& writerHpp, StreamWriter& writerCpp, std::string hppFileName, std::vector<TextureIndexItem> itemList);

class Settings
{
public:
	Settings()
	{
		platform = "ios";

		atlasSize = 0;
		
		compileLQ = false;
		compileHQ = false;
		
		rebuild = false;
	}

	void Validate()
	{
		if (platform != "ios" && platform != "ios_hd" && platform != "psp" && platform != "win" && platform != "mac" && platform != "bbos")
			throw ExceptionVA("unknown platform: %s", platform.c_str());
		if (src.empty())
			throw ExceptionVA("source not set");
		if (dst.empty())
			throw ExceptionVA("destination not set");
		if (atlasFileName.empty())
			throw ExceptionVA("texture atlas filename not set");
		if (atlasSize <= 0)
			throw ExceptionVA("invalid texture atlas size: %d", atlasSize);
	}

	std::string platform;
	std::string src;
	std::string dst;
	std::string atlasFileName;
	int atlasSize;
	
	//
	
	std::string outputDir;
	std::string stagingDir;
	std::string resourceListName;
	std::string textureListName;
	bool compileLQ;
	bool compileHQ;
	
	//
	
	bool rebuild;
};

class ProcessingItem
{
	public:
		ProcessingItem()
		{
			DataSetId = -1;
		}
		
		ProcessingItem(int dataSetId, std::string src, std::string dst, std::string name)
		{
			DataSetId = dataSetId;
			Src = src;
			Dst = dst;
			Name = name;
		}
		
		int DataSetId;
		std::string Src;
		std::string Dst;
		std::string Name;
};

static Settings gSettings;

static uint32_t gDepIdx = 0;

static std::string AllocAndWriteDep(StreamWriter & writer)
{
	std::string name = String::Format("dep%06u", gDepIdx);
	
	gDepIdx++;
	
	writer.WriteText(name.c_str());
	writer.WriteLine(" :");
	
	return name;
}

static std::string WriteDep(StreamWriter & writer, const std::string & src, const std::string & dst)
{
	if (gSettings.rebuild == false)
	{
		writer.WriteText(dst.c_str());
		writer.WriteText(" : ");
		writer.WriteLine(src.c_str());
	}
	else
	{
		return AllocAndWriteDep(writer);
	}
	return dst;
}

int main(int argc, char* argv[])
{
	try
	{
		// parse arguments
		
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				ARGS_CHECKPARAM(1);

				gSettings.src = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				gSettings.dst = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-a"))
			{
				ARGS_CHECKPARAM(1);

				gSettings.atlasFileName = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-as"))
			{
				ARGS_CHECKPARAM(1);

				gSettings.atlasSize = Parse::Int32(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-p"))
			{
				ARGS_CHECKPARAM(1);

				gSettings.platform = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-f"))
			{
				gSettings.rebuild = true;
				
				i += 1;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}

		// validate arguments

		gSettings.Validate();
		
		// determine output directory based on selected platform

		if (gSettings.platform == "ios")
		{
			gSettings.outputDir = "IOSDATA/";
			gSettings.stagingDir = "IOSDATA-Staging/";
			gSettings.compileLQ = true;
			gSettings.compileHQ = true;
			gSettings.resourceListName = "UsgResourcesIos";
			gSettings.textureListName = "UsgTexturesIos";
		}
		else if (gSettings.platform == "ios_hd")
		{
			gSettings.outputDir = "IOSDATA2/";
			gSettings.stagingDir = "IOSDATA2-Staging/";
			gSettings.compileLQ = false;
			gSettings.compileHQ = true;
			gSettings.resourceListName = "UsgResourcesIosHD";
			gSettings.textureListName = "UsgTexturesIosHD";
			
			DATASET_COUNT = 6;
		}
		else if (gSettings.platform == "psp")
		{
			gSettings.outputDir = "PSPDATA/";
			gSettings.stagingDir = "PSPDATA-Staging/";
			gSettings.compileLQ = true;
			gSettings.compileHQ = false;
			gSettings.resourceListName = "UsgResourcesPsp";
			gSettings.textureListName = "UsgTexturesPsp";
			
			DATASET_COUNT = 5;
		}
		else if (gSettings.platform == "win")
		{
			gSettings.outputDir = "GAMEDATA/";
			gSettings.stagingDir = "GAMEDATA-Staging/";
			gSettings.compileLQ = false;
			gSettings.compileHQ = true;
			gSettings.resourceListName = "UsgResourcesWin";
			gSettings.textureListName = "UsgTexturesWin";
			
			DATASET_COUNT = 5;
		}
		else if (gSettings.platform == "mac")
		{
			gSettings.outputDir = "MACOS/";
			gSettings.stagingDir = "MACOS-Staging/";
			gSettings.compileLQ = false;
			gSettings.compileHQ = true;
			gSettings.resourceListName = "UsgResourcesMac";
			gSettings.textureListName = "UsgTexturesMac";
			
			DATASET_COUNT = 5;
		}
		else if (gSettings.platform == "bbos")
		{
			gSettings.outputDir = "BBDATA/";
			gSettings.stagingDir = "BBDATA-Staging/";
			gSettings.compileLQ = false;
			gSettings.compileHQ = true;
			gSettings.resourceListName = "UsgResourcesBbos";
			gSettings.textureListName = "UsgTexturesBbos";
			
			DATASET_COUNT = 5;
		}
		else
			throw ExceptionVA("unknown platform: %s", gSettings.platform.c_str());

		// load resource index
		
		FileStream resIndexStream;
		
		resIndexStream.Open(gSettings.src.c_str(), OpenMode_Read);
		
		ResIndex resIndex;
		
		resIndex.Load(&resIndexStream);
		
		resIndexStream.Close();
		
		resIndex.FilterByPlatform(gSettings.platform);
		
		// sort resources by type
		
		std::map<std::string, std::vector<ProcessingItem> > resourceListByResourceType;

		for (size_t i = 0; i < resIndex.m_Resources.size(); ++i)
		{
			const ResInfo& resInfo = resIndex.m_Resources[i];
			
			std::string type = resInfo.m_Type;
			
			if (resourceListByResourceType.count(type) == 0)
			{
				resourceListByResourceType[type] = std::vector<ProcessingItem>();
			}
			
			std::string name = resInfo.m_Name;
			std::string src = resInfo.m_FileName;
			std::string dst = src;
			
			if (type == "vector")
				dst = Path::ReplaceExtension(src, "vgc");
			if (type == "composition")
				dst = Path::ReplaceExtension(src, "vcc");
			if (type == "image")
			{
				if (gSettings.platform == "psp")
					dst = Path::ReplaceExtension(src, "img.gz");
				else
					dst = Path::ReplaceExtension(src, "img");
			}
			if (type == "pvr4")
				dst = Path::ReplaceExtension(src, "pvr4");
			if (type == "dds" || type == "dds_alpha")
			{
				if (gSettings.platform == "psp")
					dst = Path::ReplaceExtension(src, "dds.gz");
				else
					dst = Path::ReplaceExtension(src, "dds");
			}
			if (type == "font")
				dst = Path::ReplaceExtension(src, "fnt");
			if (type == "sound")
				dst = Path::ReplaceExtension(src, "sfx");
			
			resourceListByResourceType[resInfo.m_Type].push_back(ProcessingItem(resInfo.m_DataSetId, src, dst, name));
		}
		
		// open output stream
		
		FileStream outputStream;
		
		outputStream.Open(gSettings.dst.c_str(), OpenMode_Write);
		
		StreamWriter writer(&outputStream, false);
		
		//
		
		writer.WriteLine("include ../Makefile.defs");
		writer.WriteLine("include MakeResources.defs");
		
		writer.WriteLine(String::Format("STG_DIR = %s", gSettings.stagingDir.c_str()));
		writer.WriteLine(String::Format("DST_DIR = %s", gSettings.outputDir.c_str()));
		
		//
		
		std::vector<std::string> actionList;
		std::vector<std::string> outputList;
		std::map<std::string, int> excludedFromPackage;
		
		// emit: resource index
		
		{
			std::string action = "resource_index";
			
			writer.WriteLine(String::FormatC("%s : dummy", action.c_str()));
			writer.WriteLine(String::FormatC("\t@echo \"creating resource index\""));
			writer.WriteLine(String::FormatC("\t@$(TOOL_RESCOMPILE) -c %s -name Resources -o-index %sresources.idx -o-header Classes/%s.h -o-source Classes/%s.cpp -p %s",
				gSettings.src.c_str(),
				gSettings.outputDir.c_str(),
				gSettings.resourceListName.c_str(),
				gSettings.resourceListName.c_str(),
				gSettings.platform.c_str()));
			
			actionList.push_back(action);
		}
		
		// emit: copy resources
		
		{
			std::string action = "copy_resources";
		
			writer.WriteLine(action + " : dummy");
			
			writer.WriteLine("\t@echo \"copying resources\"");
			writer.WriteLine("ifeq ($(OS), win)");
			//writer.WriteLine("\t@$(CP_UPDATE) -R -P $(SRC_DIR)*.* $(STG_DIR)");
			//writer.WriteLine("\t@$(CP_UPDATE) -R -P $(SRC_DIR)* $(STG_DIR)");
			writer.WriteLine("\t@rsync -u -t $(SRC_DIR)* $(STG_DIR)");
			writer.WriteLine("else");
			//writer.WriteLine("\t@$(CP_UPDATE) -R -P $(SRC_DIR) $(STG_DIR)");
			writer.WriteLine("\t@rsync -u -t $(SRC_DIR)* $(STG_DIR)");
			writer.WriteLine("endif");
			
			actionList.push_back(action);
		}
		
		// emit: process resources
		
		if (1)
		{
			std::string action = "process";
			
			//writer.WriteLine(action + " : dummy");
			std::vector<std::string> deps;
			
			for (std::map<std::string, std::vector<ProcessingItem> >::iterator kvp = resourceListByResourceType.begin(); kvp != resourceListByResourceType.end(); ++kvp)
			{
				std::string type = kvp->first;
				std::vector<ProcessingItem> itemList = kvp->second;
				
				deps.push_back(AllocAndWriteDep(writer));
				writer.WriteLine("\t@echo \"========================================\"");
				writer.WriteLine("\t@echo \"PROCESSING: " + type + "\"");
				writer.WriteLine("\t@echo \"========================================\"");
				
				for (size_t i = 0; i < itemList.size(); ++i)
				{				
					ProcessingItem item = itemList[i];
					
					if (type == "vector")
					{
						// note: vector graphics may reference image data.
						// if the image data is changed, the VGs won't rebuild
						// unless forced..
						
						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						
						// base version. required because of the vgc file output
						WriteVgcFromVg(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst, 1);
						if (gSettings.compileLQ)
							WriteVgcFromVg(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + std::string("lq_") + item.Dst, 1);
						if (gSettings.compileHQ)
							WriteVgcFromVg(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + std::string("hq_") + item.Dst, 2);
						//WriteVgcFromVg(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + std::string("uq_") + item.Dst, 4);
					}
					if (type == "composition")
					{
						// note: vector graphics are referenced at run-time, no direct
						// deps exist.

						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WriteVccFromVc(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst);
					}
					if (type == "image")
					{
//						deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WriteImgFromTga(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst);
					}
					if (type == "pvr4")
					{
						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WritePvr4FromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst);
					}
					if (type == "dds")
					{
						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WriteDdsFromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst); 
					}
					if (type == "dds_alpha")
					{
						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WriteDdsAlphaFromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst); 
					}
					if (type == "font")
					{
						// note: if the font image is changed, the font will not be rebuilt,
						// due to missing deps, unless forced.
						
						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WriteFntFromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst, 1);
						if (gSettings.compileLQ)
							WriteFntFromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + std::string("lq_") + item.Dst, 2);
						if (gSettings.compileHQ)
							WriteFntFromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + std::string("hq_") + item.Dst, 1);
						//WriteFntFromPng(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + std::string("uq_") + item.Dst, 1);
						
						if (gSettings.compileLQ)
							outputList.push_back(std::string("lq_") + item.Dst);
						if (gSettings.compileHQ)
							outputList.push_back(std::string("hq_") + item.Dst);
						//outputList.push_back(std::string("uq_") + item.Dst);
					}
					if (type == "sound")
					{
						//deps.push_back(AllocAndWriteDep(writer));
						deps.push_back(WriteDep(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst));
						WriteSfxFromWav(writer, "$(STG_DIR)" + item.Src, "$(STG_DIR)" + item.Dst);
					}
				}
			}
			
			writer.WriteText(action + " : dummy");
			for (size_t i = 0; i < deps.size(); ++i)
			{
				writer.WriteText(" ");
				writer.WriteText(deps[i].c_str());
			}
			writer.WriteLine();
			
			actionList.push_back(action);
		}
		else
		{
			printf("warning: processing disabled\n");
		}
		
		if (1)
		{
			for (std::map<std::string, std::vector<ProcessingItem> >::iterator kvp = resourceListByResourceType.begin(); kvp != resourceListByResourceType.end(); ++kvp)
			{
				std::string type = kvp->first;
				std::vector<ProcessingItem> itemList = kvp->second;
				
				if (type != "image" &&
					type != "font" &&
					type != "sound" &&
					type != "vector" &&
					type != "composition" &&
					type != "pvr4" &&
					type != "dds" &&
					type != "dds_alpha" &&
					type != "binary")
					continue;
				
				bool typExcludedFromPackage =
					type == "binary" &&
					(gSettings.platform == "ios" ||
					 gSettings.platform == "ios_hd");
				
				for (size_t i = 0; i < itemList.size(); ++i)
				{
					const ProcessingItem& item = itemList[i];

					// skip resources irrelevant to the selected platform

					if (String::EndsWith(item.Dst, ".mp3") && (gSettings.platform != "ios" && gSettings.platform != "ios_hd"))
						continue;
					if (String::EndsWith(item.Dst, ".at3") && (gSettings.platform != "psp"))
						continue;
					if (String::EndsWith(item.Dst, ".ogg") && (gSettings.platform != "win" && gSettings.platform != "mac" && gSettings.platform != "ios_hd" && gSettings.platform != "bbos"))
						continue;
					
					outputList.push_back(item.Dst);
					
					if (typExcludedFromPackage)
						excludedFromPackage[item.Dst] = 1;
				}
			}
		}
			
		// emit: texture atlas
		
		std::vector<TextureIndexItem> atlasTextureList;

		for (uint32_t dataSetId = 0; dataSetId < DATASET_COUNT; ++dataSetId)
		{
			std::string action = String::FormatC("atlas%d", dataSetId);
			
			writer.WriteLine(action + " : dummy");
			
			std::vector<std::string> inputListLQ;
			std::vector<std::string> inputListHQ;
			
			for (std::map<std::string, std::vector<ProcessingItem> >::iterator kvp = resourceListByResourceType.begin(); kvp != resourceListByResourceType.end(); ++kvp)
			{
				std::string type = kvp->first;
				std::vector<ProcessingItem> itemList = kvp->second;
				
				if (type != "vector" &&
					type != "font" &&
					type != "texture")
					continue;
				
				for (size_t i = 0; i < itemList.size(); ++i)
				{
					ProcessingItem item = itemList[i];

					if (item.DataSetId != dataSetId)
						continue;
					
					if (type == "vector")
					{
						if (gSettings.compileLQ)
						{
							// lq render
							
							inputListLQ.push_back("$(STG_DIR)" + std::string("lq_") + Path::ReplaceExtension(item.Src, "vgc.png"));
							inputListLQ.push_back("$(STG_DIR)" + std::string("lq_") + Path::ReplaceExtension(item.Src, "vgc.sil.png"));
						}

						if (gSettings.compileHQ)
						{
							// hq render
							
							inputListHQ.push_back("$(STG_DIR)" + std::string("hq_") + Path::ReplaceExtension(item.Src, "vgc.png"));
							inputListHQ.push_back("$(STG_DIR)" + std::string("hq_") + Path::ReplaceExtension(item.Src, "vgc.sil.png"));
						}
					}
					if (type == "font")
					{
						if (gSettings.compileLQ)
							inputListLQ.push_back("$(STG_DIR)" + std::string("lq_") + Path::ReplaceExtension(item.Src, "fnt.png"));
						if (gSettings.compileHQ)
							inputListHQ.push_back("$(STG_DIR)" + std::string("hq_") + Path::ReplaceExtension(item.Src, "fnt.png"));
					}
					if (type == "texture")
					{
						std::string inputName = Path::StripExtension(item.Name);
						std::string input = "$(STG_DIR)" + Path::ReplaceExtension(item.Src, "png") + ":" + inputName;

						inputListLQ.push_back(input);
						inputListHQ.push_back(input);

						atlasTextureList.push_back(TextureIndexItem(dataSetId, inputName));
					}
				}
			}
			
			std::string atlasFileNameLq = String::FormatC("lq_%d_%s", dataSetId, gSettings.atlasFileName.c_str());
			std::string atlasFileNameHq = String::FormatC("hq_%d_%s", dataSetId, gSettings.atlasFileName.c_str());

			if (gSettings.compileLQ)
			{
				WriteAtlFromImgList(writer, inputListLQ, "$(STG_DIR)" + atlasFileNameLq, gSettings.atlasSize * 1);
			}
			if (gSettings.compileHQ)
			{
				WriteAtlFromImgList(writer, inputListHQ, "$(STG_DIR)" + atlasFileNameHq, gSettings.atlasSize * 2);
			}
			
			std::string ext = ".tga";
			
			if (gSettings.platform == "psp")
			ext = ".tga.gz";
			
			if (gSettings.compileLQ)
			{
				outputList.push_back(atlasFileNameLq);
				outputList.push_back(atlasFileNameLq + ext);
			}
			if (gSettings.compileHQ)
			{
				outputList.push_back(atlasFileNameHq);
				outputList.push_back(atlasFileNameHq + ext);
			}
			
			actionList.push_back(action);
		}

		// emit: texture index
		
		MemoryStream textureIndexCppStream;
		MemoryStream textureIndexHppStream;

		std::string cppFileName = String::Format("Classes/%s.cpp", gSettings.textureListName.c_str());
		std::string hppFileName = String::Format("Classes/%s.h", gSettings.textureListName.c_str());

		StreamWriter textureIndexCppWriter(&textureIndexCppStream, false);
		StreamWriter textureIndexHppWriter(&textureIndexHppStream, false);

		WriteTextureIndex(textureIndexHppWriter, textureIndexCppWriter, hppFileName, atlasTextureList);

		FileStreamExtents::OverwriteIfChanged(&textureIndexCppStream, cppFileName);
		FileStreamExtents::OverwriteIfChanged(&textureIndexHppStream, hppFileName);

		// emit: package file

		{
			std::string action = "resource_package";
			
			writer.WriteLine(String::FormatC("%s : dummy", action.c_str()));

			std::string packageName = "resources.pkg";

			std::vector<std::string> inputList;

			for (size_t i = 0; i < outputList.size(); ++i)
			{
				if (!IncludeInPackage(outputList[i], excludedFromPackage))
					continue;

				inputList.push_back("$(STG_DIR)" + outputList[i]);
			}

			WritePkgFromResourceList(writer, inputList, "$(STG_DIR)" + packageName);

			outputList.push_back(packageName);

			actionList.push_back(action);
		}

		// emit: store
		
		{
			std::string action = "store";
			
			writer.WriteLine(action + " : dummy");
			
			writer.WriteLine("\t@echo \"store\"");
			
			std::vector<std::string> copyList;
			
			for (size_t i = 0; i < outputList.size(); ++i)
			{
				if (IncludeInPackage(outputList[i], excludedFromPackage))
					continue;
				
				copyList.push_back("$(STG_DIR)" + outputList[i]);
			}
			
			WriteCopyBatch(writer, copyList, "$(DST_DIR)");
			
			writer.WriteLine("\t@echo \"store: done. copied " + String::Format("%lu", copyList.size()) + " files\"");
			
			actionList.push_back(action);
		}
		
		// emit: post process
		
		{
			std::string action = "post_process";
			
			writer.WriteLine(String::FormatC("%s : dummy", action.c_str()));
			
			WritePostProcess(writer);
			
			actionList.push_back(action);
		}
		
		// emit: build order
		
		writer.WriteText("all :");
		
		for (size_t i = 0; i < actionList.size(); ++i)
		{
			writer.WriteText(" " + actionList[i]);
		}
		
		writer.WriteLine();
		writer.WriteLine("\t@echo [done]");
		
		outputStream.Close();
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}

static void WriteImgFromTga(StreamWriter& writer, std::string tga, std::string img)
{
	writer.WriteLine("\t@echo \"processing: " + tga + " => " + img + "\"");
	
	if (gSettings.platform == "ios_hd" || gSettings.platform == "win" || gSettings.platform == "mac" || gSettings.platform == "bbos")
	{
		writer.WriteLine("\t@$(TOOL_TGATOOL) -d 32 -c " + tga + " -o " + img);
	}
	else
	{
		writer.WriteLine("\t@$(TOOL_TGATOOL) -d 16 -c " + tga + " -o " + img);
	}

	if (gSettings.platform == "psp")
	{
		writer.WriteLine("\t@$(TOOL_PSP_FIXDDS) tga " + img + " " + img);
		writer.WriteLine("\t@$(CP_UPDATE) " + img + " " + img + ".tmp");
		writer.WriteLine("\t@$(TOOL_GZIP) -f -1 -c " + img + ".tmp > " + img);
	}
}

static void WriteSfxFromWav(StreamWriter& writer, std::string wav, std::string sfx)
{
#if 0
	#warning WriteSfxFromWav disabled
	return;
#endif
	writer.WriteLine("\t@echo \"processing: " + wav + " => " + sfx + "\"");
	writer.WriteLine("ifeq ($(OS), win)");
	writer.WriteLine("\t@./sox " + wav + " -b 16 temp.wav channels 1 rate 44100");
	writer.WriteLine("\t@cp temp.wav " + sfx);
	writer.WriteLine("else");
	//writer.WriteLine("\t@$(TOOL_AFCONVERT) -f caff -d LEI16@8000 -c 1 " + wav + " " + sfx);
	if (gSettings.platform == "ios")
		writer.WriteLine("\t@$(TOOL_AFCONVERT) -f WAVE -d LEI16@8000 -c 1 " + wav + " " + sfx);
	else
		writer.WriteLine("\t@$(TOOL_AFCONVERT) -f WAVE -d LEI16@22050 -c 1 " + wav + " " + sfx);
	writer.WriteLine("endif");
}

static void WriteVgcFromVg(StreamWriter& writer, std::string vg, std::string vgc, int contentScale)
{
#if 0
	#warning WriteVgcFromVg disabled
	return;
#endif
	writer.WriteLine("\t@echo \"processing: " + vg + " => " + vgc + "\"");
	writer.WriteLine("\t@$(TOOL_VECCOMPILE) -c " + vg + " -o " + vgc + " -q 5 -n -s " + String::Format("%d", contentScale));
}

/*static void WriteVgcPngFromVg(StreamWriter& writer, std::string vg, std::string vgcPng)
{
	writer.WriteLine("\t@echo \"processing: " + vg + " => " + vgcPng + "\"");
	writer.WriteLine("\t@echo \"stub: vgc.png from vg\"");
}*/

/*static void WriteVgcSilFromVg(StreamWriter& writer, std::string vg, std::string vgcSil)
{
	writer.WriteLine("\t@echo \"processing: " + vg + " => " + vgcSil + "\"");
	writer.WriteLine("\t@echo \"stub: vgc.sil from vg\"");
}*/

static void WriteVccFromVc(StreamWriter& writer, std::string vc, std::string vcc)
{
	writer.WriteLine("\t@echo \"processing: " + vc + " => " + vcc + "\"");
	writer.WriteLine("\t@$(TOOL_VECCOMPILE_VC) -c " + vc + " -o " + vcc);
}

static void WritePvr4FromPng(StreamWriter& writer, std::string png, std::string pvr4)
{
	writer.WriteLine("\t@echo \"processing: " + png + " => " + pvr4 + "\"");
	writer.WriteLine("ifeq ($(OS), win)");
	writer.WriteLine("\t@cp " + png + " " + pvr4);
	writer.WriteLine("else");
	if (gSettings.platform == "ios" || gSettings.platform == "ios_hd")
		writer.WriteLine("\t@$(TOOL_TEXTURETOOL) -f PVR -m -e PVRTC --bits-per-pixel-4 $(CHANNEL_WEIGHTING) -o " + pvr4 + " " + png);
	else
		writer.WriteLine("\t@cp " + png + " " + pvr4);
	writer.WriteLine("endif");
}

/*static void WritePvr2FromPng(StreamWriter& writer, std::string png, std::string pvr2)
{
	writer.WriteLine("\t@echo \"processing: " + png + " => " + pvr2 + "\"");
}*/

static void WriteDdsFromPng(StreamWriter& writer, std::string png, std::string dds)
{
	writer.WriteLine("\t@echo \"processing: " + png + " => " + dds + "\"");
	writer.WriteLine("\t@$(TOOL_DXTCOMPRESS) -file " + png + " -output " + dds + " -dxt1 -quality_highest -nomipmap -clampScale 512 512");
	if (gSettings.platform == "psp")
	{
		writer.WriteLine("\t@$(TOOL_PSP_FIXDDS) dds " + dds + " " + dds);
		writer.WriteLine("\t@$(CP_UPDATE) " + dds + " " + dds + ".tmp");
		writer.WriteLine("\t@$(TOOL_GZIP) -f -1 -c " +dds + ".tmp > " + dds);
	}
}

static void WriteDdsAlphaFromPng(StreamWriter& writer, std::string png, std::string dds)
{
	writer.WriteLine("\t@echo \"processing: " + png + " => " + dds + "\"");
	writer.WriteLine("\t@$(TOOL_DXTCOMPRESS) -file " + png + " -output " + dds + " -dxt3 -quality_highest -nomipmap -clampScale 512 512");
	if (gSettings.platform == "psp")
	{
		writer.WriteLine("\t@$(TOOL_PSP_FIXDDS) dds " + dds + " " + dds);
		writer.WriteLine("\t@$(CP_UPDATE) " + dds + " " + dds + ".tmp");
		writer.WriteLine("\t@$(TOOL_GZIP) -f -1 -c " + dds + ".tmp > " + dds);
	}
}

static void WriteFntFromPng(StreamWriter& writer, std::string png, std::string fnt, int contentScale)
{
	// range begin was 32
	writer.WriteLine("\t@echo \"processing: " + png + " => " + fnt + "\"");
	writer.WriteLine("\t@$(TOOL_FONTCOMPILE) -c " + png + " -o " + fnt + " -r 1 127 -s " + String::Format("%d", 800 / contentScale) + " 1024 -w -d " + String::Format("%d", contentScale) + " -p 10");
}

static void WriteCopy(StreamWriter& writer, std::string src, std::string dst)
{
	writer.WriteLine("\t@cp " + src + " " + dst);
}

static void WriteCopyBatch(StreamWriter& writer, std::vector<std::string> copyList, std::string dstDirectory)
{
	const int size = 1024 * 1024;
	
	StringBuilder<size> sb;
	
	for (size_t i = 0; i < copyList.size(); ++i)
	{
		sb.Append(copyList[i].c_str());
		sb.Append(' ');
	}
	
	sb.Append(dstDirectory.c_str());
	
	//writer.WriteLine("ifeq ($(OS), win)");
	//writer.WriteText("\t@cp ");
	//writer.WriteLine(sb.ToString());
	//writer.WriteLine("else");
	//writer.WriteLine("\t@$(CP_UPDATE) -R -P $(SRC_DIR) $(STG_DIR)");
	writer.WriteText("\t@rsync -u -t ");
	writer.WriteLine(sb.ToString());
	//writer.WriteLine("endif");
}

static void WriteAtlFromImgList(StreamWriter& writer, std::vector<std::string> inputList, std::string atl, int size)
{
	std::string inputText = String::Join(inputList, " ");

	writer.WriteLine(String::FormatC("\t@echo \"texture atlas: %s\"", atl.c_str()));
	writer.WriteLine("\t@$(TOOL_ATLCOMPILE) -o " + atl + " -c " + inputText + String::FormatC(" -b 2 -s %d -rp Resources_Staging/ -name Textures -o-header Classes/TexturesX.h -o-source Classes/TexturesX.cpp",
		size));
	//writer.WriteLine("\t@$(TOOL_ATLCOMPILE) -o " + atl + " -c " + inputText + " -b 2 -s " + String::Format("%d", size) + " -rp Resources_Staging/ -name Textures -o-header Classes/Textures" + String::Format("%d", size) + ".h -o-source Classes/Textures.cpp");
	
	if (gSettings.platform == "psp")
	{
		std::string tga = atl + ".tga";
		
		writer.WriteLine("\t@$(TOOL_PSP_FIXDDS) tga " +  tga + " " +  tga);
		writer.WriteLine("\t@$(TOOL_GZIP) -f -1 " + tga);
	}
}

static void WritePkgFromResourceList(StreamWriter& writer, std::vector<std::string> inputList, std::string pkg)
{
	std::string inputText = String::Join(inputList, " ");

	writer.WriteLine("\t@echo \"creating package file: " + pkg + "\"");
	writer.WriteLine("\t@$(TOOL_PKGCOMPILE) -c " + inputText + " -o " + pkg);
}

static void WriteTextureIndex(StreamWriter& writerHpp, StreamWriter& writerCpp, std::string hppFileName, std::vector<TextureIndexItem> itemList)
{
	std::sort(itemList.begin(), itemList.end());

	// write header

	writerHpp.WriteLine("#pragma once");
	writerHpp.WriteLine("// do not hand edit; machine generated file");
	writerHpp.WriteLine("namespace Textures");
	writerHpp.WriteLine("{");
	for (size_t i = 0; i < itemList.size(); ++i)
	{
		writerHpp.WriteLine(String::FormatC("\tconst static int %s = %d;", itemList[i].mName.c_str(), (int)i));
	}
	writerHpp.WriteLine(String::FormatC("\textern const int IndexToDataSet[%d]; // stores in which texture atlas the texture may be found", (int)itemList.size()));
	writerHpp.WriteLine(String::FormatC("\textern const int IndexToDataSetIndex[%d]; // stores the local index within the specified texture atlas", (int)itemList.size()));
	writerHpp.WriteLine("}");

	// write source

	writerCpp.WriteLine(String::FormatC("#include \"%s\"", Path::GetFileName(hppFileName).c_str()));
	writerCpp.WriteLine("namespace Textures");
	writerCpp.WriteLine("{");

	writerCpp.WriteLine(String::FormatC("\tconst int IndexToDataSet[%d] = {", (int)itemList.size()));
	for (size_t i = 0; i < itemList.size(); ++i)
	{
		writerCpp.WriteText(String::FormatC("\t\t%d", itemList[i].mDataSetId));

		if (i + 1 < itemList.size())
			writerCpp.WriteLine(",");
		else
			writerCpp.WriteLine();
	}
	writerCpp.WriteLine("\t};");

	std::map<int, int> indexMap;

	for (size_t i = 0; i < itemList.size(); ++i)
	{
		indexMap[itemList[i].mDataSetId] = 0;
	}

	writerCpp.WriteLine(String::FormatC("\tconst int IndexToDataSetIndex[%d] = {", (int)itemList.size()));
	for (size_t i = 0; i < itemList.size(); ++i)
	{
		writerCpp.WriteText(String::FormatC("\t\t%d", indexMap[itemList[i].mDataSetId]));

		if (i + 1 < itemList.size())
			writerCpp.WriteLine(",");
		else
			writerCpp.WriteLine();

		indexMap[itemList[i].mDataSetId]++;
	}
	writerCpp.WriteLine("\t};");

	writerCpp.WriteLine("}");
}

static void WriteImageScaleDown(StreamWriter& writer, std::string src, std::string dst, int scaleDown)
{
	writer.WriteLine(String::FormatC("\t@$(TOOL_IMGTOOL) -c %s -o %s -sd %d", src.c_str(), dst.c_str(), scaleDown));
}

static void WritePostProcess(StreamWriter& writer)
{
	if (gSettings.platform == "ios_hd")
	{
		// create icon and splash screen images for normal and retina display
		WriteImageScaleDown(writer, "$(DST_DIR)/Icon-iPad-HQ.png", "$(DST_DIR)/Icon-iPad.png", 4);
		WriteImageScaleDown(writer, "$(DST_DIR)/Icon-iPad-HQ.png", "$(DST_DIR)/Icon-iPad@2x.png", 2);
		WriteImageScaleDown(writer, "$(DST_DIR)/Default-Landscape-HQ.png", "$(DST_DIR)/Default-Landscape.png", 2);
		WriteImageScaleDown(writer, "$(DST_DIR)/Default-Landscape-HQ.png", "$(DST_DIR)/Default-Landscape@2x.png", 1);
	}
	
	if (gSettings.platform == "ios")
	{
		// todo: create icon and splash screen images for normal and retina display
	}
}

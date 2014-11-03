#include <IL/il.h>
#include "FileSysMgr.h"
#include "ResLoaderTex.h"
#include "ResTex.h"
#include "Types.h"

static void CheckError()
{
	int error = ilGetError();

	if (error)
	{
		printf("Error %d.\n", error);
		FASSERT(0);
	}
}

Res* ResLoaderTex::Load(const std::string& name)
{
	ilInit();
	CheckError();

	ilEnable(IL_ORIGIN_SET);  
	CheckError();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	CheckError();

	ILuint image;

	ilGenImages(1, &image);
	CheckError();

	ilBindImage(image);
	CheckError();

	ShFile file;
	
	if (!FileSysMgr::I().OpenFile(name, FILE_READ, file))
		FASSERT(0);

	std::string contents = file->Contents();
	ilLoadL(IL_TYPE_UNKNOWN, (void*)contents.c_str(), contents.size());
	//ilLoadImage((const ILstring)name.c_str());
	CheckError();

	int w = ilGetInteger(IL_IMAGE_WIDTH);
	int h = ilGetInteger(IL_IMAGE_HEIGHT);

	ResTex* tex = new ResTex();

	tex->SetSize(w, h);

	ilCopyPixels(0, 0, 0, w, h, 1, IL_RGBA, IL_UNSIGNED_BYTE, (void*)tex->GetData());
	CheckError();

	return tex;
}

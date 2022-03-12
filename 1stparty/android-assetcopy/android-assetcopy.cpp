#include "android-assetcopy.h"

#include <android/native_activity.h>

#include <errno.h> // errno, EEXIST
#include <linux/limits.h>
#include <stdio.h> // FILE
#include <sys/stat.h> // mkdir
#include <unistd.h> // chdir

namespace assetcopy
{
	std::vector<std::string> list_assets(JNIEnv * env, jobject context_object, const char * asset_path)
	{
		std::vector<std::string> result;

		auto getAssets_method = env->GetMethodID(env->GetObjectClass(context_object), "getAssets", "()Landroid/content/res/AssetManager;");
		auto assetManager_object = env->CallObjectMethod(context_object, getAssets_method);
		auto list_method = env->GetMethodID(env->GetObjectClass(assetManager_object), "list", "(Ljava/lang/String;)[Ljava/lang/String;");

		jstring path_object = env->NewStringUTF(asset_path);

		auto files_object = (jobjectArray)env->CallObjectMethod(assetManager_object, list_method, path_object);

		env->DeleteLocalRef(path_object);

		auto length = env->GetArrayLength(files_object);

		for (int i = 0; i < length; i++)
		{
			jstring jstr = (jstring)env->GetObjectArrayElement(files_object, i);

			const char * filename = env->GetStringUTFChars(jstr, nullptr);

			if (filename != nullptr)
			{
				result.push_back(filename);
				env->ReleaseStringUTFChars(jstr, filename);
			}

			env->DeleteLocalRef(jstr);
		}

		return result;
	}

	bool copy_asset_to_filesystem(AAssetManager * assetManager, const char * asset_name, const char * file_name)
	{
		bool result = true;

		AAsset * asset = AAssetManager_open(
				assetManager,
				asset_name,
				AASSET_MODE_STREAMING);

		if (asset == nullptr)
			result = false;
		else
		{
			FILE * f = fopen(file_name, "wb");

			if (f == nullptr)
				result = false;
			else
			{
				for (;;)
				{
					char buffer[BUFSIZ];
					const int readSize = AAsset_read(asset, buffer, BUFSIZ);

					if (readSize < 0)
					{
						result = false;
						break;
					}

					if (readSize == 0) // done?
						break;

					if (fwrite(buffer, readSize, 1, f) != 1)
					{
						result = false;
						break;
					}
				}

				fclose(f);
				f = nullptr;
			}

			AAsset_close(asset);
			asset = nullptr;
		}

		return result;
	}

	static bool push_dir(const char * name)
	{
		const int r = mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (r < 0 && errno != EEXIST)
			return false;
		if (chdir(name) < 0)
			return false;
		return true;
	}

	static bool pop_dir()
	{
		if (chdir("..") < 0)
			return false;
		return true;
	}

	bool recursively_copy_assets_to_filesystem(JNIEnv * env, jobject context_object, AAssetManager * assetManager, const char * asset_path)
	{
		auto filenames = list_assets(env, context_object, asset_path);

		for (auto & filename : filenames)
		{
			char full_path[PATH_MAX];

			if (asset_path[0] != 0)
				sprintf(full_path, "%s/%s", asset_path, filename.c_str());
			else
				strcpy(full_path, filename.c_str());

			auto * asset = AAssetManager_open(assetManager, full_path, AASSET_MODE_UNKNOWN);

			if (asset == nullptr)
			{
				// this is a directory. traverse

				if (!push_dir(filename.c_str()))
					return false;

				if (!recursively_copy_assets_to_filesystem(env, context_object, assetManager, full_path))
					return false;

				if (!pop_dir())
					return false;
			}
			else
			{
				AAsset_close(asset);
				asset = nullptr;

				// this is a file. copy it

				if (!copy_asset_to_filesystem(assetManager, full_path, filename.c_str()))
					return false;
			}
		}

		return true;
	}
}

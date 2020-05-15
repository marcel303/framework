#include "framework.h"

extern "C"
{
    #include <android_native_app_glue.h>
    #include <android/native_activity.h>

	#include <android/asset_manager.h>
	#include <android/asset_manager_jni.h>

	#include <sys/stat.h> // mkdir
	#include <unistd.h> // chdir

	static void extract_assets_traverse(android_app * app, AAssetDir * assetDir, const char * dirName)
	{
		if (dirName[0] != 0)
		{
			mkdir(dirName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		}

		for (;;)
		{

		break;
		}

		for (;;)
		{
			const char * filename = AAssetDir_getNextFileName(assetDir);

			if (filename == nullptr)
				break;

			AAsset * asset = AAssetManager_open(
				app->activity->assetManager,
				filename,
				AASSET_MODE_STREAMING); // todo : check for errors

			FILE * out = fopen(filename, "wb"); // todo : check for errors

			for (;;)
			{
				char buffer[BUFSIZ];
				const int readSize = AAsset_read(asset, buffer, BUFSIZ);
				if (readSize <= 0) // todo : check for errors
					break;
				fwrite(buffer, readSize, 1, out); // todo : check error code
			}

			fclose(out);
			out = nullptr;

			AAsset_close(asset);
		}
	}

	static void extract_assets_from_apk_to_cache_folder(android_app * app)
	{
		// from: https://en.wikibooks.org/wiki/OpenGL_Programming/Android_GLUT_Wrapper#Accessing_assets
		// modified to recursively process folders and to avoid some file ops when files are up to date

		const char * app_dir = app->activity->internalDataPath;

		// change current working directory to the base path
		logDebug("app_dir: %s", app_dir);
		chdir(app_dir);

		// extract assets

		AAssetManager * mgr = app->activity->assetManager;
		AAssetDir * assetDir = AAssetManager_openDir(mgr, "engine"); // todo : check for errors
		const char * filename = (const char*)NULL;
		while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
			AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_STREAMING); // todo : check for errors
			char buf[BUFSIZ];
			int nb_read = 0;
			FILE* out = fopen(filename, "wb");
			while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0) // todo : check for errors
				fwrite(buf, nb_read, 1, out); // todo : check error code
			fclose(out);
			AAsset_close(asset);
		}
		AAssetDir_close(assetDir);
	}

	static void test_asset_manager(android_app* app)
	{
		extract_assets_from_apk_to_cache_folder(app);

		auto * idp = app->activity->internalDataPath;
		auto * am = app->activity->assetManager;
		auto * d = AAssetManager_openDir(am, "engine");
		for (;;)
		{
			auto * fn = AAssetDir_getNextFileName(d);
			if (fn == nullptr)
				break;
			logDebug("dir: %s", fn);
		}
		AAssetDir_close(d);
	}

	static std::vector<std::string> list_assets(android_app * app, const char * path)
	{
		std::vector<std::string> result;

		JNIEnv * env = nullptr;
		app->activity->vm->AttachCurrentThread(&env, nullptr);

		auto context_object = app->activity->clazz;

		auto getAssets_method = env->GetMethodID(env->GetObjectClass(context_object), "getAssets", "()Landroid/content/res/AssetManager;");
		if (env->ExceptionOccurred())
			env->ExceptionDescribe();

		auto assetManager_object = env->CallObjectMethod(context_object, getAssets_method);
		if (env->ExceptionOccurred())
			env->ExceptionDescribe();

		auto list_method = env->GetMethodID(env->GetObjectClass(assetManager_object), "list", "(Ljava/lang/String;)[Ljava/lang/String;");
		if (env->ExceptionOccurred())
			env->ExceptionDescribe();

		jstring path_object = env->NewStringUTF(path);
		auto files_object = (jobjectArray)env->CallObjectMethod(assetManager_object, list_method, path_object);
		env->DeleteLocalRef(path_object);

		jsize length = env->GetArrayLength(files_object);
		for(jsize i = 0; i < length; i++)
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

		app->activity->vm->DetachCurrentThread();

		return result;
	}

	static void copy_asset_to_filesystem(android_app * app, const char * asset_name, const char * file_name)
	{
		AAsset * asset = AAssetManager_open(
				app->activity->assetManager,
				asset_name,
				AASSET_MODE_STREAMING); // todo : check for errors

		FILE * out = fopen(file_name, "wb"); // todo : check for errors

		for (;;)
		{
			char buffer[BUFSIZ];
			const int readSize = AAsset_read(asset, buffer, BUFSIZ);
			if (readSize <= 0) // todo : check for errors
				break;
			if (fwrite(buffer, readSize, 1, out) != 1) // todo : check error code
				break;
		}

		fclose(out);
		out = nullptr;

		AAsset_close(asset);
		asset = nullptr;
	}

	static void push_dir(const char * name)
	{
		mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // todo : check for errors
		chdir(name); // todo : check for errors
	}

	static void pop_dir()
	{
		chdir(".."); // todo : check for errors
	}

	static void test_asset_manager_list(android_app * app, const char * path)
	{
		auto filenames = list_assets(app, path);

		for (auto & filename : filenames)
		{
			//logDebug("file: %s", filename.c_str());

			char full_path[PATH_MAX];

			if (path[0])
				sprintf(full_path, "%s/%s", path, filename.c_str());
			else
				strcpy(full_path, filename.c_str());

			auto * asset = AAssetManager_open(app->activity->assetManager, full_path, AASSET_MODE_UNKNOWN);

			if (asset == nullptr)
			{
				// this is a directory; traverse

				push_dir(filename.c_str());
				{
					test_asset_manager_list(app, full_path);
				}
				pop_dir();
			}
			else
			{
				AAsset_close(asset);
				asset = nullptr;

				// this is a file. copy it

				copy_asset_to_filesystem(app, full_path, filename.c_str());
			}
		}
    }

    void android_main(android_app* app)
    {
        chdir(app->activity->internalDataPath); // todo : check for errors
	    test_asset_manager_list(app, "");
	    chdir(app->activity->internalDataPath); // todo : check for errors

	#if true
	    framework.init(0, 0);

	    Shader s("engine/Generic");

	    setColor(colorWhite);
	    drawRect(0, 0, 10, 10);
	#endif

	    test_asset_manager(app);
	}
}

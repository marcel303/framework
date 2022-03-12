#pragma once

#include <android/asset_manager.h>

#include <jni.h>

#include <string>
#include <vector>

struct android_app;

namespace assetcopy
{
	std::vector<std::string> list_assets(JNIEnv * env, jobject context_object, android_app * app, const char * asset_path);

	bool copy_asset_to_filesystem(AAssetManager * assetManager, const char * asset_name, const char * file_name);

	bool recursively_copy_assets_to_filesystem(JNIEnv * env, jobject context_object, AAssetManager * assetManager, const char * asset_path);
}

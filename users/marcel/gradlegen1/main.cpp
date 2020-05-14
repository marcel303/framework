#include "StringBuilder.h"
#include "StringEx.h"
#include <sys/stat.h>
#include <unistd.h>

/*

Gradle file auto-generator, for the garbage eco-system that is Android




*/

static FILE * f = nullptr;

struct S
{
	S & operator>>(const char * text)
	{
		fprintf(f, "%s", text);
		return *this;
	}
	
	S & operator<<(const char * text)
	{
		fprintf(f, "%s\n", text);
		return *this;
	}
};

static S s;

static void push_dir(const char * path)
{
// todo : error checks

	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	chdir(path);
}

static void pop_dir()
{
	chdir("..");
}

static const char * s_androidManifestTemplateForApp =
R"MANIFEST(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.chibi.generated.lib" android:versionCode="1" android:versionName="1.0" android:installLocation="auto">

	<!-- Tell the system this app requires OpenGL ES 3.1. -->
	<uses-feature android:glEsVersion="0x00030001" android:required="true"/>

	<!-- Tell the system this app works in either 3dof or 6dof mode -->
	<uses-feature android:name="android.hardware.vr.headtracking" android:required="false" />

	<!-- Network access needed for OVRMonitor -->
	<uses-permission android:name="android.permission.INTERNET" />

	<!-- Volume Control -->
	<uses-permission android:name="android.permission.MODIFY_AUDIO_SETTINGS" />
	<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />

	<application android:allowBackup="false" android:label="${appName}">
		<meta-data android:name="com.samsung.android.vr.application.mode" android:value="vr_only"/>
		<!-- launchMode is set to singleTask because there should never be multiple copies of the app running -->
		<!-- Theme.Black.NoTitleBar.Fullscreen gives solid black instead of a (bad stereoscopic) gradient on app transition -->
        <!-- If targeting API level 24+, configChanges should additionally include 'density'. -->
        <!-- If targeting API level 24+, android:resizeableActivity="false" should be added. -->
		<activity
				android:name="com.oculus.sdk.GLES3JNIActivity"
				android:theme="@android:style/Theme.Black.NoTitleBar.Fullscreen"
				android:launchMode="singleTask"
				android:screenOrientation="landscape"
				android:excludeFromRecents="false"
				android:configChanges="screenSize|screenLayout|orientation|keyboardHidden|keyboard|navigation|uiMode">
			
			<!-- Indicate the activity is aware of VrApi focus states required for system overlays  -->
			<meta-data android:name="com.oculus.vr.focusaware" android:value="true"/>
			
			<!-- This filter lets the apk show up as a launchable icon. -->
			<intent-filter>
				<action android:name="android.intent.action.MAIN" />
				<category android:name="android.intent.category.LAUNCHER" />
			</intent-filter>
		</activity>
	</application>
</manifest>)MANIFEST";

static const char * s_androidManifestTemplateForLib =
R"MANIFEST(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.chibi.generated.lib" android:versionCode="1" android:versionName="1.0" android:installLocation="auto">

	<!-- Tell the system this app requires OpenGL ES 3.1. -->
	<uses-feature android:glEsVersion="0x00030001" android:required="true"/>

	<!-- Tell the system this app works in either 3dof or 6dof mode -->
	<uses-feature android:name="android.hardware.vr.headtracking" android:required="false" />

	<!-- Network access needed for OVRMonitor -->
	<uses-permission android:name="android.permission.INTERNET" />

	<!-- Volume Control -->
	<uses-permission android:name="android.permission.MODIFY_AUDIO_SETTINGS" />
	<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
</manifest>)MANIFEST";

struct Library
{
	std::string name;
	bool is_app = false;
	bool is_shared = false;
	std::vector<std::string> files;
	std::vector<std::string> deps;
};

int main(int argc, char * argv[])
{
	chdir(CHIBI_RESOURCE_PATH);
	
	const char * module_path = CHIBI_RESOURCE_PATH;
	
	std::vector<Library> libs;
	
	{
		libs.resize(3);
		
		// add an app and two libraries, one shared and one static
		
		libs[0].name = "testapp1";
		libs[0].is_app = true;
		libs[0].is_shared = true;
		libs[0].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/framework.cpp");
		libs[0].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/opengl-ovr.cpp");
		libs[0].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/ovrFramebuffer.cpp");
		libs[0].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/VrCubeWorld_SurfaceView.cpp");
		libs[0].deps.push_back("testlib1");
		libs[0].deps.push_back("testlib2");
		
		libs[1].name = "testlib1";
		libs[1].is_app = false;
		libs[1].is_shared = true;
		libs[1].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/framework.cpp");
		libs[1].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/opengl-ovr.cpp");
		
		libs[2].name = "testlib2";
		libs[2].is_app = false;
		libs[2].is_shared = false;
		libs[2].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/framework.cpp");
		libs[2].files.push_back("/Users/thecat/repos/ovr-quest/cubes1/Src/opengl-ovr.cpp");
	}

	// generate root build.gradle file

	{
		f = fopen("build.gradle", "wt");
		
s << "buildscript {";
s << "  repositories {";
s << "    google()";
s << "    jcenter()";
s << "  }";
s << "  ";
s << "  dependencies {";
s << "    classpath 'com.android.tools.build:gradle:3.2.0'";
s << "  }";
s << "}";
s << "";
s << "allprojects {";
s << "    repositories {";
s << "        google()";
s << "      jcenter()";
s << "    }";
s << "}";

		fclose(f);
		f = nullptr;
	}
	
	// generate root settings.gradle file

	{
		f = fopen("settings.gradle", "wt");
		
s << "rootProject.name = 'Project'";
s << "";
for (auto & lib : libs)
	s >> "include '" >> lib.name.c_str() << ":Projects:Android'";
		
		fclose(f);
		f = nullptr;
	}
	
	for (auto & lib : libs)
	{
		push_dir(lib.name.c_str());
		
		const std::string appId = std::string("com.chibi.generated.") + lib.name;
	
		push_dir("Projects");
		push_dir("Android");
		
		// generate build.gradle file for each library

		{
			f = fopen("build.gradle", "wt");
			
if (lib.is_app)
	s << "apply plugin: 'com.android.application'";
else
	s << "apply plugin: 'com.android.library'";
s << "";
s << "dependencies {";
for (auto & dep : lib.deps)
	s >> "  implementation project(':" >> dep.c_str() << ":Projects:Android')";
s << "}";
s << "";
s << "android {";
s << "  // This is the name of the generated apk file, which will have";
s << "  // -debug.apk or -release.apk appended to it.";
s << "  // The filename doesn't effect the Android installation process.";
s << "  // Use only letters to remain compatible with the package name.";
s >> "  project.archivesBaseName = \"" >> lib.name.c_str() << "\"";
s << "  ";
s << "  defaultConfig {";
if (lib.is_app)
{
s << "    // Gradle replaces the manifest package with this value, which must";
s << "    // be unique on a system.  If you don't change it, a new app";
s << "    // will replace an older one.";
s << "    applicationId \"com.chibi.generated.\" + project.archivesBaseName";
}
s << "    minSdkVersion 23";
s << "    targetSdkVersion 25";
s << "    compileSdkVersion 26";
s << "    ";
s >> "    manifestPlaceholders = [appId:\"" >> appId.c_str() >> "\", appName:\"" >> lib.name.c_str() << "\"]";
s << "    ";
s << "    ";
s << "    // override app plugin abiFilters for 64-bit support";
s << "    externalNativeBuild {";
s << "        ndk {";
s << "          abiFilters 'arm64-v8a'";
s << "        }";
s << "        ndkBuild {";
s << "          abiFilters 'arm64-v8a'";
s << "        }";
s << "    }";
s << "  }";
s << "";
s << "  externalNativeBuild {";
s << "    ndkBuild {";
s << "      path 'jni/Android.mk'";
s << "    }";
s << "  }";
s << "";
s << "  sourceSets {";
s << "    main {";
s << "      manifest.srcFile 'AndroidManifest.xml'";
s << "      java.srcDirs = ['../../../java']"; // fixme : make unshared by copying files
s << "      jniLibs.srcDir 'libs'";
s << "      res.srcDirs = ['../../res']";
s << "      assets.srcDirs = ['../../assets']";
s << "    }";
s << "  }";
s << "";
s << "  lintOptions{";
s << "      disable 'ExpiredTargetSdkVersion'";
s << "  }";
s << "}";

			fclose(f);
			f = nullptr;
		}
		
		// generate AndroidManifest.xml for each library
		
		{
			f = fopen("AndroidManifest.xml", "wt");
			
if (lib.is_app)
	fprintf(f, "%s", s_androidManifestTemplateForApp);
else
	fprintf(f, "%s", s_androidManifestTemplateForLib);
			
			fclose(f);
			f = nullptr;
		}

		push_dir("jni");
		
		// generate Android.mk file for each library

		{
			f = fopen("Android.mk", "wt");
		
s << "# auto-generated. do not edit by hand";
s << "";
s << "LOCAL_PATH := $(call my-dir)";
s << "";
s << String::FormatC("# -- %s", lib.name.c_str()).c_str();
s << "";
s << "# note : CLEAR_VARS is a built-in NDK makefile";
s << "#        which attempts to clear as many LOCAL_XXX";
s << "#        variables as possible";
s << "include $(CLEAR_VARS)";
s << "";
s >> "LOCAL_MODULE           := " << lib.name.c_str();
s >> "LOCAL_SRC_FILES        :=";
for (auto & file : lib.files)
	s >> " " >> file.c_str();
s << "";
s << "LOCAL_LDLIBS           := -llog -landroid -lGLESv3 -lEGL";
s << "";
std::string static_libs = "LOCAL_STATIC_LIBRARIES :=";
std::string shared_libs = "LOCAL_SHARED_LIBRARIES :=";
for (auto & dep : lib.deps)
{
	for (auto & dep_lib : libs)
	{
		if (dep_lib.name == dep)
		{
			if (dep_lib.is_shared)
				shared_libs += " " + dep;
			else
				static_libs += " " + dep;
		}
	}
}
if (lib.is_app) shared_libs += " vrapi"; // todo : remove
s << static_libs.c_str();
s << shared_libs.c_str();
s << "";
if (lib.is_shared)
{
	s << "# note : BUILD_SHARED_LIBRARY is a built-in NDK makefile";
	s << "#        which will generate a shared library from LOCAL_SRC_FILES";
	s << "include $(BUILD_SHARED_LIBRARY)";
}
else
{
	
	s << "# note : BUILD_STATIC_LIBRARY is a built-in NDK makefile";
	s << "#        which will generate a static library from LOCAL_SRC_FILES";
	s << "include $(BUILD_STATIC_LIBRARY)";
}
s << "";
s << "$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)";
for (auto & dep : lib.deps)
	s << String::FormatC("$(call import-module,%s/Projects/Android/jni)", dep.c_str()).c_str();
s << "";

			fclose(f);
			f = nullptr;
		}
		
		// generate Application.mk file for each app

		if (lib.is_app || true)
		{
			f = fopen("Application.mk", "wt");
s << String::FormatC("NDK_MODULE_PATH := %s", module_path).c_str();
s << "APP_ABI := arm64-v8a";
s << "#APP_CPPFLAGS := ...";
s << "APP_DEBUG := true";

			fclose(f);
			f = nullptr;
		}

		pop_dir();
		
		pop_dir();
		pop_dir();
		
		pop_dir();
	}

	return 0;
}

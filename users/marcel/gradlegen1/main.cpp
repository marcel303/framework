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
	S & operator<<(const char * text)
	{
		fprintf(f, "%s\n", text);
		return *this;
	}
};

static S s;

static void mkdir(const char * path)
{
	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

static const char * s_androidManifestTemplate =
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

struct Library
{
	std::string name;
	bool shared = false;
	std::vector<std::string> deps;
};

int main(int argc, char * argv[])
{
	chdir(CHIBI_RESOURCE_PATH);
	
	const char * module_path = CHIBI_RESOURCE_PATH;
	
	std::vector<Library> libs;
	
	{
		libs.resize(3);
		
		libs[0].name = "testlib1";
		libs[0].shared = true;
		libs[0].deps.push_back("testlib3");
		
		libs[1].name = "testlib2";
		libs[2].shared = false;
		
		libs[2].name = "testlib3";
		libs[2].shared = true;
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
		{
			s << String::FormatC("include '%s:Projects:Android'", lib.name.c_str()).c_str();
		}
		
		fclose(f);
		f = nullptr;
	}
	
	for (auto & lib : libs)
	{
		mkdir(lib.name.c_str());
		chdir(lib.name.c_str());
		
		const std::string appId = std::string("com.chibi.generated.") + lib.name;
		
	#if true
		// generate a fake source file for each library
		{
			mkdir("Src");
			chdir("Src");
			
			f = fopen("file.cpp", "wt");
s << "#include <stdio.h>";
			fclose(f);
			f = nullptr;
			
			chdir("..");
		}
	#endif
	
		mkdir("Projects");
		chdir("Projects");
		
		mkdir("Android");
		chdir("Android");
		
		// generate build.gradle file for each library

		{
			f = fopen("build.gradle", "wt");
s << "apply plugin: 'com.android.application'";
s << "";
s << "dependencies {";
s << "}";
s << "";
s << "android {";
s << "  // This is the name of the generated apk file, which will have";
s << "  // -debug.apk or -release.apk appended to it.";
s << "  // The filename doesn't effect the Android installation process.";
s << "  // Use only letters to remain compatible with the package name.";
s << String::FormatC("  project.archivesBaseName = \"%s\"", lib.name.c_str()).c_str();
s << "";
s << "  defaultConfig {";
s << "    // Gradle replaces the manifest package with this value, which must";
s << "    // be unique on a system.  If you don't change it, a new app";
s << "    // will replace an older one.";
s << "    applicationId \"com.chibi.generated.\" + project.archivesBaseName";
s << "    minSdkVersion 23";
s << "    targetSdkVersion 25";
s << "    compileSdkVersion 26";
s << "";
s << String::FormatC("    manifestPlaceholders = [appId:\"%s\", appName:\"%s\"]", appId.c_str(), lib.name.c_str()).c_str();
s << "";
s << "";
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
			
			fprintf(f, "%s", s_androidManifestTemplate);
			
			fclose(f);
			f = nullptr;
		}

		mkdir("jni");
		chdir("jni");
		
		// generate Android.mk file for each library

		{
			f = fopen("Android.mk", "wt");
s << "LOCAL_PATH := $(call my-dir)";
s << "";
s << "#---";
s << String::FormatC("#--- %s", lib.name.c_str()).c_str();
s << "#---";
s << "";
s << "# CLEAR_VARS is a built-in NDK makefile which attempts to clear as many LOCAL_ variables as possible";
s << "include $(CLEAR_VARS)";
s << "";
s << String::FormatC("LOCAL_MODULE           := %s", lib.name.c_str()).c_str();
//s <<                 "LOCAL_SRC_FILES        := ../../../Src/file.cpp";
s << "LOCAL_SRC_FILES := /Users/thecat/repos/ovr-quest/cubes1/Src/framework.cpp /Users/thecat/repos/ovr-quest/cubes1/Src/opengl-ovr.cpp /Users/thecat/repos/ovr-quest/cubes1/Src/ovrFramebuffer.cpp /Users/thecat/repos/ovr-quest/cubes1/Src/VrCubeWorld_SurfaceView.cpp"; // todo : files from library
s <<                 "LOCAL_LDLIBS           := -llog -landroid";
s <<                 "";
s <<                 "LOCAL_SHARED_LIBRARIES := vrapi"; // todo : remove
s << "";
if (lib.shared)
{
	s << "# BUILD_SHARED_LIBRARY is a built-in NDK makefile which will generate a shared library from LOCAL_SRC_FILES";
	s << "include $(BUILD_SHARED_LIBRARY)";
}
else
{
	
	s << "# BUILD_STATIC_LIBRARY is a built-in NDK makefile which will generate a static library from LOCAL_SRC_FILES";
	s << "include $(BUILD_STATIC_LIBRARY)";
}
s << "";
s << "$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)";
s << "";
			fclose(f);
			f = nullptr;
		}
		
		// generate Application.mk file for each app
	// todo : make distinction between apps and libraries. for now, pretend we are building apps only

		{
			f = fopen("Application.mk", "wt");
s << String::FormatC("NDK_MODULE_PATH := %s", module_path).c_str();
s << "#APP_ABI := arm64-v8a";
s << "#sAPP_CPPFLAGS := ...";
s << "APP_DEBUG := true";
			fclose(f);
			f = nullptr;
		}

		chdir("..");
		
		chdir("..");
		chdir("..");
		chdir("..");
	}

	return 0;
}

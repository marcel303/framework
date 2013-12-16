//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include "OFSettings.h"
#include "OFXmlReader.h"
#include "OpenFeint+AddOns.h"

OFDefineSingleton(OFSettings);

OFSettings::OFSettings()
{
	// These are default settings intended for distribution
    mSettingsDict = [[NSMutableDictionary dictionaryWithCapacity:20] retain];
    
    setDefault(@"server-url", @"https://api.openfeint.com/"); 
    setDefault(@"presence-host", @"presence.openfeint.com");
    setDefault(@"facebook-application-key", @"1a2dcd0bdc7ce8056aeb1dac00c2a886");
    setDefault(@"facebook-callback-url", @"https://api.openfeint.com/");


    [OpenFeint setDefaultAddOnSettings:this];

    
    //then call the addOn things....    
	discoverLocalConfiguration();
	
#if !defined(_DISTRIBUTION)
	loadSettingsFile();
#endif
}

void OFSettings::setDefault(NSString*tag, NSString*value) {
    [mSettingsDict setObject:value forKey:tag];
    
}

void OFSettings::loadSetting(OFXmlReader &reader, const char*xmlTag) {
    OFRetainedPtr<NSString>serializedValue;
	reader.io(xmlTag, serializedValue);
    if(serializedValue) [mSettingsDict setObject:serializedValue forKey:[NSString stringWithCString:xmlTag encoding:NSUTF8StringEncoding]];
    
}

void OFSettings::loadSettingsFile()
{
#if !defined(_DISTRIBUTION)
	OFXmlReader OFSettingsReader("openfeint_internal_settings");
	NSMutableString* environmentScope = [NSMutableString stringWithString:@"environment-"];
	
#ifdef _DEBUG
	[environmentScope appendString:@"debug"];
#else
	[environmentScope appendString:@"release"];
#endif

	OFISerializer::Scope sRoot(&OFSettingsReader, "config");	
	OFISerializer::Scope sEnvironment(&OFSettingsReader, [environmentScope cStringUsingEncoding:NSUTF8StringEncoding]);
    
    loadSetting(OFSettingsReader, "server-url");
    loadSetting(OFSettingsReader, "presence-host");
    loadSetting(OFSettingsReader, "facebook-callback-url");
    loadSetting(OFSettingsReader, "facebook-application-key");
    loadSetting(OFSettingsReader, "debug-override-key");
    loadSetting(OFSettingsReader, "debug-override-secret");    
    [OpenFeint loadAddOnSettings:this fromReader:OFSettingsReader];
#endif
}

void OFSettings::discoverLocalConfiguration()
{
	NSDictionary* infoDict = [[NSBundle mainBundle] infoDictionary];
	NSString* bundleIdentifier = [infoDict valueForKey:@"CFBundleIdentifier"];
	NSString* bundleVersion = [infoDict valueForKey:@"CFBundleVersion"];
	
	mClientBundleIdentifier		= bundleIdentifier;
	mClientBundleVersion		= bundleVersion;
	mClientLocale				= [[NSLocale currentLocale] localeIdentifier];
	mClientDeviceType			= [UIDevice currentDevice].model;
	mClientDeviceSystemName		= [UIDevice currentDevice].systemName;
	mClientDeviceSystemVersion	= [UIDevice currentDevice].systemVersion;	
}

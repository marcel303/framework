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

#include "OFBase.h"
#include "OFRetainedPtr.h"

#pragma once
class OFXmlReader;
class OFSettings 
{
OFDeclareSingleton(OFSettings);
public:
    //these are old accessor functions left here for backwards compatibility
    //going forward, you should use getSetting
	NSString* getServerUrl() const						{ return getSetting(@"server-url"); }
	NSString* getMultiplayerServerUrl() const			{ return getSetting(@"multiplayer-server-url"); }
	NSString* getPresenceHost() const					{ return getSetting(@"presence-host"); }
	NSString* getFacebookApplicationKey() const			{ return getSetting(@"facebook-application-key"); }
	NSString* getFacebookCallbackServerUrl() const		{ return getSetting(@"facebook-callback-url"); }
	NSString* getDebugOverrideKey() const               { return getSetting(@"debug-override-key"); }
    NSString* getDebugOverrideSecret() const            { return getSetting(@"debug-override-secret"); }
    
    
	NSString* getClientBundleIdentifier() const			{ return mClientBundleIdentifier; }
	NSString* getClientBundleVersion() const			{ return mClientBundleVersion; }
	NSString* getClientLocale() const					{ return mClientLocale; }
	NSString* getClientDeviceType() const				{ return mClientDeviceType; }
	NSString* getClientDeviceSystemName() const			{ return mClientDeviceSystemName; }
	NSString* getClientDeviceSystemVersion() const		{ return mClientDeviceSystemVersion; }
    
    //public so that AddOns can use this as well
    void loadSetting(OFXmlReader &reader, const char*xmlTag);
    void setDefault(NSString*tag, NSString*value);
    NSString*getSetting(const NSString*key) const       { return [mSettingsDict objectForKey:key]; }
	
private:
    NSMutableDictionary* mSettingsDict;
    
	void discoverLocalConfiguration();
	void loadSettingsFile();
    
	OFRetainedPtr<NSString> mClientBundleIdentifier;
	OFRetainedPtr<NSString> mClientBundleVersion;
	OFRetainedPtr<NSString> mClientLocale;
	OFRetainedPtr<NSString> mClientDeviceType;
	OFRetainedPtr<NSString> mClientDeviceSystemName;
	OFRetainedPtr<NSString> mClientDeviceSystemVersion;	
};

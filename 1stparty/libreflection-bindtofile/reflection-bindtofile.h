#pragma once

#include "reflection.h" // bindObjectToFile<T>

/*

usage : bind an object to file and reload the object on file change

	TypeDB typeDB;
	reflect(typeDB); // add type definition for SomeStruct to the type DB
	
	SomeStruct object;
	bindObjectToFile(&typeDB, &object, "object.json"); // this will load the object from file and monitor file changes to reload the object
	
	for (;;)
	{
		// framework detects changed files and puts them in framework.changedFiles
		
		framework.process();
		
		// tickObjectToFileBinding checks framework.changedFiles and reloads object on change
		
		tickObjectToFileBinding();
		
		// it's also possible to flush changes you made to file
		
		if (keyboard.wentDown(SDLK_f))
		{
			object.name = "changed name";
			flushObjectToFile(&object);
		}
	}

usage : detect object changes through versioning

	struct SomeStruct
	{
		std::string name;
		int version = 0;
		int previousVersion = 0; // note : this member is here to track version changes only
	};
	
	TypeDB typeDB;
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<std::string>("string", kDataType_String);
	typeDB.addStructured<SomeStruct>("SomeStruct")
		.add("name", &SomeStruct::name)
		.add("version", &SomeStruct::version)
			.addFlag<ObjectVersionFlag>();
	
	SomeStruct object;
	bindObjectToFile(&typeDB, &object, "object.json"); // this will load the object from file and monitor file changes to reload the object
	
	for (;;)
	{
		// framework detects changed files and puts them in framework.changedFiles
		
		framework.process();
		
		// tickObjectToFileBinding checks framework.changedFiles and reloads object on change
		// it will also increment the version number, as we added the ObjectVersionFlag
		
		tickObjectToFileBinding();
		
		// check if the version number changed
		
		if (object.version != object.previousVersion)
		{
			object.previousVersion = object.version;
			
			logInfo("the object has changed!");
		}
	}
	
usage : load an object from file

	TypeDB typeDB;
	reflect(typeDB); // add type definition for SomeStruct to the type DB
	
	SomeStruct object;
	loadObjectFromFile(typeDB, object, "object.json");
	
usage : save an object to file

	TypeDB typeDB;
	reflect(typeDB); // add type definition for SomeStruct to the type DB
	
	SomeStruct object;
	object.name = "some object";
	object.value = 20;
	saveObjectToFile(typeDB, object, "object.json"))

*/

/**
 * Add to a member type to flag an integer type field as a version number. The version number automatically gets
 * incremented when the object is (re)loaded.
 */
struct ObjectVersionFlag : MemberFlag<ObjectVersionFlag>
{
};

/**
 * Binds an object to file. If the file doesn't exist yet, the contents of the object will be written to disk. Otherwise, the object will be loaded from file.
 * @param typeDB The type database which contains the types which may be needed to (de)serialize nested types.
 * @param type The type of the object.
 * @param object The object to bind to file.
 * @param filename The file to which to bind the object. The extension may end with .json or .txt, and the (de)serialization format will be chosen appropriately.
 * @return True if binding the object to file is succesful. False otherwise.
 */
bool bindObjectToFile(const TypeDB * typeDB, const Type * type, void * object, const char * filename);

template <typename T>
inline bool bindObjectToFile(const TypeDB * typeDB, T * object, const char * filename)
{
	auto * type = typeDB->findType<T>();
	return bindObjectToFile(typeDB, type, object, filename);
}

/**
 * Checks if the file for any of the object to file bindings has changed, and reloads objects if necessary.
 */
void tickObjectToFileBinding();

/**
 * Save the contents of the given object to disk, with the object to file binding registered before.
 * @param object The object to save to disk.
 * @return True if succesful. False otherwise.
 */
bool flushObjectToFile(const void * object);

// --- helper functions ---

bool saveObjectToFile(const TypeDB & typeDB, const Type * type, const void * object, const char * filename);
bool loadObjectFromFile(const TypeDB & typeDB, const Type * type, void * object, const char * filename);

template <typename T>
bool saveObjectToFile(const TypeDB & typeDB, const T & object, const char * filename)
{
	auto * type = typeDB.findType<T>();
	return saveObjectToFile(typeDB, type, &object, filename);
}

template <typename T>
bool loadObjectFromFile(const TypeDB & typeDB, T & object, const char * filename)
{
	auto * type = typeDB.findType<T>();
	return loadObjectFromFile(typeDB, type, &object, filename);
}

int * findObjectVersionMember(const TypeDB & typeDB, const Type * type, void * object);

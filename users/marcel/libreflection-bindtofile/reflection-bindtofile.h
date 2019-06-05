#pragma once

struct Type;
struct TypeDB;

//

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

bool saveObjectToFile(const TypeDB * typeDB, const Type * type, const void * object, const char * filename);
bool loadObjectFromFile(const TypeDB * typeDB, const Type * type, void * object, const char * filename);

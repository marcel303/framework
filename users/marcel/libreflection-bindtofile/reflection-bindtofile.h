#pragma once

struct Type;
struct TypeDB;

bool bindObjectToFile(const TypeDB * typeDB, const Type * type, void * object, const char * filename);

template <typename T>
inline bool bindObjectToFile(const TypeDB * typeDB, T * object, const char * filename)
{
	auto * type = typeDB->findType<T>();
	return bindObjectToFile(typeDB, type, object, filename);
}

void tickObjectToFileBinding();

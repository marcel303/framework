#pragma once

#include <string>
#include "BitStream.h"
#include "SharedPtr.h"

enum REPLICATION_MODE
{
	REP_ONCE, // Replicate values only @ create.
	REP_CONTINUOUS, // Keep sending updates.
	REP_ONCHANGE // Replication changes only.
};

enum PARAMETER_TYPE
{
	PARAM_INT8,
	PARAM_INT16,
	PARAM_INT32,
	PARAM_FLOAT32,
	PARAM_SPEEDF,
	PARAM_ROTF,
	PARAM_QUATF,
	PARAM_TWEENF,
	PARAM_STRING
};

enum REPLICATION_COMPRESSION
{
	COMPRESS_NONE,
	COMPRESS_U8,
	COMPRESS_U16,
	COMPRESS_8_8 // 8.8 fixed point.
};

class Parameter
{
public:
	Parameter();
	Parameter(PARAMETER_TYPE type, const std::string& name, REPLICATION_MODE rep, REPLICATION_COMPRESSION compression, void* data);
	~Parameter();

	// PARAM_ROTATIONF MyInt REP_ONCE COMPRESS_U8.

	void Initialize(PARAMETER_TYPE type, const std::string& name, REPLICATION_MODE rep, REPLICATION_COMPRESSION compression, void* data);
	int GetParameterSize() const;
	int GetRepParameterSize() const;
	bool RepSerialize(BitStream& bitStream) const;
	bool RepDeSerialize(BitStream& bitStream);
	void Invalidate();

	REPLICATION_MODE m_rep;
	PARAMETER_TYPE m_type;
	std::string m_name;
	REPLICATION_COMPRESSION m_compression;
	void* m_data;
	int m_version;
};

typedef SharedPtr<Parameter> ShParameter;

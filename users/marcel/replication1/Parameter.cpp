#include "Convert.h"
#include "Debug.h"
#include "Parameter.h"

Parameter::Parameter()
{
}

Parameter::Parameter(PARAMETER_TYPE type, const std::string& name, REPLICATION_MODE rep, REPLICATION_COMPRESSION compression, void* data)
{
	Initialize(type, name, rep, compression, data);
}

Parameter::~Parameter()
{
}

void Parameter::Initialize(PARAMETER_TYPE type, const std::string& name, REPLICATION_MODE rep, REPLICATION_COMPRESSION compression, void* data)
{
	FASSERT(data);

	m_type = type;
	m_name = name;
	m_rep = rep;
	m_compression = compression;
	m_data = data;
	m_version = 0;
}

int Parameter::GetParameterSize() const
{
	switch (m_type)
	{
	case PARAM_INT8:
		return 1;
	case PARAM_INT16:
		return 2;
	case PARAM_INT32:
	case PARAM_FLOAT32:
	case PARAM_SPEEDF:
	case PARAM_ROTF:
	case PARAM_TWEENF:
		return 4;
	case PARAM_QUATF:
		return 16;
	case PARAM_STRING:
		FASSERT(0);
		return 2 + (int)(*(std::string*)m_data).length();
	default:
		FASSERT(0);
		return 0;
	}
}

int Parameter::GetRepParameterSize() const
{
	switch (m_compression)
	{
	case COMPRESS_NONE:
		{
			switch (m_type)
			{
			case PARAM_INT8:
				return 1;
				break;
			case PARAM_INT16:
				return 2;
				break;
			case PARAM_INT32:
			case PARAM_FLOAT32:
			case PARAM_SPEEDF:
			case PARAM_ROTF:
			case PARAM_TWEENF:
				return 4;
				break;
			case PARAM_QUATF:
				return 16;
				break;
			case PARAM_STRING:
				FASSERT(0);
				return 2 + (int)(*(std::string*)m_data).length();
			default:
				FASSERT(0);
				return 0;
			}
		}
		break;
	case COMPRESS_U8:
		return 1;
		break;
	case COMPRESS_U16:
	case COMPRESS_8_8:
		return 2;
		break;
	default:
		FASSERT(0);
		return 0;
	}

	return 0;
}

bool Parameter::RepSerialize(BitStream& bitStream) const
{
	// TODO: Handle compression.

	switch (m_type)
	{
		case PARAM_INT8:
		{
			uint8_t & value = *reinterpret_cast<uint8_t*>(m_data);
			bitStream.Write(value);
		}
		break;
		case PARAM_INT16:
		{
			uint16_t & value = *reinterpret_cast<uint16_t*>(m_data);
			bitStream.Write(value);
		}
		break;
		case PARAM_INT32:
		case PARAM_FLOAT32:
		{
			uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
			bitStream.Write(value);
		}
		break;
		case PARAM_SPEEDF:
		{
			switch (m_compression)
			{
				case COMPRESS_NONE:
				{
					uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
					bitStream.Write(value);
				}
				break;
				case COMPRESS_8_8:
				{
					int16_t value = Convert::ToRealInt16(*(float*)m_data);
					bitStream.Write(value);
				}
				break;
				default:
					FASSERT(0);
					return false;
					break;
			}
		}
		break;
		case PARAM_ROTF:
		{
			switch (m_compression)
			{
				case COMPRESS_NONE:
				{
					uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
					bitStream.Write(value);
				}
				break;
				case COMPRESS_U8:
				{
					uint8_t value = Convert::ToRotUInt8(*(float*)m_data);
					bitStream.Write(value);
				}
				break;
				case COMPRESS_U16:
				{
					uint16_t value = Convert::ToRotUInt16(*(float*)m_data);
					bitStream.Write(value);
				}
				break;
				default:
					FASSERT(0);
					return false;
					break;
			}
		}
		break;
		case PARAM_TWEENF:
		{
			switch (m_compression)
			{
				case COMPRESS_NONE:
				{
					uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
					bitStream.Write(value);
				}
				break;
				case COMPRESS_U8:
				{
					uint8_t value = Convert::ToTweenUInt8(*(float*)m_data);
					bitStream.Write(value);
				}
				break;
				case COMPRESS_U16:
				{
					uint16_t value = Convert::ToTweenUInt16(*(float*)m_data);
					bitStream.Write(value);
				}
				break;
				default:
					FASSERT(0);
					return false;
			}
		}
		break;
		case PARAM_QUATF:
		{
			float* temp = (float*)m_data;
			for (int i = 0; i < 4; ++i)
				bitStream.Write(temp[i]);
		}
		break;
		case PARAM_STRING:
		{
			std::string& v = *(std::string*)m_data;
			bitStream.WriteString(v);
		}
		break;
		default:
		{
			DB_ERR("Unknown parameter type.\n");
			FASSERT(0);
			return false;
		}
		break;
	}

	return true;
}

bool Parameter::RepDeSerialize(BitStream& bitStream)
{
	// TODO: Handle compression.

	switch (m_type)
	{
		case PARAM_INT8:
		{
			uint8_t & value = *reinterpret_cast<uint8_t*>(m_data);
			bitStream.Read(value);
		}
		break;
		case PARAM_INT16:
		{
			uint16_t & value = *reinterpret_cast<uint16_t*>(m_data);
			bitStream.Read(value);
		}
		break;
		case PARAM_INT32:
		case PARAM_FLOAT32:
		{
			uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
			bitStream.Read(value);
		}
		break;
		case PARAM_SPEEDF:
		{
			switch (m_compression)
			{
				case COMPRESS_NONE:
				{
					uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
					bitStream.Read(value);
				}
				break;
				case COMPRESS_8_8:
				{
					int16_t v;
					bitStream.Read(v);
					*(float*)m_data = Convert::ToRealFloat(v);
				}
				break;
				default:
					FASSERT(0);
					return false;
					break;
			}
		}
		break;
		case PARAM_ROTF:
		{
			switch (m_compression)
			{
				case COMPRESS_NONE:
				{
					uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
					bitStream.Read(value);
				}
				break;
				case COMPRESS_U8:
				{
					uint8_t v;
					bitStream.Read(v);
					*(float*)m_data = Convert::ToRotFloat(v);
				}
				break;
				case COMPRESS_U16:
				{
					uint16_t v;
					bitStream.Read(v);
					*(float*)m_data = Convert::ToRotFloat(v);
				}
				break;
				default:
					FASSERT(0);
					return false;
					break;
			}
		}
		break;
		case PARAM_TWEENF:
		{
			switch (m_compression)
			{
				case COMPRESS_NONE:
				{
					uint32_t & value = *reinterpret_cast<uint32_t*>(m_data);
					bitStream.Read(value);
				}
				break;
				case COMPRESS_U8:
				{
					uint8_t v;
					bitStream.Read(v);
					*(float*)m_data = Convert::ToTweenFloat(v);
				}
				break;
				case COMPRESS_U16:
				{
					uint16_t v;
					bitStream.Read(v);
					*(float*)m_data = Convert::ToTweenFloat(v);
				}
				break;
				default:
					FASSERT(0);
					return false;
					break;
			}
		}
		break;
		case PARAM_QUATF:
		{
			float* temp = (float*)m_data;
			for (int i = 0; i < 4; ++i)
				bitStream.Read(temp[i]);
		}
		break;
		case PARAM_STRING:
		{
			std::string& v = *(std::string*)m_data;
			v = bitStream.ReadString();
		}
		break;
		default:
		{
			DB_ERR("Unknown parameter type.\n");
			FASSERT(0);
			return false;
		}
		break;
	}

	return true;
}

void Parameter::Invalidate()
{
	m_version++;
}

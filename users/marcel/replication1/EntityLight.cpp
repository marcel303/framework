#include "EntityLight.h"

EntityLight::EntityLight(LIGHT_TYPE type, Vec3 position, Vec3 direction) : Entity()
{
	SetClassName("Light");
	AddParameter(Parameter(PARAM_FLOAT32, "px", REP_ONCE, COMPRESS_NONE, &m_position[0]));
	AddParameter(Parameter(PARAM_FLOAT32, "py", REP_ONCE, COMPRESS_NONE, &m_position[1]));
	AddParameter(Parameter(PARAM_FLOAT32, "pz", REP_ONCE, COMPRESS_NONE, &m_position[2]));
	AddParameter(Parameter(PARAM_FLOAT32, "dx", REP_ONCE, COMPRESS_NONE, &m_direction[0]));
	AddParameter(Parameter(PARAM_FLOAT32, "dy", REP_ONCE, COMPRESS_NONE, &m_direction[1]));
	AddParameter(Parameter(PARAM_FLOAT32, "dz", REP_ONCE, COMPRESS_NONE, &m_direction[2]));
	AddParameter(Parameter(PARAM_INT32, "type", REP_ONCE, COMPRESS_NONE, &m_type));
	AddParameter(Parameter(PARAM_FLOAT32, "type", REP_ONCE, COMPRESS_NONE, &m_time));

	m_type = type;
	m_position = position;
	m_direction = direction;

	m_time = 0.0f;
}

void EntityLight::UpdateAnimation(float dt)
{
	m_position[0] = sin((m_time + m_scene->GetTime()) / 10.0f) * 100.0f;
	//m_position[1] = 20.0f;
	m_position[1] = 15.0f + sin((m_time + m_scene->GetTime()) / 10.0f) * 10.0f;
	m_position[2] = cos((m_time + m_scene->GetTime()) / 10.0f) * 100.0f;

	m_direction[0] = sin((m_time + m_scene->GetTime()) / 5.8f);
	m_direction[1] = 0.0f;
	m_direction[2] = cos((m_time + m_scene->GetTime()) / 5.8f);
}

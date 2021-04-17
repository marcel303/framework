#include "AngleAxis.h"

#include "componentType.h"
#include "reflection.h"

void AngleAxis::reflect(TypeDB & typeDB)
{
	typeDB.addStructured<AngleAxis>("AngleAxis")
		.add("angle", &AngleAxis::angle)
			.addFlag(new ComponentMemberFlag_EditorType_AngleDegrees)
		.add("axis", &AngleAxis::axis)
			.addFlag(new ComponentMemberFlag_EditorType_OrientationVector);
}

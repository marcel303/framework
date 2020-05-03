#include "componentType.h"

ComponentMemberAdder_Int::ComponentMemberAdder_Int(Member * in_member)
	: member(in_member)
{
}

ComponentMemberAdder_Int & ComponentMemberAdder_Int::limits(const int min, const int max)
{
	ComponentMemberFlag_IntLimits * limits = new ComponentMemberFlag_IntLimits();
	limits->min = min;
	limits->max = max;

	member->addFlag(limits);
	
	return *this;
}

//

ComponentMemberAdder_Float::ComponentMemberAdder_Float(Member * in_member)
	: member(in_member)
{
}

ComponentMemberAdder_Float & ComponentMemberAdder_Float::limits(const float min, const float max)
{
	ComponentMemberFlag_FloatLimits * limits = new ComponentMemberFlag_FloatLimits();
	limits->min = min;
	limits->max = max;

	member->addFlag(limits);
	
	return *this;
}

ComponentMemberAdder_Float & ComponentMemberAdder_Float::editingCurveExponential(const float value)
{
	ComponentMemberFlag_FloatEditorCurveExponential * curveExponential = new ComponentMemberFlag_FloatEditorCurveExponential();
	curveExponential->exponential = value;

	member->addFlag(curveExponential);
	
	return *this;
}

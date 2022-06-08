/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "Constraint.h"
#include "Contact.h"
#include "RigidBody.h"

namespace XenoCollide
{
	extern int32 gTimeStamp;

	Contact::Contact(RigidBody* b1, RigidBody* b2, const Vector& p1, const Vector& p2, const Vector& n)
	{
		this->b1 = b1;
		this->b2 = b2;

		normal = n;

		local1 = (~(b1->body->q)).Rotate(p1 - b1->body->x);
		local2 = (~(b2->body->q)).Rotate(p2 - b2->body->x);

		constraint = new ContactConstraint(b1->body, b2->body, local1, local2, n);
		point = 0.5f * (p1 + p2);
		timeStamp = gTimeStamp;
	}

	Contact::~Contact()
	{
		delete constraint;
		constraint = NULL;
	}

	void Contact::Update(const Vector& p1, const Vector& p2, const Vector& n)
	{
		normal = n;

		local1 = (~(b1->body->q)).Rotate(p1 - b1->body->x);
		local2 = (~(b2->body->q)).Rotate(p2 - b2->body->x);

		constraint->Update(local1, local2, n);
		timeStamp = gTimeStamp;
	}

	float32 Contact::Distance()
	{
		Vector p1 = b1->body->q.Rotate(local1) + b1->body->x;
		Vector p2 = b2->body->q.Rotate(local2) + b2->body->x;
		return (p1 - p2).Len3();
	}
}

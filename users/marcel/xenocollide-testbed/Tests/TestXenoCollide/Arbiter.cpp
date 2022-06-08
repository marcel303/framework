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

#include "Arbiter.h"
#include "Contact.h"

using namespace std;

//////////////////////////////////////////////////////////////

namespace XenoCollide
{
	Arbiter::Arbiter(RigidBody* in1, RigidBody* in2)
		: b1(in1)
		, b2(in2)
	{
	}

	Arbiter::~Arbiter()
	{
		ContactList::iterator it = contacts.begin();
		while (it != contacts.end())
		{
			delete* it;
			it++;
		}
		contacts.clear();
	}

	void Arbiter::AddContact(const Vector& p1, const Vector& p2, const Vector& normal)
	{
		Vector point = 0.5f * (p1 + p2);

		Contact* contact = NULL;
		ContactList::iterator it = contacts.begin();
		ContactList::iterator endIt = contacts.end();
		while (it != endIt)
		{
			if (((*it)->point - point).Len3Squared() < 1.0f)
			{
				contact = *it;
				break;
			}
			it++;
		}

		if (contact == NULL)
		{
			contact = new Contact(b1, b2, p1, p2, normal);
			contacts.push_back(contact);
		}
		else
		{
			contact->point = point;
			contact->Update(p1, p2, normal);
		}
	}
}

//////////////////////////////////////////////////////////////

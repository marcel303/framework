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

#pragma once

#include <list>
#include <map>

#include "Contact.h"

class RigidBody;

//////////////////////////////////////////////////////////////////////////////
// Each Arbiter maintains a list of contacts between two RigidBody objects.
//
// As new points of contact between the pair are discovered, they are added
// to the Arbiter with a call to AddContact().
//
class Arbiter
{

public:

	Arbiter(RigidBody* in1, RigidBody* in2);
	~Arbiter();

	void AddContact(const Vector& p1, const Vector& p2, const Vector& normal);

public:

	RigidBody*	b1;
	RigidBody*	b2;
	ContactList	contacts;

};

//////////////////////////////////////////////////////////////////////////////
// For easy access, Arbiters are stored in a map.  To find the Arbiter for
// two RigidBodies, build an ArbiterKey for the two bodies and use it
// as the lookup key for the ArbiterMap.
//
class ArbiterKey
{

public:

	ArbiterKey(RigidBody* b1, RigidBody* b2)
	{
		if ( b1 < b2 )
		{
			this->b1 = b1;
			this->b2 = b2;
		}
		else
		{
			this->b1 = b2;
			this->b2 = b1;
		}
	}

	bool operator < (const ArbiterKey& other) const
	{
		return (b2 < other.b2) || ((b2 == other.b2) && (b1 < other.b1));
	}

private:

	RigidBody*	b1;
	RigidBody*	b2;

};

typedef std::map<ArbiterKey, Arbiter*>::iterator ArbiterIt;

/*
 *  Entity.mm
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Entity.h"

#include "Rendering.h"

Entity::Entity()
{
	SetMass(1.0f);
}

void Entity::Render()
{
	RenderCircle(m_shape.pos.x, m_shape.pos.y, m_shape.rad);
}

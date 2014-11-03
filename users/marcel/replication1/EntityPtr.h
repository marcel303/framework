#ifndef ENTITYPTR_H
#define ENTITYPTR_H
#pragma once

class Entity;

typedef SharedPtr<Entity> ShEntity;
typedef WeakPtr<Entity> RefEntity;

#endif

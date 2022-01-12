#pragma once
#include "Entity.h"
#include "Pathfinding.h"

class Enemy :  public Entity
{
public:

	Enemy(iPoint pos);
	virtual void Move(float dt);
	virtual void UpdatePath();


	int hitPoints;
	int score;
	float detectionDistance = 10; // in pixels

protected:
	int spriteRotation = 0;
	SDL_RendererFlip spriteDir;

	fPoint lastPosition;
	fPoint playerPos;
	b2Vec2 movementSpeed;

	PathFinding* pathfinding;
	PhysBody* physBody;


	Animation idleAnim, walkingAnim, jumpingAnim, deathAnim;
};


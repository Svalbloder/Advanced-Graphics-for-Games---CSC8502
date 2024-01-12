#include "Renderer.h"

void Renderer::NpcMove(float dt)
{
	float npcSpeed = 100.0f;
	Matrix4 targetPosition = Matrix4::Translation(Vector3(0, 0, 100000.0f));
	Matrix4 originalPosition = npcNode->GetTransform();
	Matrix4 currentTarget = targetPosition;

	Matrix4 npcDirection = (currentTarget - originalPosition).Normalised(); 
	bool isMovingTowardsTarget = true; 
	Matrix4 currentPosition = npcNode->GetTransform();
	Matrix4 displacement = npcDirection * npcSpeed * dt;
	Matrix4 newPosition = currentPosition + displacement;
	if (isMovingTowardsTarget && (newPosition - currentTarget).Length() <= npcSpeed * dt)
	{
		npcDirection = (originalPosition - currentTarget).Normalised();
		isMovingTowardsTarget = false;
	}
	npcNode->SetTransform(newPosition);

}
#include "Renderer.h"


void Renderer::RenderSplitScene()
{
	BuildNodeLists(root01);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);

	viewMatrix = camera01->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
		(float)width / (float)height, 45.0f);
	DrawSkybox();
	DrawHeightmap();
	DrawWater();
	DrawNodes(camera01);


	ClearNodeLists();
	BuildNodeLists(root02);
	SortNodeLists();
	glViewport(0.60 * width, 0.30 * height, (width / height) * width / 3, (width / height) * height / 3);
	DrawSkybox();
	DrawNodes(camera01);
	ClearNodeLists();

}
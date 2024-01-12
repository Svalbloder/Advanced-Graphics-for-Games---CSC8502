#include "Renderer.h"
#include "..\nclgl\CubeRobot.h"
#include <algorithm>
const int LIGHT_NUM = 32;
const int POST_PASSES = 10;
#define SHADOWSIZE 2048

Renderer::Renderer(Window& parent) :OGLRenderer(parent)
{
	
	earthTex = SOIL_load_OGL_texture(
		TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	earthBump = SOIL_load_OGL_texture(
		TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	diffuseTex = SOIL_load_OGL_texture(
		TEXTUREDIR"diffuse.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	ship = SOIL_load_OGL_texture(
		TEXTUREDIR"Spaceship_diffuse.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	waterTex = SOIL_load_OGL_texture(
		TEXTUREDIR"water.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	waterBump = SOIL_load_OGL_texture(
		TEXTUREDIR"waterbump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"Left_Tex.jpg", TEXTUREDIR"Right_Tex.jpg",
		TEXTUREDIR"Up_Tex.jpg", TEXTUREDIR"Down_Tex.jpg",
		TEXTUREDIR"Front_Tex.jpg", TEXTUREDIR"Back_Tex.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex || !waterBump || !diffuseTex)
	{
		return;
	}

	SetTextureRepeating(diffuseTex, true);
	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(waterBump, true);

	npc = Mesh::LoadFromMeshFile("Role_T.msh");
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	npcShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	pointlightShader = new Shader("pointlightvert.glsl", "pointlightfrag.glsl");
	combineShader = new Shader("combinevert.glsl", "combinefrag.glsl");
	processShader = new Shader("TexturedVertex.glsl", "ProcessFrag.glsl");
	shadowShader = new Shader("ShadowVertex.glsl", "ShadowFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess() || !sceneShader->LoadSuccess() || !npcShader->LoadSuccess() || !pointlightShader->LoadSuccess() || !combineShader->LoadSuccess() || !processShader->LoadSuccess())
	{
		return;
	}

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	heightMap = new HeightMap(TEXTUREDIR"noise.png");
	quad = Mesh::GenerateQuad();	//Environment
	object = Mesh::LoadFromMeshFile("SP_Tree.msh");
	object01 = Mesh::LoadFromMeshFile("SP_Planet.msh");
	object02 = Mesh::LoadFromMeshFile("SP_Crystal01.msh");
	object03 = Mesh::LoadFromMeshFile("SP_Crystal02.msh");
	object04 = Mesh::LoadFromMeshFile("SP_Sci-fi_Antenna.msh");
	object05 = Mesh::LoadFromMeshFile("Spaceship.msh");
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");

	anim = new MeshAnimation("Role_T.anm");
	material = new MeshMaterial("Role_T.mat");

	texture = SOIL_load_OGL_texture(TEXTUREDIR"stainedglass.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
	if (!sceneShader->LoadSuccess() || !texture)
	{
		return;
	}
	for (int i = 0; i < npc->GetSubMeshCount(); ++i)
	{
		const MeshMaterialEntry* matEntry =
			material->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	currentFrame = 0;
	frameTime = 0.0f;

	Vector3 dimensions = heightMap->GetHeightmapSize();
	
	//Scene Generation
	root01 = new SceneNode();
	
	ash = new Ash();
	root01->AddChild(ash);
	for (int i = 0; i < 5; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
		s->SetTransform(Matrix4::Translation(Vector3(0, 100.0f, -300.0f + 100.0f + 100 * i)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetBoundingRadius(100.0f);
		s->SetMesh(quad);
		s->SetTexture(texture);
		root01->AddChild(s);
	}
	root01->AddChild(new CubeRobot(cube));
	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 3.0f, 5.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(dimensions * Vector3(0.2f * i, 0.5f, 0.3f * i)));//1long2z3left
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetMesh(object04);
		s->SetTexture(diffuseTex);
		root01->AddChild(s);
	}

	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 3.0f, 5.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(dimensions * Vector3(0.19f * i, 0.5f, 0.2f * i)));
		s->SetModelScale(Vector3(120.0f, 120.0f, 120.0f));
		s->SetMesh(object03);
		s->SetTexture(diffuseTex);
		root01->AddChild(s);
	}

	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 3.0f, 5.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(dimensions * Vector3(0.5f * i, 0.5f, 0.1f * i)));
		s->SetModelScale(Vector3(400.0f, 400.0f, 400.0f));
		s->SetMesh(object02);
		s->SetTexture(diffuseTex);
		root01->AddChild(s);
	}

	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(dimensions * Vector3(2.0f * i, 0.5f, 2.1f * i)));
		s->SetModelScale(Vector3(300.0f, 300.0f, 300.0f));
		s->SetMesh(object01);
		s->SetTexture(diffuseTex);
		root01->AddChild(s);
	}
	for (int i = 1; i < 2; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 3.0f, 5.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(dimensions * Vector3(0.9f * i, 2.0f, 1.2f * i)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetMesh(object05);
		s->SetTexture(ship);
		root01->AddChild(s);
	}

	
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 3.0f, 5.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(dimensions * Vector3(1.2f, 0.08f, 1.2f)));
		s->SetModelScale(Vector3(250.0f, 250.0f, 250.0f));
		s->SetMesh(object);
		s->SetTexture(diffuseTex);
		root01->AddChild(s);


	npcNode = new SceneNode();
	npcNode->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	npcNode->SetTransform(Matrix4::Translation(dimensions* Vector3(0.3f, 0.4f, 0.45f)));
	npcNode->SetModelScale(Vector3(300.0f, 300.0f, 300.0f));
	npcNode->SetMesh(npc);
	npcNode->SetAniTexture(matTextures);
	npcNode->SetAnimation(anim);
	root01->AddChild(npcNode);
	root02 = new SceneNode();
	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(Vector3(1.0f * i, 0.1f, 2.1f * i)));
		s->SetModelScale(Vector3(500.0f, 500.0f, 500.0f));
		s->SetMesh(object01);
		s->SetTexture(diffuseTex);
		root02->AddChild(s);
	}


	camera02 = new Camera(10, 270, Vector3());
	camera02->SetPosition(dimensions * Vector3(0, 0, 0.));

	camera01 = new Camera(0, 270, Vector3());
	camera01->SetPosition(dimensions * Vector3(0, 1, 0.6));
	
		
	
	//Light
	light = new Light(dimensions * Vector3(0.5f, 1.5f, 0.5f),
		Vector4(1, 1, 1, 1), dimensions.x);

	pointLights = new Light[LIGHT_NUM];

	for (int i = 0; i < LIGHT_NUM; ++i)
	{
		Light& l = pointLights[i];
		l.SetPosition(Vector3(rand() % (int)dimensions.x, 150.0f, rand() % (int)dimensions.z));

		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX),
			0.5f + (float)(rand() / (float)RAND_MAX),
			0.5f + (float)(rand() / (float)RAND_MAX),
			1));

		l.SetRadius(250.0f + (rand() % 250));
	}

	//Matrix
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f,
		(float)width / (float)height, 45.0f);

	//Generate Deferred Rendering FBO
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1
	};

	GenerateScreenTexture(bufferDepthTex, true);
	if (!bufferDepthTex)
	{
		return;
	}
	for (int i = 0; i < 2; ++i)
	{
		GenerateScreenTexture(bufferColourTex[i]);
		if (!bufferColourTex[i])
		{
			return;
		}
	}
	GenerateScreenTexture(bufferNormalTex);
	if (!bufferNormalTex)
	{
		return;
	}
	GenerateScreenTexture(lightDiffuseTex);
	if (!lightDiffuseTex)
	{
		return;
	}
	GenerateScreenTexture(lightSpecularTex);
	if (!lightSpecularTex)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bufferColourTex[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, bufferNormalTex, 0);
	glDrawBuffers(2, buffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Enable Functions
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;

}

Renderer::~Renderer(void)
{
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	//Delete Meshes
	delete heightMap;
	delete quad;
	delete object;
	delete object01;
	delete object02;
	delete object03;
	delete object04;
	delete npc;
	delete sphere;
	delete cube;
	glDeleteTextures(1, &texture);

	//Delete Others
	delete camera01;
	delete camera02;
	delete light;
	delete root01;
	delete root02;
	delete[] pointLights;

	//Delete Shaders
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete sceneShader;
	delete npcShader;
	delete combineShader;
	delete pointlightShader;

	//Delete FBO
	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);

}
//Camera
void Renderer::UpdateScene(float dt)
{
	camera01->UpdateCamera(dt);
	viewMatrix = camera01->BuildViewMatrix();
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;

	frameTime -= dt;
	while (frameTime < 0.0f)
	{
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
	npcNode->SetCurrentFrame(currentFrame);

	//Scene root
	root01->Update(dt);
}
void Renderer::AutoUpdateCamera(float dt)
{
	Vector3 dimensions = heightMap->GetHeightmapSize();
	camera01->AutoUpdateCamera(dimensions, dt);
	viewMatrix = camera01->BuildViewMatrix();
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
	
	frameTime -= dt;
	while (frameTime < 0.0f)
	{
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
	npcNode->SetCurrentFrame(currentFrame);

	//Scene root
	root01->Update(dt);
}

//Scene Management
void Renderer::BuildNodeLists(SceneNode* from)
{

	Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera01->GetPosition();
	from->SetCameraDistance(Vector3::Dot(dir, dir));

	if (from->GetColour().w < 1.0f)
	{
		transparentNodeList.push_back(from);
	}
	else
	{
		nodeList.push_back(from);
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i)
	{
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists()
{
	std::sort(transparentNodeList.rbegin(),
		transparentNodeList.rend(),
		SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(),
		nodeList.end(),
		SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes(Camera* camera)
{
	for (const auto& i : nodeList)
	{
		DrawNode(camera, i);
	}
	for (const auto& i : transparentNodeList)
	{
		DrawNode(camera, i);
	}
}

void Renderer::DrawNode(Camera* camera, SceneNode* n)
{	
		if (n == npcNode) {
			
			BindShader(npcShader);

			glUniform1i(glGetUniformLocation(npcShader->GetProgram(), "diffuseTex"), 0);
			UpdateShaderMatrices();
			
			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(npcShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(npcShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(npcShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
			
			nodeTex = matTextures[0];

			glUniform1i(glGetUniformLocation(npcShader->GetProgram(), "useTexture"), nodeTex);

			vector<Matrix4> frameMatrices;

			const Matrix4* invBindPose = npc->GetInverseBindPose();
			const Matrix4* frameData = anim->GetJointData(currentFrame);

			for (unsigned int i = 0; i < npc->GetJointCount(); ++i)
			{
				frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
			}

			int j = glGetUniformLocation(npcShader->GetProgram(), "joints");
			glUniformMatrix4fv(j, frameMatrices.size(), false,
				(float*)frameMatrices.data());

			for (int i = 0; i < npc->GetSubMeshCount(); ++i)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, matTextures[i]);
				npc->DrawSubMesh(i);
			}
		}
		else
		{
			
			BindShader(sceneShader);
			
			glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(sceneShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			diffuseTex = n->GetTexture();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex);

			glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "useTexture"), diffuseTex);

			n->Draw(*this);

		}
}

void Renderer::ClearNodeLists()
{
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::RenderScene()
{
	BuildNodeLists(root01);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);
	viewMatrix = camera01->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,(float)width / (float)height, 45.0f);

	DrawSkybox();
	DrawHeightmap();
	DrawWater();
	DrawNodes(camera01);

	ClearNodeLists();
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth)
{
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}
void Renderer::RenderBlurScene() 
{
	BuildNodeLists(root01);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);
	viewMatrix = camera01->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,(float)width / (float)height, 45.0f);
	DrawHeightmap();
	DrawNodes(camera01);
	ClearNodeLists();
	
}
void Renderer::DrawPostProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		quad->Draw();

		// Now to swap the colour buffers and do the second blur pass
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();
}
//Functions
void Renderer::DrawScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,(float)width / (float)height, 45.0f);
	DrawSkybox();
	DrawHeightmap();
	DrawWater();
	DrawNodes(camera01);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawSkybox()
{
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap()
{
	BindShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(),
		"cameraPos"), 1, (float*)&camera01->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
		"diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
		"bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}



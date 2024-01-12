#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
#include "Ash.h"


class Renderer :public OGLRenderer
{
public:
	Renderer(Window& parent);
	~Renderer(void);

	//Scene
	void RenderScene() override;
	void RederDeferredScene();
	void RenderSplitScene();
	void RenderBlurScene();
	void UpdateScene(float dt) override;
	void AutoUpdateCamera(float dt);
	void NpcMove(float dt);
	void DrawShadowScene();
protected:

	void PresentScene();
	void DrawPostProcess();
	
	//Scene Management
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes(Camera* camera);
	void DrawNode(Camera* camera, SceneNode* n);

	//HeightMap Functions
	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();

	//deferred shading
	void DrawScene();
	void DrawPointLights();
	void CombineBuffers();
	void GenerateScreenTexture(GLuint& into, bool depth = false);
	
	//Scene
	SceneNode* root01;
	SceneNode* root02;
	SceneNode* npcNode;

	HeightMap* heightMap;
	Camera* camera01;
	Camera* camera02;
	Light* light;
	Light* pointLights;

	//Mesh
	Mesh* quad;		//Skybox
	Mesh* object;
	Mesh* object01;
	Mesh* object02;
	Mesh* object03;
	Mesh* object04;
	Mesh* object05;
	Mesh* cube;
	Mesh* npc;
	Mesh* sphere;
	//Shader
	Shader* lightShader;	//lighting terrain shader
	Shader* reflectShader;
	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* npcShader;
	Shader* pointlightShader;
	Shader* combineShader;
	Shader* processShader;
	Shader* shadowShader;

	//FBO
	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferDepthTex;
	GLuint bufferNormalTex;
	GLuint bufferColourTex[2];

	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;
	GLuint shadowFBO;

	//Texture
	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;	//Terrain texture
	GLuint earthBump;
	GLuint waterBump;
	GLuint diffuseTex;
	GLuint ship;
	GLuint nodeTex;
	GLuint heightTexture;
	GLuint texture;
	GLuint shadowTex;
	GLuint sceneDiffuse;
	GLuint sceneBump;

	//Animation
	MeshAnimation* anim;
	MeshMaterial* material;
	vector<Matrix4> sceneTransforms;
	vector <GLuint > matTextures;
	vector<Mesh*> sceneMeshes;
	//Anim frame
	int currentFrame;
	float frameTime;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	//Water
	float waterRotate;
	float waterCycle;

	//Ash
	Ash* ash;

};

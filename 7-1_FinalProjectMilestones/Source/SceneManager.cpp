////////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <iostream>


// shader uniform references
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = 0;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	if (m_loadedTextures > 0)
	{
		DestroyGLTextures();
	}
}


/***********************************************************
 *  CreateGLTexture()
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// prevent overflow beyond 16 texture slots
	if (m_loadedTextures >= 16)
	{
		std::cout << "ERROR: TEXTURE LIMIT REACHED. " << filename << std::endl;
		return false;
	}

	stbi_set_flip_vertically_on_load(true);
	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

	if (!image)
	{
		std::cout << "FAILED TO LOAD IMAGE: " << filename << std::endl;
		return false;
	}

	std::cout << "LOADED IMAAGE: " << filename << " (" << width << "x" << height << ", channels: " << colorChannels << ")" << std::endl;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (colorChannels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	else if (colorChannels == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	else
	{
		std::cout << "UNSUPPORTED: " << colorChannels  << std::endl;
		stbi_image_free(image);
		return false;
	}

	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	m_textureIDs[m_loadedTextures].ID = textureID;
	m_textureIDs[m_loadedTextures].tag = tag;
	m_loadedTextures++;

	return true;
}


/***********************************************************
 *  BindGLTextures()
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	if (m_loadedTextures <= 0)
	{
		std::cout << "NO TEXTURES TO DESTROY." << std::endl;
		return;
	}

	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].ID != 0)
		{
			glDeleteTextures(1, &m_textureIDs[i].ID);
			m_textureIDs[i].ID = 0;
		}
	}
	m_loadedTextures = 0;

	std::cout << "TEXTURES DESTROYED SUCCESSFULLY." << std::endl;
}


/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].tag == tag)
			return i;
	}
	return -1;
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string tag)
{
	if (m_pShaderManager == NULL)
		return;

	int slot = FindTextureSlot(tag);
	if (slot != -1)
	{
		m_pShaderManager->setIntValue("bUseTexture", true);
		m_pShaderManager->setSampler2DValue("objectTexture", slot);
	}
}

/***********************************************************
 *  LoadSceneTextures()
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	std::cout << "LOADING TEXTURES..." << std::endl;

	CreateGLTexture("textures/cake.jpg", "cake");
	CreateGLTexture("textures/floor.jpg", "floor");
	CreateGLTexture("textures/fridge.jpg", "fridge");
	CreateGLTexture("textures/wall.jpg", "wall");
	CreateGLTexture("textures/wood.jpg", "wood");
	CreateGLTexture("textures/grass.jpg", "grass");
	CreateGLTexture("textures/sky.jpg", "sky");
	CreateGLTexture("textures/ceiling.jpg", "ceiling");
	CreateGLTexture("textures/ceiling.jpg", "");
	CreateGLTexture("textures/paper.jpg", "paper");
	CreateGLTexture("textures/paper2.jpg", "paper2");
	CreateGLTexture("textures/frosting.jpg", "frosting");


	BindGLTextures();

	std::cout << "ALL TEXTURES LOADED." << std::endl;
}


/***********************************************************
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();

	LoadSceneTextures();
}


/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 scale = glm::scale(scaleXYZ);
	glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0));
	glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0));
	glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1));
	glm::mat4 translation = glm::translate(positionXYZ);

	glm::mat4 modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (m_pShaderManager)
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
	glm::vec4 color(r, g, b, a);
	if (m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, color);
	}
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	BindGLTextures();

	m_pShaderManager->setIntValue("bUseLighting", true);


	//LIGHT 0 

	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.0f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.0f);


	//LIGHT 1 (SUNLIGHT)

	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(0.0f, 6.5f, -14.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.08f, 0.06f, 0.03f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.6f, 0.45f, 0.25f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.7f, 0.55f, 0.35f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.7f);


	//LIGHT 2 (ROOM LIGHT)

	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, 20.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.025f, 0.025f, 0.025f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.06f, 0.06f, 0.06f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.08f, 0.08f, 0.08f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);



	//LIGHT 3

	m_pShaderManager->setVec3Value("lightSources[3].position", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.0f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);


	//MATERIALS

	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.025f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.75f, 0.75f, 0.75f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.3f, 0.3f, 0.3f));
	m_pShaderManager->setFloatValue("material.shininess", 20.0f);




//FLOOR//

	scaleXYZ = glm::vec3(25.0f, 1.0f, 25.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("floor");
	m_basicMeshes->DrawPlaneMesh();

	m_pShaderManager->setIntValue("bUseTexture", false);




//CEILING//

	float savedAmbientStrength = 0.03f;
	glm::vec3 savedSpecularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	float savedShininess = 24.0f;

	m_pShaderManager->setFloatValue("material.ambientStrength", 0.02f);
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.08f, 0.08f, 0.08f));
	m_pShaderManager->setFloatValue("material.shininess", 4.0f);

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("ceiling");
	scaleXYZ = glm::vec3(25.0f, 1.0f, 25.0f);
	positionXYZ = glm::vec3(0.0f, 12.0f, 0.0f);
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawPlaneMesh();

	m_pShaderManager->setFloatValue("material.ambientStrength", savedAmbientStrength);
	m_pShaderManager->setVec3Value("material.specularColor", savedSpecularColor);
	m_pShaderManager->setFloatValue("material.shininess", savedShininess);




//TRIM AROUND ROOM// 

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("trim");

	float trimHeight = 0.25f;
	float trimDepth = 0.1f;

	//BACK WALL

	scaleXYZ = glm::vec3(25.0f, trimHeight, trimDepth);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.125f, -12.45f));
	m_basicMeshes->DrawBoxMesh();

	// LEFT WALL

	scaleXYZ = glm::vec3(25.0f, trimHeight, trimDepth);
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, glm::vec3(-12.45f, 0.125f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	//RIGHT WALL

	scaleXYZ = glm::vec3(25.0f, trimHeight, trimDepth);
	SetTransformations(scaleXYZ, 0.0f, -90.0f, 0.0f, glm::vec3(12.45f, 0.125f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	//FRONT WALL

	scaleXYZ = glm::vec3(25.0f, trimHeight, trimDepth);
	SetTransformations(scaleXYZ, 0.0f, 180.0f, 0.0f, glm::vec3(0.0f, 0.125f, 12.45f));
	m_basicMeshes->DrawBoxMesh();



//BACK WALL//


	//BACK WALL LEFT

	scaleXYZ = glm::vec3(6.0f, 1.0f, 12.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-9.5f, 6.0f, -12.51f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();


	//BACK WALL RIGHT

	scaleXYZ = glm::vec3(6.0f, 1.0f, 12.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.5f, 6.0f, -12.51f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();


	//BACK WALL LEFT

	scaleXYZ = glm::vec3(6.0f, 1.0f, 4.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 10.5f, -12.51f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();


	// LEFT WALL
	scaleXYZ = glm::vec3(12.0f, 1.0f, 25.0f);
	positionXYZ = glm::vec3(-12.51f, 6.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();


	//RIGHT WALL

	scaleXYZ = glm::vec3(12.0f, 1.0f, 25.0f);
	positionXYZ = glm::vec3(12.51f, 6.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, -90.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();

	//FRONT WALL

	scaleXYZ = glm::vec3(25.0f, 1.0f, 12.0f);
	positionXYZ = glm::vec3(0.0f, 6.0f, 12.51f);     
	SetTransformations(scaleXYZ, -90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();



//FRIDGE//

	scaleXYZ = glm::vec3(3.5f, 6.5f, 3.0f);
	positionXYZ = glm::vec3(-10.5f, 3.25f, -9.5f);
	SetTransformations(scaleXYZ, 0.0f, 10.0f, 0.0f, positionXYZ);
	SetShaderTexture("fridge");
	m_basicMeshes->DrawBoxMesh();


	//FRIDGE HANDLES

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);


	//TOP HANDLE

	scaleXYZ = glm::vec3(0.15f, 1.1f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-8.9f, 5.05f, -8.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();


	//BOTTOM HANDLE

	scaleXYZ = glm::vec3(0.15f, 2.2f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-8.9f, 2.45f, -8.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();


	//A+ PAPER

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("paper");

	scaleXYZ = glm::vec3(0.7f, 0.9f, 0.01f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 2.0f;

	glm::vec3 paperPos(-10.5f, 4.5f, -7.95f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, paperPos);
	m_basicMeshes->DrawBoxMesh();
	;


	//STICK FIGURE PAPER

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("paper2");

	scaleXYZ = glm::vec3(0.7f, 0.9f, 0.01f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -1.5f;

	glm::vec3 paper2Pos(-10.5f, 3.25f, -7.95f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, paper2Pos);
	m_basicMeshes->DrawBoxMesh();



//TABLETOP//

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("wood");


	scaleXYZ = glm::vec3(5.0f, 0.15f, 5.0f);
	positionXYZ = glm::vec3(0.0f, 2.7f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();



//PLATE//

	m_pShaderManager->setIntValue("bUseTexture", false);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);

	scaleXYZ = glm::vec3(2.3f, 0.1f, 2.3f);
	positionXYZ = glm::vec3(0.0f, 2.95f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();



//WINDOW AND OUTDOORS//


	//BACK WALL

	scaleXYZ = glm::vec3(25.0f, 1.0f, 12.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 6.0f, -12.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wall");
	m_basicMeshes->DrawPlaneMesh();


	//SKY TEXTURE

	scaleXYZ = glm::vec3(2.95f, 1.0f, 1.65f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 7.0f, -12.35f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("sky");
	m_basicMeshes->DrawPlaneMesh();


	//GRASS TEXTURE

	scaleXYZ = glm::vec3(2.95f, 1.0f, 0.65f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 5.15f, -12.36f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("grass");
	m_basicMeshes->DrawPlaneMesh();


	//WINDOW FRAME

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);


	//TOP FRAME

	scaleXYZ = glm::vec3(6.4f, 0.3f, 0.3f);
	positionXYZ = glm::vec3(0.0f, 8.5f, -12.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();


	//BOTTOM FRAME

	scaleXYZ = glm::vec3(6.4f, 0.3f, 0.3f);
	positionXYZ = glm::vec3(0.0f, 4.5f, -12.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();


	//LEFT FRAME

	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f);
	positionXYZ = glm::vec3(-3.1f, 6.5f, -12.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();


	//RIGHT FRAME

	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f);
	positionXYZ = glm::vec3(3.1f, 6.5f, -12.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	//WINDOW SILL

	scaleXYZ = glm::vec3(6.0f, 0.3f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 4.2f, -12.2f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();



//WINDOW PANES//


	m_pShaderManager->setIntValue("bUseTexture", false);
	m_pShaderManager->setVec3Value("objectColor", glm::vec3(1.0f));

	float windowCenterX = 0.0f;
	float windowCenterY = 6.1f;
	float windowCenterZ = -12.33f;

	float windowWidth = 5.9f;
	float windowHeight = 4.4f;
	float paneThickness = 0.08f;


	//VERTICAL PANE

	scaleXYZ = glm::vec3(paneThickness, windowHeight, 0.05f);
	positionXYZ = glm::vec3(windowCenterX, windowCenterY + 0.4f, windowCenterZ + 0.02f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();


	//HORIZONTAL PANE

	scaleXYZ = glm::vec3(windowWidth, paneThickness, 0.05f);
	positionXYZ = glm::vec3(windowCenterX, windowCenterY, windowCenterZ + 0.02f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();



	
//TABLETOP LEGS//

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("wood");

	scaleXYZ = glm::vec3(0.3f, 2.7f, 0.3f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	float legRadius = 4.3f;

	//FRONT LEG

	positionXYZ = glm::vec3(0.0f, 0.0f, -3.0f + legRadius);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	//BACK LEG

	positionXYZ = glm::vec3(0.0f, 0.0f, -3.0f - legRadius);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	//LEFT LEG

	positionXYZ = glm::vec3(-legRadius, 0.0f, -3.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	//RIGHT LEG

	positionXYZ = glm::vec3(legRadius, 0.0f, -3.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();



	//CAKE//

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("cake");
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(0.9f, 0.8f, 0.6f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.3f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.9f, 0.8f, 0.6f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.05f, 0.05f, 0.05f));
	m_pShaderManager->setFloatValue("material.shininess", 4.0f);


	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f);
	positionXYZ = glm::vec3(0.0f, 3.0f, -3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();



	// BLUE CANDLE


	// CANDLE BODY

	m_pShaderManager->setIntValue("bUseTexture", false);
	SetShaderColor(0.15f, 0.35f, 0.85f, 1.0f);

	float cakeTopY = 3.0f + 1.0f;
	float candleBaseY = cakeTopY;
	float candleX = 0.0f;
	float candleZ = -3.0f;
	scaleXYZ = glm::vec3(0.12f, 1.0f, 0.12f);
	positionXYZ = glm::vec3(candleX, candleBaseY, candleZ);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();


	//WICK

	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	scaleXYZ = glm::vec3(0.02f, 0.10f, 0.02f);
	positionXYZ = glm::vec3(candleX, candleBaseY + 1.0f, candleZ);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();


	//FLAME

	SetShaderColor(1.0f, 0.8f, 0.3f, 1.0f);
	scaleXYZ = glm::vec3(0.10f, 0.18f, 0.10f);
	positionXYZ = glm::vec3(candleX, candleBaseY + 1.18f, candleZ);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();


	//FLAME LIGHT
	m_pShaderManager->setVec3Value("lightSources[4].position", glm::vec3(0.15f, 5.45f, -2.85f));
	m_pShaderManager->setVec3Value("lightSources[4].ambientColor", glm::vec3(0.4f, 0.25f, 0.1f)); 
	m_pShaderManager->setVec3Value("lightSources[4].diffuseColor", glm::vec3(1.0f, 0.85f, 0.55f));  
	m_pShaderManager->setVec3Value("lightSources[4].specularColor", glm::vec3(1.0f, 0.95f, 0.7f)); 
	m_pShaderManager->setFloatValue("lightSources[4].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[4].specularIntensity", 2.5f);


	// FROSTING

	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("frosting");

	scaleXYZ = glm::vec3(1.95f, 0.05f, 1.95f);

	positionXYZ = glm::vec3(0.0f, 4.02f, -3.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();




//ANT//

	m_pShaderManager->setIntValue("bUseTexture", false);
	m_pShaderManager->setVec3Value("material.ambientColor", glm::vec3(0.01f, 0.01f, 0.01f));
	m_pShaderManager->setFloatValue("material.ambientStrength", 0.15f);
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.03f, 0.03f, 0.03f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("material.shininess", 4.0f);
	m_pShaderManager->setVec3Value("objectColor", glm::vec3(0.0f, 0.0f, 0.0f));


	//ANT POSITION

	float tableTopY = 2.7f + 0.15f;
	float antBodyY = tableTopY + 0.09f;
	float legBaseY = tableTopY + 0.02f;



	//BODY

	scaleXYZ = glm::vec3(0.15f, 0.09f, 0.10f);
	positionXYZ = glm::vec3(-3.0f, antBodyY, -0.8f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.12f, 0.08f, 0.09f);
	positionXYZ = glm::vec3(-2.85f, antBodyY, -0.8f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.09f, 0.07f, 0.07f);
	positionXYZ = glm::vec3(-2.70f, antBodyY, -0.8f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();


	//LEGS

	scaleXYZ = glm::vec3(0.015f, 0.08f, 0.015f);
	float legZLeft = -0.87f;
	float legZRight = -0.73f;


	//LEFT

	for (int i = 0; i < 2; i++) {
		float offsetX = -3.05f + (i * 0.22f);
		float rotZ = 25.0f;
		positionXYZ = glm::vec3(offsetX, legBaseY, legZLeft);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, rotZ, positionXYZ);
		m_basicMeshes->DrawCylinderMesh();
	}

	//RIGHT

	for (int i = 0; i < 2; i++) {
		float offsetX = -3.05f + (i * 0.22f);
		float rotZ = -25.0f;
		positionXYZ = glm::vec3(offsetX, legBaseY, legZRight);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, rotZ, positionXYZ);
		m_basicMeshes->DrawCylinderMesh();
	}


	//ANTENNA//

	scaleXYZ = glm::vec3(0.02f, 0.07f, 0.02f);


	//LEFT

	XrotationDegrees = -35.0f;
	positionXYZ = glm::vec3(-2.68f, antBodyY + 0.065f, -0.81f);
	SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();


	//RIGHT

	XrotationDegrees = 35.0f;
	positionXYZ = glm::vec3(-2.73f, antBodyY + 0.065f, -0.81f);
	SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();


}

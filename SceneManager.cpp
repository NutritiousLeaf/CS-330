///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++) {
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

// NEW: SetupLights function
void SceneManager::SetupLights() {
	// Light Source 0 (main light - dim it a bit)
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.2f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.7f));  // was 0.5
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.9f)); // was 0.6
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.5f);

	// Light Source 1 (secondary - subtle fill light)
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.05f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.25f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.3f));
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);

	// Light Source 2 (warm reddish lamp glow)
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.1f, 0.05f, 0.05f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.9f, 0.3f, 0.3f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(1.0f, 0.5f, 0.5f));
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.6f);

	// Make sure lighting is enabled in your shader
	m_pShaderManager->setIntValue("bUseLighting", true);
}

// NEW: SetupMaterials function
void SceneManager::SetupMaterials() {
	m_objectMaterials.clear();

	OBJECT_MATERIAL steel = {
		0.2f,
		glm::vec3(0.2f),
		glm::vec3(0.7f),
		glm::vec3(1.0f),
		64.0f,
		"steel"
	};

	OBJECT_MATERIAL plastic = {
		0.1f,
		glm::vec3(0.1f),
		glm::vec3(0.5f),
		glm::vec3(0.3f),
		8.0f,
		"plastic"
	};

	OBJECT_MATERIAL darkplastic = {
		0.1f,
		glm::vec3(0.05f),
		glm::vec3(0.2f),
		glm::vec3(0.2f),
		4.0f,
		"darkplastic"
	};

	OBJECT_MATERIAL gold = {
		0.25f,
		glm::vec3(0.3f),
		glm::vec3(0.6f),
		glm::vec3(1.0f),
		64.0f,
		"gold"
	};

	OBJECT_MATERIAL burntsand = {
		0.1f,
		glm::vec3(0.2f),
		glm::vec3(0.45f),
		glm::vec3(0.2f),
		8.0f,
		"burntsand"
	};

	m_objectMaterials.push_back(steel);
	m_objectMaterials.push_back(plastic);
	m_objectMaterials.push_back(darkplastic);
	m_objectMaterials.push_back(gold);
	m_objectMaterials.push_back(burntsand);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	SetupLights();          // NEW: moved from inside this method
	LoadSceneTextures();
	SetupMaterials();       // NEW: moved from inside LoadSceneTextures()

	// Load all required meshes for the highly detailed desk lamp scene
	// Each mesh type only needs to be loaded once in memory
	m_basicMeshes->LoadPlaneMesh();          // For desk surface and backgrounds
	m_basicMeshes->LoadCylinderMesh();       // For lamp components and structural elements
	m_basicMeshes->LoadSphereMesh();         // For lamp shade and joints
	m_basicMeshes->LoadBoxMesh();            // For rectangular objects
	m_basicMeshes->LoadConeMesh();           // For lamp shade interior and tapered elements
	m_basicMeshes->LoadTaperedCylinderMesh(); // For lamp arm segments with realistic tapering
}

/***********************************************************
* LoadSceneTextures()
*
***********************************************************/
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainless");
	CreateGLTexture("../../Utilities/textures/gold-seamless-texture.jpg", "gold");
	CreateGLTexture("../../Utilities/textures/wood_cherry_seamless.jpg", "wood");
	CreateGLTexture("../../Utilities/textures/plastic_blue_seamless.jpg", "plastic");
	CreateGLTexture("../../Utilities/textures/plastic_dark_seamless.jpg", "darkplastic");

	BindGLTextures();
	// Material definitions are now in SetupMaterials()
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes to create
 *  a highly detailed and realistic desk lamp scene
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scale, pos;
	float xRot = 0, yRot = 0, zRot = 0;

	// === World Plane ===
	scale = glm::vec3(100.0f, 1.0f, 100.0f);
	pos = glm::vec3(0.0f, -0.3f, 0.0f);
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderMaterial("burntsand");
	m_basicMeshes->DrawPlaneMesh();

	// === Desk ===
	scale = glm::vec3(25.0f, 0.4f, 8.0f);
	pos = glm::vec3(0.0f, -0.3f, 0.0f);
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderTexture("wood");
	SetTextureUVScale(2.5f, 1.5f);
	m_basicMeshes->DrawBoxMesh();

	// === Lamp Base ===
	scale = glm::vec3(1.5f, 0.3f, 1.5f);
	pos = glm::vec3(-8.0f, 0.6f, 0.0f);
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderTexture("stainless");
	SetTextureUVScale(3.0f, 3.0f);
	m_basicMeshes->DrawCylinderMesh();

	// === Lamp Base Post ===
	scale = glm::vec3(0.3f, 1.0f, 0.3f);
	pos = glm::vec3(-8.0f, 1.0f, 0.0f); // half-height offset
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderTexture("darkplastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// === Lamp Lower Arm ===
	float lowerArmLength = 2.2f;
	scale = glm::vec3(0.15f, lowerArmLength, 0.15f);
	xRot = -50.0f;
	pos = glm::vec3(-8.0f, 2.0f, 0.0f);  // top of post
	SetTransformations(scale, xRot, 0, 0, pos);
	SetShaderMaterial("darkplastic");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// === Lamp Elbow Joint (connected to top of lower arm) ===
	float elbowOffset = -0.9f;

	float yRotElbow = 0.0f;
	float lowerArmAngleRad = glm::radians(-xRot);
	float lowerArmYRotRad = glm::radians(yRotElbow);

	float dx = lowerArmLength * sin(lowerArmAngleRad) * cos(lowerArmYRotRad);
	float dy = lowerArmLength * cos(lowerArmAngleRad);
	float dz = lowerArmLength * sin(lowerArmAngleRad) * sin(lowerArmYRotRad);

	float elbowX = pos.x + dx + elbowOffset;
	float elbowY = pos.y + dy;
	float elbowZ = pos.z + dz;

	scale = glm::vec3(0.25f, 0.25f, 0.25f);
	pos = glm::vec3(elbowX, elbowY, elbowZ);
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// === Lamp Upper Arm ===
	float upperArmLength = 2.0f;
	scale = glm::vec3(0.15f, upperArmLength, 0.15f);
	xRot = -25.0f;
	yRot = 90.0f;
	// To continue 3D alignment, update pos and use xRot, yRot for the new direction
	SetTransformations(scale, xRot, yRot, 0, pos);
	SetShaderTexture("darkplastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkplastic");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// === Lamp Head Joint (connected to top of upper arm) ===
	float upperX = pos.x + upperArmLength * sin(glm::radians(-xRot));
	float upperY = pos.y + upperArmLength * cos(glm::radians(-xRot));
	// If you want to include Z, use the same kind of calculation as for elbow
	scale = glm::vec3(0.25f, 0.25f, 0.25f);
	pos = glm::vec3(upperX, upperY, pos.z);
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// === Lamp Neck ===
	float neckLength = 0.7f;
	scale = glm::vec3(0.12f, neckLength, 0.12f);
	xRot = -50.0f;
	glm::vec3 neckStart = pos;
	SetTransformations(scale, xRot, 0, 0, neckStart);
	SetShaderMaterial("darkplastic");
	m_basicMeshes->DrawCylinderMesh();

	// === Lamp Shade (at end of neck) ===
	float shadeX = neckStart.x + neckLength * sin(glm::radians(-xRot));
	float shadeY = neckStart.y + neckLength * cos(glm::radians(-xRot));
	scale = glm::vec3(1.2f, 0.8f, 1.2f);
	xRot = 220.0f;
	pos = glm::vec3(shadeX, shadeY, neckStart.z);
	SetTransformations(scale, xRot, 0, 0, pos);
	SetShaderTexture("stainless");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// === Lamp Bulb ===
	scale = glm::vec3(0.2f, 0.2f, 0.2f);
	pos = glm::vec3(shadeX, shadeY + 0.2f, neckStart.z);
	SetTransformations(scale, 0, 0, 0, pos);
	SetShaderTexture("gold");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("gold");
	m_basicMeshes->DrawSphereMesh();

	// ==== CLOSED LAPTOP ====
	glm::vec3 laptopPos = glm::vec3(0.0f, -0.1f + 0.13f / 2, 0.0f); // centered, just on top of desk
	glm::vec3 laptopScale = glm::vec3(7.0f, 0.13f, 4.5f); // wide, thin

	// Laptop base
	SetTransformations(laptopScale, 0, 0, 0, laptopPos);
	SetShaderTexture("darkplastic");      
	SetTextureUVScale(2.0f, 1.5f);
	SetShaderMaterial("darkplastic");
	m_basicMeshes->DrawBoxMesh();

	// Laptop lid (closed, just above base)
	glm::vec3 lidScale = glm::vec3(6.9f, 0.10f, 4.4f); // slightly smaller
	glm::vec3 lidPos = laptopPos + glm::vec3(0.0f, (laptopScale.y + lidScale.y) / 2, 0.0f); // slightly higher
	SetTransformations(lidScale, 0, 0, 0, lidPos);
	SetShaderTexture("stainless");     
	SetTextureUVScale(2.0f, 1.5f);
	SetShaderMaterial("steel");
	m_basicMeshes->DrawBoxMesh();

	// ==== COFFEE CUP ====
	glm::vec3 cupPos = glm::vec3(4.5f, -0.1f + 0.0f / 2, 0.3f); // right of laptop, near books
	glm::vec3 cupScale = glm::vec3(0.42f, 0.95f, 0.42f); // cup body proportions

	// Cup body
	SetTransformations(cupScale, 0, 0, 0, cupPos);
	SetShaderTexture("plastic"); 
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();

	// Cup rim
	glm::vec3 rimScale = glm::vec3(0.48f, 0.09f, 0.48f);
	glm::vec3 rimPos = cupPos + glm::vec3(0.0f, 0.94f, 0.0f);
	SetTransformations(rimScale, 180.0, 0, 0, rimPos);
	SetShaderTexture("stainless");
	SetTextureUVScale(2.0f, 1.5f);
	SetShaderMaterial("steel");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Cup handle
	float mugRadius = cupScale.x / 2.0f;
	float handleRadius = mugRadius * 0.8f; // slightly smaller than mug radius to hug the mug
	float mugCenterY = cupPos.y + (cupScale.y / 2.0f);
	float segmentLength = 0.13f;           // segment length to overlap/fit the curve
	float segmentRadius = 0.07f;           // thickness of handle
	int handleSegments = 7;                // more segments = smoother handle

	// Center of the handle arc (right side of mug)
	float handleCenterX = cupPos.x + mugRadius + handleRadius + segmentRadius * 0.72f;

	for (int i = 0; i < handleSegments; ++i) {
		// Evenly spaced angles from top (-90°) to bottom (+90°)
		float t = (float)i / (handleSegments - 1);
		float angle = glm::radians(-90.0f + t * 180.0f);

		// Arc in the Y (height) and X (out from cup) plane
		float x = handleCenterX + handleRadius * cos(angle);
		float y = mugCenterY + handleRadius * sin(angle);
		float z = cupPos.z;

		// Each segment is tangent to the arc: rotate around Z
		float zRot = glm::degrees(angle);

		SetTransformations(
			glm::vec3(segmentRadius, segmentLength, segmentRadius),
			0, 0, zRot, // Xrot, Yrot, Zrot
			glm::vec3(x, y, z)
		);
		SetShaderTexture("plastic");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderMaterial("plastic");
		m_basicMeshes->DrawCylinderMesh();
	}

	// Book 1 (tall, back)
	glm::vec3 book1Pos = glm::vec3(5.90f, -0.1f + 0.95f / 2, 1.0f); // rightmost
	glm::vec3 book1Scale = glm::vec3(0.50f, 2.17f, 1.62f);
	SetTransformations(book1Scale, 0, 0, -8, book1Pos); // slight tilt for realism
	SetShaderTexture("darkplastic");
	SetTextureUVScale(0.5f, 1.0f);
	SetShaderMaterial("darkplastic");
	m_basicMeshes->DrawBoxMesh();

	// Book 2 (shorter, in front)
	glm::vec3 book2Pos = glm::vec3(6.5f, -0.1f + 0.95f / 2, 1.0f); // slightly left and forward
	glm::vec3 book2Scale = glm::vec3(0.47f, 1.67f, 1.37f);
	SetTransformations(book2Scale, 0, 0, 0, book2Pos); // opposite tilt
	SetShaderTexture("plastic");
	SetTextureUVScale(0.5f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// Book 3, lying flat (extra realism/points)
	glm::vec3 book3Pos = glm::vec3(7.89f, -0.1f + 0.18f / 2, 0.98f);
	glm::vec3 book3Scale = glm::vec3(1.37f, 0.45f, 2.27f);
	SetTransformations(book3Scale, 0, -90, 0, book3Pos); // on its side
	SetShaderTexture("gold");
	SetTextureUVScale(0.7f, 1.0f);
	SetShaderMaterial("gold");
	m_basicMeshes->DrawBoxMesh();

}
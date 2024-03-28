#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include <vector>
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Material.h"
#include "Lights.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// UI variables
	float bgColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	bool showDemoUI = false;
	bool thisBox = false;
	bool thatBox = false;

	// Vectors to hold game objects
	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<Material>> materials;
	std::vector<std::shared_ptr<GameEntity>> entities;
	std::vector<Light> lights;
	std::vector<std::shared_ptr<Camera>> cameras;
	int activeCameraIndex = 0;

	// Initialization helper methods
	void LoadShaders(); 
	void CreateGeometry();
	void CreateEntities();
	void CreateLights();
	void CreateCameras();
	void LoadMaterials();
	void UpdateImGui(float deltaTime, float totalTime);
	void BuildUI();

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Shaders and shader-related constructs
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::vector<std::shared_ptr<SimplePixelShader>> customShaders;
	DirectX::XMFLOAT3 ambientColor = { 0.1f, 0.1f, 0.25f };
};


#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Mesh.h"
#include <vector>
#include <math.h>
#include <string>

#include "WICTextureLoader.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialization helpers
	InitShadows();
	InitPostProcessing();
	LoadShaders();
	CreateGeometry();
	LoadMaterials();
	CreateEntities();
	CreateLights();
	CreateCameras();

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();
	
	// Set initial graphics API state
	{
		// Tell the input assembler (IA) what kind of geometric primitives we want to draw. 
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
}

// --------------------------------------------------------
// Set up resources required for shadow mapping
// --------------------------------------------------------
void Game::InitShadows()
{
	// Create the shadow map texture
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution; 
	shadowDesc.Height = shadowMapResolution; 
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	// Set up the rasterizer state for depth biasing
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	// Set up the sampler for softer shadows
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f;
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	// Set up the light projection matrix
	float lightProjectionSize = 12.0f;
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);
	XMStoreFloat4x4(&lightProjectionMatrix, lightProjection);

}

// --------------------------------------------------------
// Calculates the light view matrix of the given light
// --------------------------------------------------------
void Game::UpdateLightViewMatrix(Light light)
{
	XMVECTOR lightDirection = XMLoadFloat3(&light.Direction);
	XMMATRIX lightView = XMMatrixLookToLH(
		lightDirection * -1.0f * 7.0f,
		lightDirection,
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		);
	XMStoreFloat4x4(&lightViewMatrix, lightView);
}

// --------------------------------------------------------
// Set up resources required for post processing effects
// --------------------------------------------------------
void Game::InitPostProcessing()
{
	// Create the render target description
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = windowWidth;
	textureDesc.Height = windowHeight;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture;
	device->CreateTexture2D(&textureDesc, 0, ppTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(ppTexture.Get(), &rtvDesc, ppRTV.ReleaseAndGetAddressOf());

	// Create the Shader Resource View
	device->CreateShaderResourceView(ppTexture.Get(), 0, ppSRV.ReleaseAndGetAddressOf());
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
	VS_NormalMap = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader_NormalMap.cso").c_str());
	PS_NormalMap = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader_NormalMap.cso").c_str());
	VS_Sky = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"SkyVertexShader.cso").c_str());
	PS_Sky = std::make_shared<SimplePixelShader>(device, context, FixPath(L"SkyPixelShader.cso").c_str());
	VS_Shadow = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"ShadowVertexShader.cso").c_str());
	ppVS = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"FullscreenVertexShader.cso").c_str());
	ppPS = std::make_shared<SimplePixelShader>(device, context, FixPath(L"BlurPixelShader.cso").c_str());
	customShaders.push_back(std::make_shared<SimplePixelShader>(device, context, FixPath(L"CustomPS.cso").c_str()));
}


// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	#pragma region /* Old Mesh Code */
	/*
	// Big rectangle
	Vertex vertices[] =
	{
		{ XMFLOAT3(+0.5f, +0.5f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, +0.5f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }
	};
	unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

	meshes.push_back(std::make_shared<Mesh>(
		"Big Rectangle",	// Name of mesh
		vertices,			// Array of vertices
		4,					// Number of vertices
		indices,			// Array of indices
		6,					// Number of indices
		device,				// Pointer to "device"
		context				// Pointer to "context"
	));
	
	// Star
	Vertex vertices2[] =
	{
		{ XMFLOAT3(+0.0f, -0.1f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(-0.2f, -0.2f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(-0.1f, +0.0f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(-0.25f, +0.1f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(-0.05f, +0.1f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(+0.0f, +0.2f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(+0.05f, +0.1f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(+0.25f, +0.1f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(+0.1f, +0.0f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
		{ XMFLOAT3(+0.2f, -0.2f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)}
	};
	unsigned int indices2[] = { 0, 1, 5, 2, 3, 4, 5, 9, 0, 6, 7, 8 };
	meshes.push_back(std::make_shared<Mesh>("Star", vertices2, 10, indices2, 12, device, context));

	// Small green rectangle
	Vertex vertices3[] =
	{
		{ XMFLOAT3(-0.1f, +0.15f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(+0.1f, +0.15f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(+0.1f, -0.15f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.1f, -0.15f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }
	};
	unsigned int indices3[] = { 0, 1, 2, 2, 3, 0 };
	meshes.push_back(std::make_shared<Mesh>("Rectangle", vertices3, 4, indices3, 6, device, context));

	// Small Triangle
	Vertex vertices4[] =
	{
		{ XMFLOAT3(+0.0f, +0.25f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(+0.5f, -0.25f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-0.5f, -0.25f, +0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }
	};
	unsigned int indices4[] = { 0, 1, 2 };

	meshes.push_back(std::make_shared<Mesh>(
		"Mini Triangle",		// Name of mesh
		vertices4,				// Array of vertices
		3,						// Number of vertices
		indices4,				// Array of indices
		3,						// Number of indices
		device,
		context
	));
	*/
	#pragma endregion

	meshes.push_back(std::make_shared<Mesh>("Cube", FixPath(L"../../Assets/Models/cube.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>("Cylinder", FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>("Helix", FixPath(L"../../Assets/Models/helix.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>("Quad", FixPath(L"../../Assets/Models/quad.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>("Quad_Double_Sided", FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>("Sphere", FixPath(L"../../Assets/Models/sphere.obj").c_str(), device, context));
	meshes.push_back(std::make_shared<Mesh>("Torus", FixPath(L"../../Assets/Models/torus.obj").c_str(), device, context));
}


// --------------------------------------------------------
// Load textures and create materials for our objects
// --------------------------------------------------------
void Game::LoadMaterials()
{
	#pragma region Loading Texture Images

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBasic;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Basic/Basic_albedo.png").c_str(),
		nullptr, srvBasic.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv1Albedo;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Cobblestone/cobblestone_albedo.png").c_str(),
		nullptr, srv1Albedo.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv1Normal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Cobblestone/cobblestone_normals.png").c_str(),
		nullptr, srv1Normal.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv1Roughness;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Cobblestone/cobblestone_roughness.png").c_str(),
		nullptr, srv1Roughness.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv1Metal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Cobblestone/cobblestone_metal.png").c_str(),
		nullptr, srv1Metal.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2Albedo;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Bronze/bronze_albedo.png").c_str(),
		nullptr, srv2Albedo.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2Normal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Bronze/bronze_normals.png").c_str(),
		nullptr, srv2Normal.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2Roughness;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Bronze/bronze_roughness.png").c_str(),
		nullptr, srv2Roughness.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2Metal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Bronze/bronze_metal.png").c_str(),
		nullptr, srv2Metal.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv3Albedo;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Wood/wood_albedo.png").c_str(),
		nullptr, srv3Albedo.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv3Normal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Wood/wood_normals.png").c_str(),
		nullptr, srv3Normal.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv3Roughness;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Wood/wood_roughness.png").c_str(),
		nullptr, srv3Roughness.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv3Metal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Wood/wood_metal.png").c_str(),
		nullptr, srv3Metal.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv4Albedo;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Scratched/scratched_albedo.png").c_str(),
		nullptr, srv4Albedo.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv4Normal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Scratched/scratched_normals.png").c_str(),
		nullptr, srv4Normal.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv4Roughness;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Scratched/scratched_roughness.png").c_str(),
		nullptr, srv4Roughness.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv4Metal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Scratched/scratched_metal.png").c_str(),
		nullptr, srv4Metal.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv5Albedo;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Rough/rough_albedo.png").c_str(),
		nullptr, srv5Albedo.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv5Normal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Rough/rough_normals.png").c_str(),
		nullptr, srv5Normal.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv5Roughness;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Rough/rough_roughness.png").c_str(),
		nullptr, srv5Roughness.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv5Metal;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Rough/rough_metal.png").c_str(),
		nullptr, srv5Metal.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvRamp;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/ramp.png").c_str(),
		nullptr, srvRamp.GetAddressOf());

	#pragma endregion

	// Create sampling state
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 12;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&sampDesc, sampler.GetAddressOf());
	
	// Create basic color materials
	materials.push_back(std::make_shared<Material>(0.9f, 0.2f, 0.2f, 1.0f, 0.1f, vertexShader, pixelShader)); // Red
	materials.push_back(std::make_shared<Material>(0.145f, 0.878f, 0.365f, 1.0f, 0.9f, vertexShader, customShaders[0])); // Rainbow
	for (int i = 0; i < 2; i++) {
		materials[i]->AddTextureSRV("Albedo", srvBasic);
		materials[i]->AddTextureSRV("RoughnessMap", srvBasic);
		materials[i]->AddSampler("BasicSampler", sampler);
	}

	// Create textured materials
	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, VS_NormalMap, PS_NormalMap)); // Cobblestone
	materials[2]->AddTextureSRV("Albedo", srv1Albedo);
	materials[2]->AddTextureSRV("NormalMap", srv1Normal);
	materials[2]->AddTextureSRV("RoughnessMap", srv1Roughness);
	materials[2]->AddTextureSRV("MetalnessMap", srv1Metal);
	materials[2]->AddSampler("BasicSampler", sampler);

	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, VS_NormalMap, PS_NormalMap)); // Bronze
	materials[3]->AddTextureSRV("Albedo", srv2Albedo);
	materials[3]->AddTextureSRV("NormalMap", srv2Normal);
	materials[3]->AddTextureSRV("RoughnessMap", srv2Normal);
	materials[3]->AddTextureSRV("MetalnessMap", srv2Metal);
	materials[3]->AddSampler("BasicSampler", sampler);

	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, VS_NormalMap, PS_NormalMap)); // Wood
	materials[4]->AddTextureSRV("Albedo", srv3Albedo);
	materials[4]->AddTextureSRV("NormalMap", srv3Normal);
	materials[4]->AddTextureSRV("RoughnessMap", srv3Normal);
	materials[4]->AddTextureSRV("MetalnessMap", srv3Metal);
	materials[4]->AddSampler("BasicSampler", sampler);

	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, VS_NormalMap, PS_NormalMap)); // Scratched
	materials[5]->AddTextureSRV("Albedo", srv4Albedo);
	materials[5]->AddTextureSRV("NormalMap", srv4Normal);
	materials[5]->AddTextureSRV("RoughnessMap", srv4Normal);
	materials[5]->AddTextureSRV("MetalnessMap", srv4Metal);
	materials[5]->AddSampler("BasicSampler", sampler);

	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, VS_NormalMap, PS_NormalMap)); // Rough
	materials[6]->AddTextureSRV("Albedo", srv5Albedo);
	materials[6]->AddTextureSRV("NormalMap", srv5Normal);
	materials[6]->AddTextureSRV("RoughnessMap", srv5Normal);
	materials[6]->AddTextureSRV("MetalnessMap", srv5Metal);
	materials[6]->AddSampler("BasicSampler", sampler);

	// Add ramp for cel shading
	for (int i = 0; i < materials.size(); i++) {
		materials[i]->AddTextureSRV("Ramp", srvRamp);
	}

	// Create sky box
	sky = std::make_shared<Sky>(meshes[0], sampler, device, context, VS_Sky, PS_Sky,
		FixPath(L"../../Assets/Textures/Skies/Cold Sunset/right.png").c_str(),
		FixPath(L"../../Assets/Textures/Skies/Cold Sunset/left.png").c_str(),
		FixPath(L"../../Assets/Textures/Skies/Cold Sunset/up.png").c_str(),
		FixPath(L"../../Assets/Textures/Skies/Cold Sunset/down.png").c_str(),
		FixPath(L"../../Assets/Textures/Skies/Cold Sunset/front.png").c_str(),
		FixPath(L"../../Assets/Textures/Skies/Cold Sunset/back.png").c_str()
		);
}


// --------------------------------------------------------
// Creates the GameEntities that will be drawn to the screen
// --------------------------------------------------------
void Game::CreateEntities() 
{	
	entities.push_back(std::make_shared<GameEntity>(meshes[1], materials[2])); // Cobblestone cylinder
	entities[0]->GetTransform()->SetPosition(4.5f, 0.5f, 1.0f);
	entities[0]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[5], materials[5])); // Scratched sphere
	entities[1]->GetTransform()->SetPosition(-0.7f, -0.2f, 0.0f);
	entities[1]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[6], materials[3])); // Bronze torus
	entities[2]->GetTransform()->SetPosition(-1.3f, +1.0f, 0.0f);
	entities[2]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[0], materials[4])); // Wood cube
	entities[3]->GetTransform()->SetPosition(+1.5f, -0.5f, 0.0f);
	entities[3]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[2], materials[0])); // Red helix
	entities[4]->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
	entities[4]->GetTransform()->SetPosition(0.4f, 0.7f, 0.0f);

	entities.push_back(std::make_shared<GameEntity>(meshes[5], materials[1])); // Rainbow sphere
	entities[5]->GetTransform()->SetPosition(-2.0f, 0.0f, -1.0f);
	entities[5]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[0], materials[6])); // Floor
	entities[6]->GetTransform()->SetPosition(0.0f, -2.0f, +2.5f);
	entities[6]->GetTransform()->SetScale(6.0f, 0.3f, 6.0f);

	entities.push_back(std::make_shared<GameEntity>(meshes[1], materials[2])); // Distant pillar 1
	entities[7]->GetTransform()->SetPosition(+1.5f, -0.5f, 3.0f);
	entities[7]->GetTransform()->SetScale(0.5f, 3.0f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[1], materials[2])); // Distant pillar 2
	entities[8]->GetTransform()->SetPosition(+1.5f, -0.5f, 8.0f);
	entities[8]->GetTransform()->SetScale(0.5f, 3.0f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[1], materials[2])); // Distant pillar 3
	entities[9]->GetTransform()->SetPosition(-1.5f, -0.5f, 5.0f);
	entities[9]->GetTransform()->SetScale(0.5f, 3.0f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[1], materials[2])); // Distant pillar 4
	entities[10]->GetTransform()->SetPosition(-1.5f, -0.5f, 11.0f);
	entities[10]->GetTransform()->SetScale(0.5f, 3.0f, 0.5f);

	/*
	entities.push_back(std::make_shared<GameEntity>(meshes[0], materials[1])); // Cube duplicate (for shader testing)
	entities[6]->GetTransform()->SetPosition(+0.8f, -0.5f, -1.5f);
	entities[6]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[5], materials[1])); // Sand sphere duplicate (for shader testing)
	entities[7]->GetTransform()->SetPosition(-0.7f, -0.2f, -1.5f);
	entities[7]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);
	*/
}


// --------------------------------------------------------
// Creates all the lights in the scene
// --------------------------------------------------------
void Game::CreateLights()
{
	// Main directional
	lights.push_back(Light{});
	lights[0].Direction = XMFLOAT3(0.0f, -2.0f, 1.0f);
	lights[0].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[0].Intensity = 1.0f;
	UpdateLightViewMatrix(lights[0]);

	// Added directional
	lights.push_back(Light{});
	lights[1].Direction = XMFLOAT3(0.0f, -1.0f, 0.3f);
	lights[1].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[1].Intensity = 1.0f;

	// Added directional
	lights.push_back(Light{});
	lights[2].Direction = XMFLOAT3(1.0f, 1.0f, 2.3f);
	lights[2].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[2].Intensity = 1.0f;
}


// --------------------------------------------------------
// Set up all the cameras in the scene and set initial active camera
// --------------------------------------------------------
void Game::CreateCameras()
{
	// Initialize camera field
	DirectX::XMFLOAT3 camInitPos = { 0.04f, 0.0f, -3.92f };
	DirectX::XMFLOAT3 camInitRot = { 0.0f, 0.0f, -1.0f };
	cameras.push_back(std::make_shared<Camera>((float)this->windowWidth / this->windowHeight, camInitPos, camInitRot, (float)XM_PI / 3));

	camInitPos = { -1.29f, -0.46f, 1.06f };
	camInitRot = { -0.3f, 2.5f, 0.95f };
	cameras.push_back(std::make_shared<Camera>(
		(float)this->windowWidth / this->windowHeight,
		camInitPos,
		camInitRot,
		XM_PIDIV2
	));
	activeCameraIndex = 0;
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	for (int i = 0; i < cameras.size(); i++) {
		cameras[i]->UpdateProjectionMatrix((float)this->windowWidth / this->windowHeight);
	}
}

// --------------------------------------------------------
// Updates the UI data every frame
// --------------------------------------------------------
void Game::UpdateImGui(float deltaTime, float totalTime) 
{
	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);

	// Show the demo window
	if (showDemoUI) {
		ImGui::ShowDemoWindow();
	}
}

// --------------------------------------------------------
// Creates the UI layout
// --------------------------------------------------------
void Game::BuildUI() 
{
	ImGui::Begin("Inspector");

	// --------------------------------------------
	// APP DETAILS - frame rate, window size, background color
	// --------------------------------------------
	if (ImGui::TreeNode("App Details")) 
	{
		ImGui::Text("Frame Rate: %f fps", ImGui::GetIO().Framerate);
		ImGui::Text("Window Client Size: %dx%d", windowWidth, windowHeight);
		ImGui::ColorEdit4("Background Color", bgColor);
		ImGui::Spacing();
		if (ImGui::Button("Show ImGui Demo Window")) {
			showDemoUI = !showDemoUI;
		}

		ImGui::TreePop();
	}

	// --------------------------------------------
	// MESHES - mesh data
	// --------------------------------------------
	if (ImGui::TreeNode("Meshes"))
	{
		for (int i = 0; i < meshes.size(); i++)
		{
			ImGui::Text("(%03d) %s: %d triangle(s)", i + 1, meshes[i]->GetName().c_str(), meshes[i]->GetIndexCount() / 3);
		}

		ImGui::TreePop();
	}

	// --------------------------------------------
	// SCENE ENTITIES - entity data
	// --------------------------------------------
	
	if (ImGui::TreeNode("Scene Entities"))
	{
		for (int i = 0; i < entities.size(); i++)
		{
			std::string title = "Entity ";
			title.append(std::to_string(i)).append(" (").append(entities[i]->GetMesh()->GetName()).append(")");
			if (ImGui::TreeNode(title.c_str()))
			{
				float entPos[3] = {
					entities[i]->GetTransform()->GetPosition().x,
					entities[i]->GetTransform()->GetPosition().y,
					entities[i]->GetTransform()->GetPosition().z,
				};
				ImGui::DragFloat3("Position", entPos, 0.01f);
				entities[i]->GetTransform()->SetPosition(entPos[0], entPos[1], entPos[2]);

				float entRot[3] = {
					entities[i]->GetTransform()->GetPitchYawRoll().x,
					entities[i]->GetTransform()->GetPitchYawRoll().y,
					entities[i]->GetTransform()->GetPitchYawRoll().z,
				};
				ImGui::DragFloat3("Rotation", entRot, 0.01f);
				entities[i]->GetTransform()->SetRotation(entRot[0], entRot[1], entRot[2]);

				float entScl[3] = {
					entities[i]->GetTransform()->GetScale().x,
					entities[i]->GetTransform()->GetScale().y,
					entities[i]->GetTransform()->GetScale().z,
				};
				ImGui::DragFloat3("Scale", entScl, 0.01f);
				entities[i]->GetTransform()->SetScale(entScl[0], entScl[1], entScl[2]);

				ImGui::Text("Mesh Index Count: %d", entities[i]->GetMesh()->GetIndexCount());
				ImGui::Spacing();

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();

		
	}

	// --------------------------------------------
	// CAMERA - camera data
	// --------------------------------------------
	
	if (ImGui::TreeNode("Camera")) 
	{
		ImGui::Text("Active Camera: ");
		ImGui::SameLine(0.0f, 10.0f);
		if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { 
			activeCameraIndex--; 
			if (activeCameraIndex <= -1) activeCameraIndex = (int)cameras.size() - 1;
		}
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::Text("Camera %d", activeCameraIndex + 1);
		ImGui::SameLine(0.0f, 10.0f);
		if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { 
			activeCameraIndex++; 
			if (activeCameraIndex >= cameras.size()) activeCameraIndex = 0;
		}
		ImGui::SameLine(0.0f, 20.0f);
		
		std::shared_ptr<Camera> camera = cameras[activeCameraIndex];
		DirectX::XMFLOAT3 camPos = camera->GetTransform()->GetPosition();
		DirectX::XMFLOAT3 camForward = camera->GetTransform()->GetForward();
		ImGui::Text("FOV: %6.2f", camera->GetFOV());
		ImGui::Text("Camera Position:       %6.2f  %6.2f  %6.2f", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Forward Vector: %6.2f  %6.2f  %6.2f", camForward.x, camForward.y, camForward.z);

		ImGui::TreePop();
	}

	// --------------------------------------------
	// LIGHTS - light data, allows user to edit
	// --------------------------------------------

	if (ImGui::TreeNode("Lights"))
	{
		int dirLights = 0;
		int pointLights = 0;
		XMFLOAT3 oldDir = lights[0].Direction;
		for (int i = 0; i < (int)lights.size(); i++)
		{
			// Get the title
			std::string title = "";
			if (lights[i].Type == 0)
			{
				dirLights++;
				title.append("Directional Light #");
				title.append(std::to_string(dirLights));
			}
			else if (lights[i].Type == 1)
			{
				pointLights++;
				title.append("Point Light #");
				title.append(std::to_string(pointLights));
			}

			if (ImGui::TreeNode(title.c_str()))
			{
				if (lights[i].Type == 1 || lights[i].Type == 2)
				{
					float lightPos[3] = {
						lights[i].Position.x,
						lights[i].Position.y,
						lights[i].Position.z
					};
					ImGui::DragFloat3("Position", lightPos, 0.01f);
					lights[i].Position = DirectX::XMFLOAT3(lightPos[0], lightPos[1], lightPos[2]);
				}
				if (lights[i].Type == 0 || lights[i].Type == 2)
				{
					float lightDir[3] = {
						lights[i].Direction.x,
						lights[i].Direction.y,
						lights[i].Direction.z
					};
					ImGui::DragFloat3("Direction", lightDir, 0.01f);
					lights[i].Direction = DirectX::XMFLOAT3(lightDir[0], lightDir[1], lightDir[2]);
					
				}
				
				float color[3] = {
					lights[i].Color.x,
					lights[i].Color.y,
					lights[i].Color.z
				};
				ImGui::ColorEdit3("Color", color);
				lights[i].Color = DirectX::XMFLOAT3(color[0], color[1], color[2]);

				ImGui::DragFloat("Intensity", &lights[i].Intensity, 0.1f, 0.0f, 100.0f);

				if (lights[i].Type == 1 || lights[i].Type == 2)
				{
					ImGui::DragFloat("Range", &lights[i].Range, 0.1f, 0.0f, 10000.0f);
				}

				ImGui::TreePop();
			}
		}

		if (lights[0].Direction.x != oldDir.x ||
			lights[0].Direction.y != oldDir.y ||
			lights[0].Direction.z != oldDir.z) {
			UpdateLightViewMatrix(lights[0]);
		}

		ImGui::TreePop();
	}

	// --------------------------------------------
	// POST PROCESSING - blur and more
	// --------------------------------------------

	if (ImGui::TreeNode("Post-Processing"))
	{
		if (ImGui::TreeNode("Box Blur")) 
		{
			ImGui::SliderInt("Blur Radius ", &blurRadius, 0, 10);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Fog"))
		{
			// Turn on fog
			bool fog = false;
			if (isFog == 1) fog = true;
			ImGui::Checkbox("Distance Fog", &fog);
			if (fog) isFog = 1;
			else isFog = 0;

			// Fog settings
			float color[3] = {
					fogColor.x,
					fogColor.y,
					fogColor.z
			};
			ImGui::ColorEdit3("Color", color);
			fogColor = DirectX::XMFLOAT3(color[0], color[1], color[2]);

			ImGui::DragFloat("Start Distance", &startFog, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("Full Fog Distance", &fullFog, 0.1f, startFog, 100.0f);

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	#pragma region /* Extra UI Features from Assignment #2 */
	/*
	// --------------------------------------------
	// EXTRA FEATURES - test UI from Assignment #2
	// --------------------------------------------
	if (ImGui::TreeNode("Other Test UI Elements"))
	{
		ImGui::SeparatorText("This or That?");
		ImGui::Checkbox("This", &thisBox);
		ImGui::SameLine(); ImGui::Checkbox("That", &thatBox);
		
		if (thisBox && !thatBox) {
			ImGui::Text("Yeah, I suppose This is pretty cool.");
		}
		else if (!thisBox && thatBox) {
			ImGui::Text("I'm definitely more of a That person, myself.");
		}
		else if (thisBox && thatBox) {
			ImGui::Text("Both? I can respect it.");
		}

		ImGui::Spacing();

		ImGui::SeparatorText("A Cool List");
		const char* items[] = { 
			"The Legend of Zelda (NES)", 
			"Zelda II: The Adventure of Link (NES)", 
			"A Link to the Past (SNES)", 
			"Link's Awakening (Game Boy)", 
			"Ocarina of Time (N64)", 
			"Majora's Mask (N64)", 
			"Oracle of Ages / Oracle of Seasons (Game Boy Color)", 
			"Four Swords (Game Boy Advance)", 
			"The Wind Waker (Game Cube)", 
			"Four Swords Adventures (Game Cube)", 
			"The Minish Cap (Game Boy Advance)", 
			"Twilight Princess (Wii)", 
			"Phantom Hourglass (DS)", 
			"Link's Crossbow Training (Wii)", 
			"Spirit Tracks (DS)", 
			"Skyward Sword (Wii)", 
			"A Link Between Worlds (3DS)", 
			"Tri Force Heroes (3DS)", 
			"Breath of the Wild (Switch)", 
			"Tears of the Kingdom (Switch)"};
		static int itemIndex = 0;
		ImGui::Text("Pick your favorite Zelda game: %s", items[itemIndex]);
		ImGui::Spacing();
		if (ImGui::BeginListBox(""))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool isSelected = (itemIndex == n);
				if (ImGui::Selectable(items[n], isSelected))
					itemIndex = n;

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}

		ImGui::SeparatorText("This is a cool rainbow");
		static float values[11] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };

		for (int i = 0; i < 11; i++) {
			if (i > 0) ImGui::SameLine();
			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(i / 11.0f, 0.5f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(i / 11.0f, 0.9f, 0.9f));
			ImGui::VSliderFloat("", ImVec2(18, 160), &values[i], 0.0f, 1.0f, "");
			ImGui::PopStyleColor(2);
			ImGui::PopID();
		}

		ImGui::TreePop();
	}
	*/
	#pragma endregion

	ImGui::End();
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	// Update entities
	entities[0]->GetTransform()->Rotate(0.0f, 0.0f, 3.0f * deltaTime);
	entities[0]->GetTransform()->Scale(1.0f + 0.0005f * sinf(3.0f*totalTime), 1.0f + 0.0005f * sinf(3.0f*totalTime), 1.0f);
	entities[2]->GetTransform()->Rotate(0.0f, 0.0f, -1.0f * deltaTime);
	entities[3]->GetTransform()->MoveAbsolute(0.0003f*sinf(totalTime), 0.0f, 0.0f);
	entities[3]->GetTransform()->Rotate(0.2f * deltaTime, 0.7f * deltaTime, 0.0f);

	// cube duplicate (for testing purposes)
	//entities[6]->GetTransform()->MoveAbsolute(0.0003f * sinf(totalTime), 0.0f, 0.0f);
	//entities[6]->GetTransform()->Rotate(0.2f * deltaTime, 0.7f * deltaTime, 0.0f);

	entities[4]->GetTransform()->Scale(1.0f + 0.0001f * sinf(0.7f*totalTime),  1.0f + 0.0001f * sinf(0.7f*totalTime), 1.0f);
	if (((int)totalTime % 12) - 6 < 0)
		entities[1]->GetTransform()->MoveAbsolute(0.02f * deltaTime, 0.04f * deltaTime, 0.0f);
	else 
		entities[1]->GetTransform()->MoveAbsolute(-0.02f * deltaTime, -0.04f * deltaTime, 0.0f);

	// Update camera
	cameras[activeCameraIndex]->Update(deltaTime);

	// Refresh UI
	UpdateImGui(deltaTime, totalTime);
	BuildUI();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// ----------------------------------
	// Frame START - happens once per frame before anything else
	// ----------------------------------
	{
		// Clear the back buffer (erases what's on the screen)
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);
		context->ClearRenderTargetView(ppRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// ----------------------------------
	// Shadow Mapping
	// ----------------------------------

	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	ID3D11RenderTargetView* nullRTV{};
	context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());
	context->PSSetShader(0, 0, 0);
	context->RSSetState(shadowRasterizer.Get());

	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	VS_Shadow->SetShader();
	VS_Shadow->SetMatrix4x4("view", lightViewMatrix);
	VS_Shadow->SetMatrix4x4("projection", lightProjectionMatrix);

	// Loop and draw all entities
	for (auto& e : entities)
	{
		VS_Shadow->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
		VS_Shadow->CopyAllBufferData();
		e->GetMesh()->Draw();
	}

	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);
	context->RSSetState(0);
	context->OMSetRenderTargets(1, ppRTV.GetAddressOf(), depthBufferDSV.Get());


	// ----------------------------------
	// Draw Geometry
	// ----------------------------------

	// Call draw for each game entity
	for (int i = 0; i < entities.size(); i++) 
	{
		entities[i]->GetMaterial()->GetVertexShader()->SetMatrix4x4("lightView", lightViewMatrix);
		entities[i]->GetMaterial()->GetVertexShader()->SetMatrix4x4("lightProjection", lightProjectionMatrix);
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat3("ambient", ambientColor);
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat("numLights", (float)lights.size());
		entities[i]->GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		entities[i]->GetMaterial()->GetPixelShader()->SetShaderResourceView("ShadowMap", shadowSRV);
		entities[i]->GetMaterial()->GetPixelShader()->SetSamplerState("ShadowSampler", shadowSampler);
		entities[i]->GetMaterial()->GetPixelShader()->SetInt("fog", isFog);
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat3("fogColor", fogColor);
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat("startFog", startFog);
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat("fullFog", fullFog);
		entities[i]->Draw(context, cameras[activeCameraIndex], totalTime);
	}

	sky->Draw(cameras[activeCameraIndex]);


	// ----------------------------------
	// Frame END - happens once per frame after drawing everything
	// ----------------------------------
	{
		// Restore back buffer
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);

		// Activate shaders 
		ppVS->SetShader();
		ppPS->SetShader();
		ppPS->SetShaderResourceView("Pixels", ppSRV.Get());
		ppPS->SetSamplerState("ClampSampler", ppSampler.Get());
	
		ppPS->SetInt("blurRadius", blurRadius);
		ppPS->SetFloat("pixelWidth", 1.0f / windowWidth);
		ppPS->SetFloat("pixelHeight", 1.0f / windowHeight);

		ppPS->CopyAllBufferData();
		

		context->Draw(3, 0);

		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;

		ImGui::Render(); // Turns this frame’s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

		// Unbind shadow map
		ID3D11ShaderResourceView* nullSRVs[128] = {};
		context->PSSetShaderResources(0, 128, nullSRVs);
	}
}
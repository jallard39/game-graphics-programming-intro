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
	// Do we want a console window?  Probably only in debug mode
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
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
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
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
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
	// Load texture images
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBasic;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/Basic/Basic_albedo.png").c_str(),
		nullptr, srvBasic.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv1;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/FineClumpySand_2k/FineClumpySand_albedo.png").c_str(),
		nullptr, srv1.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/SpaceLabWallOld_2k/SpaceLabWallOld_albedo.png").c_str(),
		nullptr, srv2.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2Spec;
	CreateWICTextureFromFile(device.Get(), context.Get(),
		FixPath(L"../../Assets/Textures/SpaceLabWallOld_2k/SpaceLabWallOld_specular.png").c_str(),
		nullptr, srv2Spec.GetAddressOf());

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
	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.3f, vertexShader, pixelShader)); // White
	materials.push_back(std::make_shared<Material>(1.0f, 0.961f, 0.22f, 1.0f, 0.5f, vertexShader, pixelShader)); // Yellow
	materials.push_back(std::make_shared<Material>(0.608f, 0.945f, 1.0f, 1.0f, 0.7f, vertexShader, pixelShader)); // Blue
	materials.push_back(std::make_shared<Material>(0.169f, 0.51f, 0.161f, 1.0f, 0.9f, vertexShader, pixelShader)); // Green
	materials.push_back(std::make_shared<Material>(0.145f, 0.878f, 0.365f, 1.0f, 0.9f, vertexShader, customShaders[0])); // Rainbow
	for (int i = 0; i < 6; i++) {
		materials[i]->AddTextureSRV("SurfaceTexture", srvBasic);
		materials[i]->AddSampler("BasicSampler", sampler);
	}

	// Create textured materials
	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, vertexShader, pixelShader)); // Fine Clumpy Sand
	materials[6]->AddTextureSRV("SurfaceTexture", srv1);
	materials[6]->AddSampler("BasicSampler", sampler);

	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, vertexShader, pixelShader)); // Space Lab Wall Old
	materials[7]->AddTextureSRV("SurfaceTexture", srv2);
	materials[7]->AddTextureSRV("SpecularMap", srv2Spec);
	materials[7]->AddSampler("BasicSampler", sampler);

	materials.push_back(std::make_shared<Material>(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, vertexShader, pixelShader)); // Space Lab Scaled
	materials[8]->AddTextureSRV("SurfaceTexture", srv2);
	materials[8]->AddTextureSRV("SpecularMap", srv2Spec);
	materials[8]->AddSampler("BasicSampler", sampler);
	materials[8]->SetUVScale(10.0f, 5.0f);
}


// --------------------------------------------------------
// Creates the GameEntities that will be drawn to the screen
// --------------------------------------------------------
void Game::CreateEntities() 
{
	entities.push_back(std::make_shared<GameEntity>(meshes[1], materials[6])); // Sand cylinder
	entities[0]->GetTransform()->SetPosition(3.5f, 0.5f, 1.0f);
	entities[0]->GetTransform()->SetScale(0.5f, 0.7f, 1.0f);

	entities.push_back(std::make_shared<GameEntity>(meshes[5], materials[6])); // Sand sphere
	entities[1]->GetTransform()->SetPosition(-0.7f, -0.2f, 0.0f);
	entities[1]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[6], materials[8])); // Space wall torus
	entities[2]->GetTransform()->SetPosition(-1.0f, +1.0f, 0.0f);
	entities[2]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[0], materials[7])); // Space wall cube
	entities[3]->GetTransform()->SetPosition(+0.8f, -0.5f, 0.0f);
	entities[3]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);

	entities.push_back(std::make_shared<GameEntity>(meshes[2], materials[0])); // Red helix
	entities[4]->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
	entities[4]->GetTransform()->SetPosition(0.4f, 1.0f, 0.0f);

	entities.push_back(std::make_shared<GameEntity>(meshes[5], materials[5])); // Rainbow sphere
	entities[5]->GetTransform()->SetPosition(-1.5f, 0.0f, -1.0f);
	entities[5]->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);
}


// --------------------------------------------------------
// Creates all the lights in the scene
// --------------------------------------------------------
void Game::CreateLights()
{
	// White directional
	lights.push_back(Light{});
	lights[0].Direction = XMFLOAT3(1.0f, -1.0f, 0.0f);
	lights[0].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[0].Intensity = 1.0f;

	// Red directional
	lights.push_back(Light{});
	lights[1].Direction = XMFLOAT3(-1.0f, -0.3f, 1.0f);
	lights[1].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[1].Intensity = 0.5f;

	// Yellow directional
	lights.push_back(Light{});
	lights[2].Direction = XMFLOAT3(-0.2f, -0.8f, -2.0f);
	lights[2].Color = XMFLOAT3(1.0f, 1.0f, 0.0f);
	lights[2].Intensity = 1.0f;

	// Blue point
	lights.push_back(Light{});
	lights[3].Type = 1;
	lights[3].Position = XMFLOAT3(1.0f, 0.5f, -0.7f);
	lights[3].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[3].Intensity = 1.0f;
	lights[3].Range = 4.0f;

	// Green point
	lights.push_back(Light{});
	lights[4].Type = 1;
	lights[4].Position = XMFLOAT3(0.0f, 1.5f, 1.0f);
	lights[4].Color = XMFLOAT3(0.0f, 0.5f, 0.0f);
	lights[4].Intensity = 1.0f;
	lights[4].Range = 6.0f;
}


// --------------------------------------------------------
// Set up all the cameras in the scene and set initial active camera
// --------------------------------------------------------
void Game::CreateCameras()
{
	// Initialize camera field
	DirectX::XMFLOAT3 camInitPos = { 0.0f, 0.0f, -3.0f };
	cameras.push_back(std::make_shared<Camera>((float)this->windowWidth / this->windowHeight, camInitPos));

	camInitPos = { -1.29f, -0.46f, 1.06f };
	DirectX::XMFLOAT3 camInitRot = { -0.3f, 2.5f, 0.95f };
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

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	
	// ----------------------------------
	// Draw Geometry
	// ----------------------------------

	// Call draw for each game entity
	for (int i = 0; i < entities.size(); i++) 
	{
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat3("ambient", ambientColor);
		entities[i]->GetMaterial()->GetPixelShader()->SetFloat("numLights", (float)lights.size());
		entities[i]->GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		entities[i]->Draw(context, cameras[activeCameraIndex], totalTime);
	}

	// ----------------------------------
	// Frame END - happens once per frame after drawing everything
	// ----------------------------------
	{
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
	}
}
#include "GameEntity.h"
#include <d3d11.h>
#include <wrl/client.h>

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) :
	mesh(mesh),
	material(material)
{
	transform = std::make_shared<Transform>();
}

GameEntity::~GameEntity()
{

}

// ======================
// GETTERS
// ======================

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> GameEntity::GetTransform()
{
	return transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial()
{
	return material;
}

DirectX::XMFLOAT4 GameEntity::GetColorTint()
{
	return colorTint;
}

// ======================
// SETTERS
// ======================

void GameEntity::SetMaterial(std::shared_ptr<Material> material)
{
	this->material = material;
}

void GameEntity::SetColorTint(DirectX::XMFLOAT4 color)
{
	colorTint = color;
}

void GameEntity::SetColorTint(float r, float g, float b, float a)
{
	colorTint = { r, g, b, a };
}

void GameEntity::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	std::shared_ptr<Camera> camera,
	float totalTime
	)
{
	// Set shaders
	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();

	// Prepare materials
	material->PrepareMaterial();

	// Set up data for vertex shader
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();

	vs->SetMatrix4x4("world", transform->GetWorldMatrix());
	vs->SetMatrix4x4("view", camera->GetViewMatrix());
	vs->SetMatrix4x4("projection", camera->GetProjectionMatrix());
	vs->SetMatrix4x4("worldInvTranspose", transform->GetWorldInverseTransposeMatrix());

	// Set up data for pixel shader
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();

	ps->SetFloat4("colorTint", material->GetColorTint());
	ps->SetFloat("totalTime", totalTime);
	ps->SetFloat("roughness", material->GetRoughness());
	ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());

	// Map/MemCopy/Unmap
	vs->CopyAllBufferData();
	ps->CopyAllBufferData();

	// Set vertex & index buffers and render
	mesh->Draw();
}
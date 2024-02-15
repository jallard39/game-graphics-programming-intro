#include "GameEntity.h"
#include <d3d11.h>
#include <wrl/client.h>
#include "BufferStructs.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh) :
	mesh(mesh)
{
	transform = std::make_shared<Transform>();
}

GameEntity::~GameEntity()
{

}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> GameEntity::GetTransform()
{
	return transform;
}

DirectX::XMFLOAT4 GameEntity::GetColorTint()
{
	return colorTint;
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
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer
	)
{
	// Collect data for current entity and store in a struct
	VertexShaderData vsData;
	vsData.colorTint = colorTint;
	DirectX::XMFLOAT4X4 worldMat = transform->GetWorldMatrix();
	vsData.world = DirectX::XMLoadFloat4x4(&worldMat);

	// Map / Memcpy / Unmap Constant Buffer resource
	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
	context->Unmap(vsConstantBuffer.Get(), 0);

	// Set vertex & index buffers and render
	mesh->Draw();
}
#pragma once

#include "Transform.h"
#include "Mesh.h"
#include <memory>

class GameEntity {

public:
	GameEntity(std::shared_ptr<Mesh> mesh);
	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();

	DirectX::XMFLOAT4 GetColorTint();
	void SetColorTint(DirectX::XMFLOAT4 color);
	void SetColorTint(float r, float g, float b, float a);

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer
	);

private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
	DirectX::XMFLOAT4 colorTint = { 1.0f, 1.0f, 1.0f, 1.0f };
};
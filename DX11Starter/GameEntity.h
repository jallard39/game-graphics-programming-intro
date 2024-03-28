#pragma once

#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include "Camera.h"
#include "Material.h"

class GameEntity {

public:
	GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	std::shared_ptr<Material> GetMaterial();

	DirectX::XMFLOAT4 GetColorTint();

	void SetMaterial(std::shared_ptr<Material> material);
	void SetColorTint(DirectX::XMFLOAT4 color);
	void SetColorTint(float r, float g, float b, float a);

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<Camera> camera,
		float totalTime
	);

private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
	std::shared_ptr<Material> material;

	DirectX::XMFLOAT4 colorTint = { 1.0f, 1.0f, 1.0f, 1.0f };
};
#pragma once

#include "Input.h"
#include "Transform.h"
#include <DirectXMath.h>
#include <memory>

class Camera {

public:

	Camera(float aspectRatio, DirectX::XMFLOAT3 initPos);
	Camera(float aspectRatio, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 orientation, float fov);
	Camera(float aspectRatio, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 orientation, float fov, float nearClip, float farClip, float moveSpeed, float lookSpeed, bool isOrtho);
	~Camera();

	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	DirectX::XMFLOAT4X4 GetViewMatrix();
	std::shared_ptr<Transform> GetTransform();
	float GetFOV();

	void UpdateProjectionMatrix(float aspectRatio);
	void UpdateViewMatrix();

	void Update(float dt);

private:

	std::shared_ptr<Transform> transform;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;

	float fov;
	float nearClipDistance;
	float farClipDistance;
	float movementSpeed;
	float mouseLookSpeed;
	bool isOrthographic;
};
#pragma once
#define _USE_MATH_DEFINES

#include "Camera.h"
#include <cmath>
#include <algorithm>

Camera::Camera(float aspectRatio, DirectX::XMFLOAT3 initPos)
{
	transform = std::make_shared<Transform>();
	transform->SetPosition(initPos);

	fov = (float)M_PI / 3;
	nearClipDistance = 0.01f;
	farClipDistance = 50.0f;
	movementSpeed = 1.0f;
	mouseLookSpeed = 0.002f;
	isOrthographic = false;

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

Camera::Camera(float aspectRatio, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 orientation, float fov) :
	fov(fov)
{
	nearClipDistance = 0.01f;
	farClipDistance = 50.0f;
	movementSpeed = 1.0f;
	mouseLookSpeed = 0.002f;
	isOrthographic = false;

	transform = std::make_shared<Transform>();
	transform->SetRotation(orientation);
	transform->SetPosition(initPos);

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

Camera::Camera(float aspectRatio, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 orientation, float fov, float nearClip, float farClip, float moveSpeed, float lookSpeed, bool isOrtho) :
	fov(fov),
	nearClipDistance(nearClip),
	farClipDistance(farClip),
	movementSpeed(moveSpeed),
	mouseLookSpeed(lookSpeed),
	isOrthographic(isOrtho)
{
	transform = std::make_shared<Transform>();
	transform->SetPosition(initPos);
	transform->SetRotation(orientation);

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

Camera::~Camera() 
{

}


void Camera::UpdateProjectionMatrix(float aspectRatio)
{
	DirectX::XMMATRIX projectionMat = DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearClipDistance, farClipDistance);
	DirectX::XMStoreFloat4x4(&projection, projectionMat);
}

void Camera::UpdateViewMatrix()
{
	DirectX::XMFLOAT3 transformPos = transform->GetPosition();
	DirectX::XMFLOAT3 transformForward = transform->GetForward();

	DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&transformPos);
	DirectX::XMVECTOR forwardVec = DirectX::XMLoadFloat3(&transformForward);
	DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	DirectX::XMMATRIX viewMat = DirectX::XMMatrixLookToLH(posVec, forwardVec, upVec);
	DirectX::XMStoreFloat4x4(&view, viewMat);
}

void Camera::Update(float dt)
{
	Input& input = Input::GetInstance();

	if (input.KeyDown('W'))
	{
		transform->MoveRelative(0.0f, 0.0f, movementSpeed * dt);
	}
	if (input.KeyDown('S'))
	{
		transform->MoveRelative(0.0f, 0.0f, -movementSpeed * dt);
	}
	if (input.KeyDown('A'))
	{
		transform->MoveRelative(-movementSpeed * dt, 0.0f, 0.0f);
	}
	if (input.KeyDown('D'))
	{
		transform->MoveRelative(movementSpeed * dt, 0.0f, 0.0f);
	}
	if (input.KeyDown(VK_SPACE))
	{
		transform->MoveAbsolute(0.0f, movementSpeed * dt, 0.0f);
	}
	if (input.KeyDown(VK_SHIFT))
	{
		transform->MoveAbsolute(0.0f, -movementSpeed * dt, 0.0f);
	}

	if (input.MouseLeftDown())
	{
		float cursorMovementX = input.GetMouseXDelta() * mouseLookSpeed;
		float cursorMovementY = input.GetMouseYDelta() * mouseLookSpeed;

		transform->Rotate(0.0f, cursorMovementX, 0.0f);
		// clamp() & max() & min() is not working for some reason so I'll just do it the long way
		if (cursorMovementY > (float)M_PI / 2.0f) cursorMovementY = (float)M_PI / 2.0f;
		else if (cursorMovementY < -1 * (float)M_PI / 2.0f) cursorMovementY = -1 * (float)M_PI / 2.0f;
		transform->Rotate(cursorMovementY, 0.0f, 0.0f);
	}

	UpdateViewMatrix();
}


// ----------------------
// GETTERS
// ----------------------

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix() 
{
	return projection;
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return view;
}

std::shared_ptr<Transform> Camera::GetTransform()
{
	return transform;
}

float Camera::GetFOV() {
	return fov;
}

float Camera::GetFarClipDistance() {
	return farClipDistance;
}
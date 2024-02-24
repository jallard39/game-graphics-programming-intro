#pragma once

#include "Transform.h"

Transform::Transform() 
{
	position = { 0.0f, 0.0f, 0.0f };
	rotation = { 0.0f, 0.0f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };

	right = { 1.0f, 0.0f, 0.0f };
	up = { 0.0f, 1.0f, 0.0f };
	forward = { 0.0f, 0.0f, 1.0f };
	
	DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&worldInverseTranspose, DirectX::XMMatrixIdentity());
}

Transform::~Transform() 
{

}

void Transform::CalculateWorldMatrices()
{
	DirectX::XMMATRIX translationMat = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX rotationMat = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	DirectX::XMMATRIX worldMat = scaleMat * rotationMat * translationMat;
	DirectX::XMStoreFloat4x4(&world, worldMat);
	DirectX::XMStoreFloat4x4(&worldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));
}

void Transform::UpdateRightUpForward()
{
	DirectX::XMVECTOR rotVec = DirectX::XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	
	DirectX::XMVECTOR worldRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMStoreFloat3(&right, DirectX::XMVector3Rotate(worldRight, rotVec));

	DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	DirectX::XMStoreFloat3(&up, DirectX::XMVector3Rotate(worldUp, rotVec));

	DirectX::XMVECTOR worldForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
	DirectX::XMStoreFloat3(&forward, DirectX::XMVector3Rotate(worldForward, rotVec));
}


// ----------------------
// SETTERS
// ----------------------

void Transform::SetPosition(float x, float y, float z)
{
	position = { x, y, z };
}

void Transform::SetPosition(DirectX::XMFLOAT3 position)
{
	this->position = position;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	rotation = { pitch, yaw, roll };
	UpdateRightUpForward();
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	this->rotation = rotation;
	UpdateRightUpForward();
}

void Transform::SetScale(float x, float y, float z)
{
	scale = { x, y, z };
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	this->scale = scale;
}


// ----------------------
// GETTERS
// ----------------------

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return rotation;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix() 
{
	CalculateWorldMatrices();
	return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	CalculateWorldMatrices();
	return worldInverseTranspose;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
	return right;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
	return up;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
	return forward;
}



// ----------------------
// TRANSFORMERS
// ----------------------

void Transform::MoveAbsolute(float x, float y, float z)
{
	DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR offsetVec = DirectX::XMVectorSet(x, y, z, 0.0f);
	posVec = DirectX::XMVectorAdd(posVec, offsetVec);
	DirectX::XMStoreFloat3(&position, posVec);
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR offsetVec = DirectX::XMLoadFloat3(&offset);
	posVec = DirectX::XMVectorAdd(posVec, offsetVec);
	DirectX::XMStoreFloat3(&position, posVec);
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	DirectX::XMVECTOR rotVec = DirectX::XMLoadFloat3(&rotation);
	DirectX::XMVECTOR offsetVec = DirectX::XMVectorSet(pitch, yaw, roll, 0.0f);
	rotVec = DirectX::XMVectorAdd(rotVec, offsetVec);
	DirectX::XMStoreFloat3(&rotation, rotVec);

	UpdateRightUpForward();
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	DirectX::XMVECTOR rotVec = DirectX::XMLoadFloat3(&(this->rotation));
	DirectX::XMVECTOR offsetVec = DirectX::XMLoadFloat3(&rotation);
	rotVec = DirectX::XMVectorAdd(rotVec, offsetVec);
	DirectX::XMStoreFloat3(&(this->rotation), rotVec);

	UpdateRightUpForward();
}

void Transform::Scale(float x, float y, float z)
{
	DirectX::XMVECTOR scaleVec = DirectX::XMLoadFloat3(&scale);
	DirectX::XMVECTOR offsetVec = DirectX::XMVectorSet(x, y, z, 1.0f);
	scaleVec = DirectX::XMVectorMultiply(scaleVec, offsetVec);
	DirectX::XMStoreFloat3(&scale, scaleVec);
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	DirectX::XMVECTOR scaleVec = DirectX::XMLoadFloat3(&(this->scale));
	DirectX::XMVECTOR offsetVec = DirectX::XMLoadFloat3(&scale);
	scaleVec = DirectX::XMVectorMultiply(scaleVec, offsetVec);
	DirectX::XMStoreFloat3(&(this->scale), scaleVec);
}

void Transform::MoveRelative(float x, float y, float z) 
{
	DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR absOffsetVec = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR rotVec = DirectX::XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	DirectX::XMVECTOR relOffsetVec = DirectX::XMVector3Rotate(absOffsetVec, rotVec);
	posVec = DirectX::XMVectorAdd(posVec, relOffsetVec);
	DirectX::XMStoreFloat3(&position, posVec);
}
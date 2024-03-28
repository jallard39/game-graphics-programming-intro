#pragma once

#include <memory>
#include "SimpleShader.h"
#include <DirectXMath.h>
#include <unordered_map>
#include <wrl/client.h>

class Material {

public:

	Material(
		DirectX::XMFLOAT4 colorTint, 
		float roughness,
		std::shared_ptr<SimpleVertexShader> vertexShader, 
		std::shared_ptr<SimplePixelShader> pixelShader
	);
	Material(
		float r, float g, float b, float a,
		float roughness,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader
	);
	~Material();

	DirectX::XMFLOAT4 GetColorTint();
	float GetRoughness();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT2 GetUVScale();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetColorTint(float r, float g, float b, float a);
	void SetRoughness(float x);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetUVOffset(float x, float y);
	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVScale(float x, float y);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);

	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);
	void PrepareMaterial();

private:

	DirectX::XMFLOAT4 colorTint;
	float roughness;
	DirectX::XMFLOAT2 uvOffset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 uvScale = { 1.0f, 1.0f };

	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;

};
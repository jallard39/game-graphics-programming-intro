#include "Material.h"

Material::Material(
	DirectX::XMFLOAT4 colorTint,
	float roughness,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader
) {
	this->colorTint = colorTint;
	this->roughness = roughness;
	this->vertexShader = vertexShader;
	this->pixelShader = pixelShader;
}

Material::Material(
	float r, float g, float b, float a,
	float roughness,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader
) {
	this->roughness = roughness;
	this->colorTint = { r, g, b, a };
	this->vertexShader = vertexShader;
	this->pixelShader = pixelShader;
}

Material::~Material()
{

}

// ======================
// GETTERS
// ======================

DirectX::XMFLOAT4 Material::GetColorTint()
{
	return colorTint;
}

float Material::GetRoughness() 
{
	return roughness;
}

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader()
{
	return vertexShader;
}

std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
	return pixelShader;
}


// ======================
// SETTERS
// ======================

void Material::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
	this->colorTint = colorTint;
}

void Material::SetColorTint(float r, float g, float b, float a)
{
	this->colorTint = { r, g, b, a };
}

void Material::SetRoughness(float x)
{
	this->roughness = x;
}

void Material::SetUVOffset(DirectX::XMFLOAT2 offset)
{
	uvOffset = offset;
}

void Material::SetUVOffset(float x, float y)
{
	uvOffset = { x, y };
}

void Material::SetUVScale(DirectX::XMFLOAT2 scale)
{
	uvScale = scale;
}

void Material::SetUVScale(float x, float y)
{
	uvScale = { x, y };
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader)
{
	this->vertexShader = vertexShader;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
{
	this->pixelShader = pixelShader;
}

// ======================
// Other Methods
// ======================

void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	textureSRVs.insert({ name, srv });
}

void Material::AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
	samplers.insert({ name, sampler });
}

void Material::PrepareMaterial()
{
	pixelShader->SetFloat2("uvOffset", uvOffset);
	pixelShader->SetFloat2("uvScale", uvScale);
	for (auto& t : textureSRVs) { pixelShader->SetShaderResourceView(t.first.c_str(), t.second); }
	for (auto& s : samplers) { pixelShader->SetSamplerState(s.first.c_str(), s.second); }
}
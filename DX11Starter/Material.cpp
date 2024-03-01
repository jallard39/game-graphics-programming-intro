#include "Material.h"

Material::Material(
	DirectX::XMFLOAT4 colorTint,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader
) {
	this->colorTint = colorTint;
	this->vertexShader = vertexShader;
	this->pixelShader = pixelShader;
}

Material::Material(
	float r, float g, float b, float a,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader
) {
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

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader)
{
	this->vertexShader = vertexShader;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
{
	this->pixelShader = pixelShader;
}


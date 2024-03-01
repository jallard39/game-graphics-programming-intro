#pragma once

#include <memory>
#include "SimpleShader.h"
#include <DirectXMath.h>

class Material {

public:

	Material(
		DirectX::XMFLOAT4 colorTint, 
		std::shared_ptr<SimpleVertexShader> vertexShader, 
		std::shared_ptr<SimplePixelShader> pixelShader
	);
	Material(
		float r, float g, float b, float a,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader
	);
	~Material();

	DirectX::XMFLOAT4 GetColorTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetColorTint(float r, float g, float b, float a);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);

private:

	DirectX::XMFLOAT4 colorTint;

	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;

};
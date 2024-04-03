#pragma once

#include <wrl/client.h>
#include "SimpleShader.h"
#include <memory>
#include "Mesh.h"
#include "Camera.h"

class Sky {

public: 

	Sky(
		std::shared_ptr<Mesh> mesh, 
		Microsoft::WRL::ComPtr<ID3D11SamplerState> skySampler, 
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
	~Sky();

	void Draw(std::shared_ptr<Camera> camera);

private: 

	Microsoft::WRL::ComPtr<ID3D11SamplerState> skySampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeMap;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencil;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	std::shared_ptr<Mesh> mesh;

	std::shared_ptr<SimplePixelShader> ps;
	std::shared_ptr<SimpleVertexShader> vs;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

};
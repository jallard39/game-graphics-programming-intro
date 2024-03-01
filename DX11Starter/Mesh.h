#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>

#include "Vertex.h"

class Mesh {
	
public:
	Mesh(
		std::string name,
		Vertex* vertices, 
		int numVertices,
		unsigned int* indices,
		int numIndices,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context
	);
	Mesh(
		std::string name,
		const wchar_t* filename,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context
	);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	int GetIndexCount();
	std::string GetName();
	void Draw();

private:
	std::string name = "MyMesh";

	int numVertices;
	int numIndices;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;

	void CreateBuffers(
		const void* vertices, 
		int numVertices, 
		const void* indices, 
		int numIndices, 
		Microsoft::WRL::ComPtr<ID3D11Device> device
	);
};
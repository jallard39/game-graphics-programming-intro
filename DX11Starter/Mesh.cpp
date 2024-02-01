#include "Mesh.h"
#include <Windows.h>

Mesh::Mesh(std::string name, Vertex* vertices, int numVertices, unsigned int* indices, int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context) :
	name(name),
	numVertices(numVertices),
	numIndices(numIndices),
	context(context)
{
	// Create a Vertex Buffer
	{
		D3D11_BUFFER_DESC vbd = {};
		vbd.Usage = D3D11_USAGE_IMMUTABLE;	
		vbd.ByteWidth = sizeof(Vertex) * numVertices;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		// Create the proper struct to hold the initial vertex data
		D3D11_SUBRESOURCE_DATA initialVertexData = {};
		initialVertexData.pSysMem = vertices;

		// Actually create the buffer on the GPU with the initial data
		device->CreateBuffer(&vbd, &initialVertexData, vertexBuffer.GetAddressOf());
	}

	// Create an Index Buffer 
	{
		D3D11_BUFFER_DESC ibd = {};
		ibd.Usage = D3D11_USAGE_IMMUTABLE;	
		ibd.ByteWidth = sizeof(unsigned int) * numIndices;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;	
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		// Specify the initial data for this buffer, similar to above
		D3D11_SUBRESOURCE_DATA initialIndexData = {};
		initialIndexData.pSysMem = indices;

		// Actually create the buffer with the initial data
		device->CreateBuffer(&ibd, &initialIndexData, indexBuffer.GetAddressOf());
	}
}


Mesh::~Mesh() 
{
	// Empty for now
}


Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer() 
{
	return vertexBuffer;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
	return indexBuffer;
}

int Mesh::GetIndexCount() 
{
	return numIndices;
}

std::string Mesh::GetName()
{
	return name;
}


void Mesh::Draw()
{
	// DRAW geometry
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set buffers in the input assembler (IA) stage
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Tell Direct3D to draw
	context->DrawIndexed(
		numIndices, // The number of indices to use (we could draw a subset if we wanted)
		0,			// Offset to the first index we want to use
		0);			// Offset to add to each index when looking up vertices
}
#pragma once

#include <DirectXMath.h>

// --------------------------------------------------------
// A custom vertex definition
//
// You will eventually ADD TO this, and/or make more of these!
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The local position of the vertex 
	DirectX::XMFLOAT3 Normal;		// The normal of the face
	DirectX::XMFLOAT2 UV;			// The uv coordinates of the texture
};
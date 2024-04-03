#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix view;
    matrix projection;
}

VertexToPixel_Sky main( VertexShaderInput input ) 
{
    VertexToPixel_Sky output;
    
    // Zero out translation in view matrix
    matrix updatedView = view;
    updatedView._14 = 0;
    updatedView._24 = 0;
    updatedView._34 = 0;
    
    output.position = mul(projection, mul(updatedView, float4(input.localPosition, 1.0f)));
    output.position.z = output.position.w;
    output.sampleDir = input.localPosition;
    
    return output;
}
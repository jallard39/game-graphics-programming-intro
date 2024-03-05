#include "ShaderIncludes.hlsli"

// Struct representing data from a constant buffer
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float roughness;
    float3 cameraPosition;
    float3 ambient;
    float numLights;
    Light lights[5];
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    float4 ambientTerm = float4(ambient.rgb, 1.0f);
    
    float3 v = normalize(cameraPosition - input.worldPosition);
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    
    float3 totalLight = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < numLights; i++)
    {
        if (lights[i].Type == 0)
            totalLight += DirectionalLight(input.normal, lights[i], v, specExponent, colorTint.rgb);
        else if (lights[i].Type == 1)
            totalLight += PointLight(input.normal, lights[i], v, specExponent, colorTint.rgb, input.worldPosition);
    }   
    
    return (colorTint * ambientTerm) + float4(totalLight, 1.0f);
}
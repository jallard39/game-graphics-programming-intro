#include "ShaderIncludes.hlsli"

Texture2D SurfaceTexture : register(t0);
Texture2D SpecularMap : register(t1);
SamplerState BasicSampler : register(s0);

// Struct representing data from a constant buffer
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float roughness;
    float3 cameraPosition;
    float3 ambient;
    float numLights;
    float2 uvOffset;
    float2 uvScale;
    Light lights[5];
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    
    // Scale UVs
    float2 uv = input.uv + uvOffset;
    uv = float2(uv.x * uvScale.x, uv.y * uvScale.y);
    
    // Sample texture
    float4 surfaceColor = SurfaceTexture.Sample(BasicSampler, uv) * colorTint;
    float specMapValue = SpecularMap.Sample(BasicSampler, uv).b;
    
    // Ambient lighting -  same for all
    float4 ambientTerm = float4(ambient.rgb, 1.0f);
    
    // Specular exponent - same for all
    float3 v = normalize(cameraPosition - input.worldPosition);
    float specExponent = (1.0f - roughness) * specMapValue * MAX_SPECULAR_EXPONENT;
    
    float3 totalLight = float3(0.0f, 0.0f, 0.0f);
    
    // Calculate total lighting
    for (int i = 0; i < numLights; i++)
    {
        if (lights[i].Type == 0)
            totalLight += DirectionalLight(input.normal, lights[i], v, specExponent, surfaceColor.rgb);
        else if (lights[i].Type == 1)
            totalLight += PointLight(input.normal, lights[i], v, specExponent, surfaceColor.rgb, input.worldPosition);
    }   
    
    return (surfaceColor * ambientTerm) + float4(totalLight, 1.0f);
}
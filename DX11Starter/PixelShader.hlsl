#include "ShaderIncludes.hlsli"

Texture2D Albedo : register(t0);
Texture2D RoughnessMap : register(t1);
Texture2D MetalnessMap : register(t2);

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
    
    // Sample textures
    float4 surfaceColor = Albedo.Sample(BasicSampler, uv) * colorTint;
    surfaceColor = float4(pow(surfaceColor.rgb, 2.2f), 1.0f);
    
    float roughness = RoughnessMap.Sample(BasicSampler, uv).r;
    float metalness = 0.0f;
    
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    // ====== Lighting ======
    
    // Ambient, specular, direction -  same for all
    float3 v = normalize(cameraPosition - input.worldPosition);
    
    // Calculate total lighting
    float3 totalLight = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numLights; i++)
    {
        if (lights[i].Type == 0)
            totalLight += DirectionalLight(input.normal, lights[i], v, roughness, surfaceColor.rgb, specularColor, metalness);
        else if (lights[i].Type == 1)
            totalLight += PointLight(input.normal, lights[i], v, roughness, surfaceColor.rgb, specularColor, input.worldPosition, metalness);
    }
    
    //return float4(input.normal, 1.0f);
    return float4(pow(totalLight, 1.0f / 2.2f), 1.0f);
}
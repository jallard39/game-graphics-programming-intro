#include "ShaderIncludes.hlsli"

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);

Texture2D ShadowMap : register(t4); 
Texture2D Ramp : register(t5);

SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

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
    float3 fogColor;
    float startFog;
    float fullFog;
    int fog;
    //int cel;
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// --------------------------------------------------------
float4 main(VertexToPixel_NormalMap input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    // ====== Shadows ======
    
    // Perspective divide
    input.shadowMapPos /= input.shadowMapPos.w;
    
    // Convert the normalized device coordinates to UVs for sampling
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    
    // Compare light-to-pixel distance and closest-surface distance
    float distToLight = input.shadowMapPos.z;
    float shadowAmount = ShadowMap.SampleCmpLevelZero(
        ShadowSampler, shadowUV, distToLight).r;
    
    
    // ====== Sampling ======
    
    // Scale UVs
    float2 uv = input.uv + uvOffset;
    uv = float2(uv.x * uvScale.x, uv.y * uvScale.y);
    
    // Sample textures
    float4 surfaceColor = Albedo.Sample(BasicSampler, uv) * colorTint;
    surfaceColor = float4(pow(surfaceColor.rgb, 2.2f), 1.0f);
    
    float roughness = RoughnessMap.Sample(BasicSampler, uv).r;
    float metalness = MetalnessMap.Sample(BasicSampler, uv).r;
    
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    
    // ====== Normals ======
    
    // Unpack normals
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, uv).rgb * 2 - 1;
    unpackedNormal = normalize(unpackedNormal);
    
    // Calculate TBN rotation matrix
    float3 tangent = normalize(input.tangent - input.normal * dot(input.tangent, input.normal));
    float3 bitangent = cross(tangent, input.normal);
    float3x3 TBN = float3x3(input.tangent, bitangent, input.normal);
    input.normal = mul(unpackedNormal, TBN);
    

    // ====== Lighting ======

    // Ambient, specular, direction -  same for all
    float3 v = normalize(cameraPosition - input.worldPosition);
    
    // Calculate total lighting
    float3 totalLight = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numLights; i++)
    {
        /*
            if (cel == 1)
        {
            float3 lightDir = -1 * normalize(lights[i].Direction);
            float diffuse = DiffusePBR(input.normal, lightDir);
            float2 rampUV = float2(diffuse, 0.5);
            float celDiffuse = Ramp.Sample(BasicSampler, rampUV).r;
            float3 lightResult = celDiffuse * surfaceColor.rgb * lights[i].Intensity;
            if (i == 0)
                lightResult *= shadowAmount;
            totalLight += lightResult;
        } */
        if (lights[i].Type == 0)
        {
            float3 lightResult = DirectionalLight(input.normal, lights[i], v, roughness, surfaceColor.rgb, specularColor, metalness);
            if (i == 0)
                lightResult *= shadowAmount;
            totalLight += lightResult;
        }
        else if (lights[i].Type == 1)
        {
            totalLight += PointLight(input.normal, lights[i], v, roughness, surfaceColor.rgb, specularColor, input.worldPosition, metalness);
        }    
    }
    
    float4 pixelColor = float4(pow(totalLight, 1.0f / 2.2f), 1.0f);
    
    if (fog == 1)
    {
        float dist = length(cameraPosition - input.worldPosition);
        float fogAmt = smoothstep(startFog, fullFog, dist);
        float3 finalColor = lerp(pixelColor.rgb, fogColor, fogAmt);
        return float4(finalColor, 1.0f);

    }
    else
    {
        return pixelColor;
    }
}
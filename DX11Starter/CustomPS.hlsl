#include "ShaderIncludes.hlsli"

// Struct representing data from a constant buffer
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float totalTime;
    float3 cameraPosition;
    float3 fogColor;
    float startFog;
    float fullFog;
    int fog;
}


// The entry point (main method) for our pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{    
    float speed = 0.01;
    float timer = ((totalTime + 6) - (input.uv.y * 6)) % 6;
    float brightness = 0.9;
    
    float r = (float(timer > 5) * (timer - 4)) + (float(timer <= 3)) - float(timer > 2) * saturate(timer - 2);
    float g = float(timer <= 5) * float(timer >= 1) * saturate(timer - 1) - float(timer > 4) * saturate(timer - 4);
    float b = float(timer <= 1) + (float(timer >= 3) * saturate(timer - 3)) - float(timer < 1) * saturate(timer + 0);
    
    float4 pixelColor = brightness * float4(r, g, b, 1.0);
    
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
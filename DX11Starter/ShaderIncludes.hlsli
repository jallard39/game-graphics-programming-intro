#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__

struct VertexShaderInput
{
    float3 localPosition : POSITION; // XYZ position
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

// Struct representing the data we expect to receive from earlier pipeline stages
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION;
};

struct VertexToPixel_NormalMap
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION;
    float3 tangent : TANGENT;
};

struct VertexToPixel_Sky
{
    float4 position : SV_Position;
    float3 sampleDir : DIRECTION;
};

#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

#define MAX_SPECULAR_EXPONENT   256.0f

struct Light
{
    int Type;
    float3 Direction;
    float Range;
    float3 Position;
    float Intensity;
    float3 Color;
    float SpotFalloff;
    float3 Padding;
};

// Equation for diffuse calculations
float Lambert(float3 normal, float3 lightDirection)
{
    return saturate(dot(normal, -lightDirection));
}

// Equation for specular calculations
float Phong(float3 normal, float3 view, float3 lightDirection, float specularPower)
{
    float spec = 0.0f;
    if (specularPower > 0.05f)
    {
        float3 r = reflect(lightDirection, normal);
        spec = pow(saturate(dot(r, view)), specularPower);
    }
    return spec;
}

// Calculate attenuation of point lights
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

// Complete lighting equation for directional lights
float3 DirectionalLight(float3 normal, Light light, float3 viewVector, float specularPower, float3 surfaceColor)
{
    float3 lightDir = normalize(light.Direction);
    
    float3 diffuse = Lambert(normal, lightDir) * light.Color * light.Intensity * surfaceColor;
    float spec = Phong(normal, viewVector, lightDir, specularPower);
    spec *= any(diffuse);
    
    return diffuse + spec.xxx;
}

// Complete lighting equation for point lights
float3 PointLight(float3 normal, Light light, float3 viewVector, float specularPower, float3 surfaceColor, float3 worldPos)
{
    float3 lightDir = normalize(worldPos - light.Position);
    
    float3 diffuse = Lambert(normal, lightDir) * light.Color * light.Intensity * surfaceColor;
    float spec = Phong(normal, viewVector, lightDir, specularPower);
    spec *= any(diffuse);
    
    return (diffuse + spec.xxx) * Attenuate(light, worldPos);

}

#endif
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
    float4 shadowMapPos : SHADOW_POSITION;
};

struct VertexToPixel_NormalMap
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION;
    float3 tangent : TANGENT;
    float4 shadowMapPos : SHADOW_POSITION;
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

// A constant Fresnel value for non-metals (glass and plastic have values of about 0.04)
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

static const float PI = 3.14159265359f;

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


// PBR FUNCTIONS ================

// Calculates diffuse amount based on energy conservation
//
// diffuse   - Diffuse amount
// F         - Fresnel result from microfacet BRDF
// metalness - surface metalness amount 
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}
 

// Normal Distribution Function: GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
	// Pre-calculations
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	// Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}


// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
    float VdotH = saturate(dot(v, h));

	// Final value
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}


// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
//
// n - Normal
// v - View vector
//
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
//
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
	// End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));

	// Final value
    return 1 / (NdotV * (1 - k) + k);
}

 
// Cook-Torrance Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
	// Other vectors
    float3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
	// Pass F out of the function for diffuse balance
    F_out = F;

	// Final specular formula
    float3 specularResult = (D * F * G) / 4;
    return specularResult * max(dot(n, l), 0);
}

// Equation for diffuse calculations
float DiffusePBR(float3 normal, float3 lightDirection)
{
    return saturate(dot(normal, lightDirection));
}

// Calculate attenuation of point lights
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

// Calculates total light given a diffuse value
float3 SpecAndTotal(
    float3 normal, 
    float3 lightDir, 
    float diffuse, 
    float lightIntensity, 
    float3 lightColor, 
    float3 viewVector, 
    float roughness, 
    float3 surfaceColor, 
    float3 specularColor, 
    float metalness
) {
    float3 F;
    float3 spec = MicrofacetBRDF(normal, lightDir, viewVector, roughness, specularColor, F);
    
    float3 balancedDiff = DiffuseEnergyConserve(float3(diffuse, diffuse, diffuse), F, metalness);
    
    float3 total = (balancedDiff * surfaceColor + spec) * lightIntensity * lightColor;
    
    return total;
}

// Complete lighting equation for directional lights
float3 DirectionalLight(float3 normal, Light light, float3 viewVector, float roughness, float3 surfaceColor, float3 specularColor, float metalness)
{
    float3 lightDir = -1 * normalize(light.Direction);
    
    float diffuse = DiffusePBR(normal, lightDir);
    
    return SpecAndTotal(normal, lightDir, diffuse, light.Intensity, light.Color, viewVector, roughness, surfaceColor, specularColor, metalness);
}

// Complete lighting equation for point lights
float3 PointLight(float3 normal, Light light, float3 viewVector, float roughness, float3 surfaceColor, float3 specularColor, float3 worldPos, float metalness)
{
    float3 lightDir = normalize(light.Position - worldPos);
    float diffuse = DiffusePBR(normal, lightDir);
    
    float3 total = SpecAndTotal(normal, lightDir, diffuse, light.Intensity, light.Color, viewVector, roughness, surfaceColor, specularColor, metalness);
    
    return total * Attenuate(light, worldPos);
}

#endif
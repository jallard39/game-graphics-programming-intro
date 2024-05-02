struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D Pixels : register(t0);
SamplerState ClampSampler : register(s0);

cbuffer ExternalData : register(b0)
{
    int blurRadius;
    float pixelWidth;
    float pixelHeight;
}

float4 main(VertexToPixel input) : SV_TARGET
{
    // Track the total color and number of samples
    float4 total = 0;
    int sampleCount = 0;
    //int blurRadius = 5;
    //float pixelWidth = 1.0f / 1280.0f;
    //float pixelHeight = 1.0f / 720.0f;
    
    // Loop through the "box"
    for (int x = -blurRadius; x <= blurRadius; x++)
    {
        for (int y = -blurRadius; y <= blurRadius; y++)
        {
            // Calculate the UV for this sample
            float2 uv = input.uv;
            uv += float2(x * pixelWidth, y * pixelHeight);
            
            // Add this color to the running total
            total += Pixels.Sample(ClampSampler, uv);
            sampleCount++;
        }
    }
    
    // Return the average 
    return total / sampleCount;
}
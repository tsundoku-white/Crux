struct VSInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 color    : COLOR0;
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 pos    : SV_Position;
    float2 uv     : TEXCOORD0;
    float3 color  : COLOR0;
    float3 normal : NORMAL;
    nointerpolation uint textureIndex : TEXCOORD1;
    float3 worldPos : TEXCOORD2; // Added for correct tiling
};

// camera, once per frame
struct UBO
{
    float4x4 projection;
    float4x4 view;
};

// one element per object
struct SSBO
{
    float4x4 model;
    uint     textureIndex;
};

// set 0: per-frame data
[[vk::binding(0, 0)]] ConstantBuffer<UBO>     ubo;
[[vk::binding(1, 0)]] StructuredBuffer<SSBO>  ssbo;

// set 1: every texture in one array (bindless)
[[vk::binding(0, 1)]] Sampler2D textures[];

[shader("vertex")]
VSOutput vertexMain(VSInput input,
                    uint instanceID : SV_InstanceID,
                    uint baseInstance : SV_StartInstanceLocation)
{
    VSOutput output;
    SSBO obj = ssbo[baseInstance + instanceID];

    float4 worldPos = mul(obj.model, float4(input.position, 1.0));
    output.pos      = mul(ubo.projection, mul(ubo.view, worldPos));
    output.worldPos = worldPos.xyz;

    // Pass UVs and normal through directly
    output.uv     = input.uv; 
    output.color  = input.color;
    output.normal = mul((float3x3)obj.model, input.normal);
    output.textureIndex = obj.textureIndex;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target
{
    float3 N    = normalize(input.normal);
    float3 L    = normalize(float3(0.5, 1.0, -0.3));
    float ndotl = max(dot(N, L), 0.0);

    // --- TRIPLANAR TILING ---
    // Determine how much the normal points in each axis direction
    float3 blend = abs(N);
    blend = blend / (blend.x + blend.y + blend.z); // Normalize weights

    // Sample the texture 3 times, projecting from X, Y, and Z world axes.
    // The scale is 1.0, meaning 1 texture tile = 1 world unit.
    float2 uvX = input.worldPos.zy;
    float2 uvY = input.worldPos.xz;
    float2 uvZ = input.worldPos.xy;

    Sampler2D texSampler = textures[NonUniformResourceIndex(input.textureIndex)];
    
    float4 texX = texSampler.Sample(uvX);
    float4 texY = texSampler.Sample(uvY);
    float4 texZ = texSampler.Sample(uvZ);

    // Blend the 3 projections based on the face normal
    float4 tex = (texX * blend.x) + (texY * blend.y) + (texZ * blend.z);

    float3 finalColor = tex.rgb * input.color * (ndotl * 0.8 + 0.2);
    return float4(finalColor, tex.a);
}

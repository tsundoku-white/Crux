struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 dir : TEXCOORD0;
    nointerpolation uint textureIndex : TEXCOORD1;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
};

struct SSBO
{
    float4x4 model;
    uint     textureIndex;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO>    ubo;
[[vk::binding(1, 0)]] StructuredBuffer<SSBO> ssbo;
[[vk::binding(0, 1)]] Sampler2D              textures[];

[shader("vertex")]
VSOutput vertexMain(VSInput input,
                    uint instanceID   : SV_InstanceID,
                    uint baseInstance : SV_StartInstanceLocation)
{
    VSOutput output;
    SSBO obj = ssbo[baseInstance + instanceID];

    // rotation only: upper-left 3x3 drops the camera translation
    float3 viewPos = mul((float3x3)ubo.view, input.position);
    float4 clip    = mul(ubo.projection, float4(viewPos, 1.0));

    output.pos          = clip.xyww; // depth is always 1.0
    output.dir          = input.position;
    output.textureIndex = obj.textureIndex;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target
{
    float3 d  = normalize(input.dir);
    float2 uv = float2(atan2(d.z, d.x), asin(d.y)) * float2(0.1591549, 0.3183099) + 0.5;

    Sampler2D tex = textures[NonUniformResourceIndex(input.textureIndex)];
    float3 c = tex.SampleLevel(uv, 0).rgb; // SampleLevel avoids the seam line at the atan2 wrap

    c = c / (c + 1.0); // Reinhard tonemap
    // c = pow(c, 1.0 / 2.2); // only if the swapchain is NOT sRGB

    return float4(c, 1.0);
}

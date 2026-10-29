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
};

struct SceneData
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};

ConstantBuffer<SceneData> SceneCB : register(b0);

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    
    float4 worldPos = mul(SceneCB.model, float4(input.position, 1.0));
    output.pos      = mul(SceneCB.projection, mul(SceneCB.view, worldPos));
    output.uv       = input.uv;
    output.color    = input.color;    
    output.normal   = mul((float3x3)SceneCB.model, input.normal);
    
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 L = normalize(float3(0.5, 1.0, -0.3));
    float ndotl = max(dot(N, L), 0.0);
    
    float3 finalColor = input.color * (ndotl * 0.8 + 0.2);
    
    return float4(finalColor, 1.0);
}


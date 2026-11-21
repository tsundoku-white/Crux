struct VSInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 color    : COLOR0;
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 pos      : SV_Position;
    float2 uv       : TEXCOORD0;
    float3 color    : COLOR0;
    float3 normal   : NORMAL;
    float3 worldPos : TEXCOORD2;
    nointerpolation uint3  texIndex  : TEXCOORD1; // x = base, y = normal, z = roughness
    nointerpolation float3 baseColor : TEXCOORD3;
    float3 objPos    : TEXCOORD4; // local position * object scale (texture space)
    float3 objNormal : TEXCOORD5; // local space normal
    nointerpolation float3 axisX : TEXCOORD6; // object axes in world space (unit length)
    nointerpolation float3 axisY : TEXCOORD7;
    nointerpolation float3 axisZ : TEXCOORD8;
};

// camera, once per frame
struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4   cameraPos;
    uint     debugView; // 0 lit, 1 normal map, 2 roughness map, 3 albedo
};

// one element per object
struct SSBO
{
    float4x4 model;
    float4   baseColor;
    uint     textureIndex; // base color
    uint     normalIndex;
    uint     roughnessIndex;
    uint     pad;
};

// texture tiles per world unit. 1.0 = one tile per unit of object size.
static const float TILE = 1.0;

static const uint DEBUG_LIT       = 0;
static const uint DEBUG_NORMAL    = 1;
static const uint DEBUG_ROUGHNESS = 2;
static const uint DEBUG_ALBEDO    = 3;

[[vk::binding(0, 0)]] ConstantBuffer<UBO>     ubo;
[[vk::binding(1, 0)]] StructuredBuffer<SSBO>  ssbo;
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

    output.uv        = input.uv;
    output.color     = input.color;
    output.normal    = mul((float3x3)obj.model, input.normal);
    output.texIndex  = uint3(obj.textureIndex, obj.normalIndex, obj.roughnessIndex);
    output.baseColor = obj.baseColor.rgb;

    // object axes in world space. mul() is used so this works with either matrix layout
    float3 ax = mul(obj.model, float4(1, 0, 0, 0)).xyz;
    float3 ay = mul(obj.model, float4(0, 1, 0, 0)).xyz;
    float3 az = mul(obj.model, float4(0, 0, 1, 0)).xyz;
    float3 scale = max(float3(length(ax), length(ay), length(az)), float3(1e-6));

    output.axisX = ax / scale.x;
    output.axisY = ay / scale.y;
    output.axisZ = az / scale.z;

    // local position times scale: follows the object's rotation and position,
    // but keeps texel density constant, so scaling the object repeats the texture
    output.objPos    = input.position * scale * TILE;
    output.objNormal = input.normal;
    return output;
}

// blend the 3 object-axis projections of a texture
float4 sampleTriplanar(Sampler2D tex, float3 wp, float3 w)
{
    return tex.Sample(wp.zy) * w.x +
           tex.Sample(wp.xz) * w.y +
           tex.Sample(wp.xy) * w.z;
}

// triplanar normal mapping, "whiteout" blend. No tangents needed.
// returns an object space normal
float3 sampleTriplanarNormal(Sampler2D tex, float3 wp, float3 N, float3 w)
{
    float3 tX = tex.Sample(wp.zy).xyz * 2.0 - 1.0;
    float3 tY = tex.Sample(wp.xz).xyz * 2.0 - 1.0;
    float3 tZ = tex.Sample(wp.xy).xyz * 2.0 - 1.0;

    tX = float3(tX.xy + N.zy, abs(tX.z) * N.x);
    tY = float3(tY.xy + N.xz, abs(tY.z) * N.y);
    tZ = float3(tZ.xy + N.xy, abs(tZ.z) * N.z);

    return normalize(tX.zyx * w.x + tY.xzy * w.y + tZ.xyz * w.z);
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target
{
    float3 N  = normalize(input.normal);     // world, for lighting
    float3 No = normalize(input.objNormal);  // object, for texture projection
    float3 w  = abs(No);
    w = w / (w.x + w.y + w.z);
    float3 p  = input.objPos;

    Sampler2D baseTex  = textures[NonUniformResourceIndex(input.texIndex.x)];
    Sampler2D normTex  = textures[NonUniformResourceIndex(input.texIndex.y)];
    Sampler2D roughTex = textures[NonUniformResourceIndex(input.texIndex.z)];

    // debug views (ubo.debugView is uniform, so the early outs are safe)
    if (ubo.debugView == DEBUG_NORMAL)
    {
        float3 n = sampleTriplanarNormal(normTex, p, No, w);
        n = normalize(n.x * input.axisX + n.y * input.axisY + n.z * input.axisZ);
        return float4(n * 0.5 + 0.5, 1.0);
    }
    if (ubo.debugView == DEBUG_ROUGHNESS)
    {
        float r = sampleTriplanar(roughTex, p, w).r;
        return float4(r, r, r, 1.0);
    }

    float4 base   = sampleTriplanar(baseTex, p, w);
    float3 albedo = base.rgb * input.baseColor * input.color;

    if (ubo.debugView == DEBUG_ALBEDO)
        return float4(albedo, 1.0);

    float3 nLocal = sampleTriplanarNormal(normTex, p, No, w);
    float3 Nm     = normalize(nLocal.x * input.axisX + nLocal.y * input.axisY + nLocal.z * input.axisZ);
    float  rough  = saturate(sampleTriplanar(roughTex, p, w).r);

    float3 L     = normalize(float3(0.5, 1.0, -0.3));
    float3 V     = normalize(ubo.cameraPos.xyz - input.worldPos);
    float3 H     = normalize(L + V);
    float  ndotl = max(dot(Nm, L), 0.0);

    float gloss = 1.0 - rough;
    float spec  = pow(max(dot(Nm, H), 0.0), lerp(4.0, 128.0, gloss)) * gloss * ndotl;

    float3 finalColor = albedo * (ndotl * 0.8 + 0.2) + spec * 0.5;
    return float4(finalColor, base.a);
}

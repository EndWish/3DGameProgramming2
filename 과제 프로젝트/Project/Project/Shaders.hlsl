cbuffer cbCameraInfo : register(b1) {
	matrix view : packoffset(c0);
	matrix projection : packoffset(c4);
	float3 cameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2) {
	matrix worldTransform : packoffset(c0);
};

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

Texture2D gtxtAlbedoTexture : register(t6);
Texture2D gtxtSpecularTexture : register(t7);
Texture2D gtxtNormalTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtEmissionTexture : register(t10);
Texture2D gtxtDetailAlbedoTexture : register(t11);
Texture2D gtxtDetailNormalTexture : register(t12);

SamplerState gssWrap : register(s0);
SamplerState gssClamp : register(s1);

#include "Light.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Basic

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VS_OUTPUT DefaultVertexShader(VS_INPUT input)
{	
	VS_OUTPUT output;

	output.normal = mul(input.normal, (float3x3)worldTransform);
	output.normal = normalize(output.normal);

	// 조명 계산을 위해 월드좌표내에서의 포지션값을 계산해 따로 저장
	output.positionW = (float3)mul(float4(input.position, 1.0f), worldTransform);

	output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
    output.uv = input.uv;
	return output;
}

[earlydepthstencil]
float4 DefaultPixelShader(VS_OUTPUT input) : SV_TARGET
{
    float4 albedoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specularColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 normalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 metallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 emissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	if (textureType & MATERIAL_ALBEDO_MAP) 
        albedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
	if (textureType & MATERIAL_SPECULAR_MAP) 
        specularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_NORMAL_MAP)
        normalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_METALLIC_MAP)
        metallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_EMISSION_MAP)
        emissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);
	
    float4 color = albedoColor + specularColor + emissionColor;
    
    float4 cIllumination = float4(1.0f, 1.0f, 1.0f, 1.0f);
    cIllumination = CalculateLight(input.positionW, input.normal);
    
    //color = lerp(color, cIllumination, 0.5f);
    color.rgb = lerp(color.rgb, cIllumination.rgb, 0.5f);
    
	return color;
}

///////////////////////////////////////////////////////////////////////////////
/// AlphaBlending

// 매개변수 구조체는 Basic과 공유한다.
VS_OUTPUT AlphaBlendingVertexShader(VS_INPUT input)
{
    VS_OUTPUT output;

    output.normal = mul(input.normal, (float3x3) worldTransform);
    output.normal = normalize(output.normal);

	// 조명 계산을 위해 월드좌표내에서의 포지션값을 계산해 따로 저장
    output.positionW = (float3) mul(float4(input.position, 1.0f), worldTransform);

    output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
    output.uv = input.uv;
    return output;
}

float4 AlphaBlendingPixelShader(VS_OUTPUT input) : SV_TARGET
{
    float4 albedoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specularColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 normalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 metallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 emissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    if (textureType & MATERIAL_ALBEDO_MAP) 
        albedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_SPECULAR_MAP) 
        specularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_NORMAL_MAP)
        normalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_METALLIC_MAP)
        metallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_EMISSION_MAP)
        emissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);
	
    float4 color = albedoColor + specularColor + emissionColor;
    
    float4 cIllumination = float4(1.0f, 1.0f, 1.0f, 1.0f);
    cIllumination = CalculateLight(input.positionW, input.normal);
    
    //color = lerp(color, cIllumination, 0.5f);
    color.rgb = lerp(color.rgb, cIllumination.rgb, 0.5f);
    
    return color;
}

///////////////////////////////////////////////////////////////////////////////
/// HitBox

struct VS_HITBOX_INPUT {
    float3 position : POSITION;
};

struct VS_HITBOX_OUTPUT {
    float4 position : SV_POSITION;
};


VS_HITBOX_OUTPUT HitboxVertexShader(VS_HITBOX_INPUT input) {
    VS_HITBOX_OUTPUT output;
    output.position = mul(mul(mul(float4(input.position, 1.0f), worldTransform), view), projection);
    return output;
}

[earlydepthstencil]
float4 HitboxPixelShader(VS_HITBOX_OUTPUT input) : SV_TARGET {
    float4 color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    return color;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain

struct VS_TERRAIN_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
};

VS_TERRAIN_OUTPUT TerrainVertexShader(VS_TERRAIN_INPUT input)
{
    VS_TERRAIN_OUTPUT output;

    output.normal = mul(input.normal, (float3x3) worldTransform);
    output.normal = normalize(output.normal);

	// 조명 계산을 위해 월드좌표내에서의 포지션값을 계산해 따로 저장
    output.positionW = (float3) mul(float4(input.position, 1.0f), worldTransform);

    output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
    output.uv = input.uv;
    output.uv2 = input.uv2;
    

    
    return output;
}

[earlydepthstencil]
float4 TerrainPixelShader(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
    float4 albedoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specularColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 normalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 metallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 emissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 detailAlbedoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    if (textureType & MATERIAL_ALBEDO_MAP) 
        albedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_SPECULAR_MAP) 
        specularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_NORMAL_MAP)
        normalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_METALLIC_MAP)
        metallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_EMISSION_MAP)
        emissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_DETAIL_ALBEDO_MAP)
        detailAlbedoColor = gtxtDetailAlbedoTexture.Sample(gssWrap, input.uv2);
	
    float4 color = albedoColor + specularColor + emissionColor + detailAlbedoColor;
    
    float4 cIllumination = float4(1.0f, 1.0f, 1.0f, 1.0f);
    cIllumination = CalculateLight(input.positionW, input.normal);
    
    //color = lerp(color, cIllumination, 0.5f);
    color.rgb = lerp(color.rgb, cIllumination.rgb, 0.5f);
    
    return color;
} 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BillBoard

struct VS_BILLBOARD_INPUT
{
    float3 position : POSITION;
    float2 boardSize : BOARDSIZE;
};

struct VS_BILLBOARD_OUTPUT
{
    float3 positionW : POSITION;
    float2 boardSize : BOARDSIZE;
    float3 normal : NORMAL;
};

struct GS_BILLBOARD_OUTPUT
{
    float3 positionW : POSITION;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VS_BILLBOARD_OUTPUT BillBoardVertexShader(VS_BILLBOARD_INPUT input)
{
    VS_BILLBOARD_OUTPUT output;
    
    output.positionW = (float3) mul(float4(input.position, 1.0f), worldTransform);
    
    output.boardSize = input.boardSize;

    output.normal = cameraPosition - output.positionW;
    output.normal = normalize(output.normal);
    
    return output;
}

[maxvertexcount(4)]
void BillBoardGeometryShader(point VS_BILLBOARD_OUTPUT input[1], inout TriangleStream<GS_BILLBOARD_OUTPUT> outStream)
{
    GS_BILLBOARD_OUTPUT output;
    output.normal = input[0].normal;
    
    float3 rightVector = normalize(cross(float3(0, 1, 0), input[0].normal)); // Y축과 look벡터 외적해서 rightVector를 얻는다.
    float3 upVector = normalize(cross(input[0].normal, rightVector)); //
    
    float3 dxOffset = rightVector * input[0].boardSize.x / 2.0f;
    float3 dyOffset = upVector * input[0].boardSize.y / 2.0f;
    
     // 시계방향이 앞쪽
    float3 dx[4] = { -dxOffset, dxOffset, -dxOffset, dxOffset };
    float3 dy[4] = { -dyOffset, -dyOffset, dyOffset, dyOffset };
    float2 uv[4] = { float2(1, 1 - 0), float2(0, 1 - 0), float2(1, 1 - 1), float2(0, 1 - 1) };
    for (int i = 0; i < 4; ++i)
    {
        output.positionW = input[0].positionW + dx[i] + dy[i];
        output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
        output.uv = uv[i];
        outStream.Append(output);
    }
}

float4 BillBoardPixelShader(GS_BILLBOARD_OUTPUT input) : SV_TARGET
{
    float4 albedoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specularColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 normalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 metallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 emissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    if (textureType & MATERIAL_ALBEDO_MAP) 
        albedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_SPECULAR_MAP) 
        specularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_NORMAL_MAP)
        normalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_METALLIC_MAP)
        metallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    if (textureType & MATERIAL_EMISSION_MAP)
        emissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);
	
    float4 color = albedoColor + specularColor + emissionColor;
    
    float4 cIllumination = float4(0.0f, 0.0f, 0.0f, 0.0f);
    cIllumination = CalculateLight(input.positionW, input.normal);
    
    color.rgb = lerp(color.rgb, cIllumination.rgb, 0.5f);
    if (color.a < 0.1f)
        discard;
    return color; 
}



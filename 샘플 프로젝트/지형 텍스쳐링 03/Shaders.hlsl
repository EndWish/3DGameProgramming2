cbuffer cbPlayerInfo : register(b0)
{
	matrix		gmtxPlayerWorld : packoffset(c0);
};

cbuffer cbCameraInfo : register(b1)
{
	matrix		gmtxView : packoffset(c0);
	matrix		gmtxProjection : packoffset(c4);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix		gmtxGameObject : packoffset(c0);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_DIFFUSED_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VS_DIFFUSED_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VS_DIFFUSED_OUTPUT VSPlayer(VS_DIFFUSED_INPUT input)
{
	VS_DIFFUSED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxPlayerWorld), gmtxView), gmtxProjection);
	output.color = input.color;

	return(output);
}

float4 PSPlayer(VS_DIFFUSED_OUTPUT input) : SV_TARGET
{
	return(input.color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D gtxtTexture : register(t0);
SamplerState gSamplerState : register(s0);

struct VS_TEXTURED_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input)
{
	VS_TEXTURED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}

float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTexture.Sample(gSamplerState, input.uv);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D<float4> gtxtTerrainBaseTexture : register(t1);
Texture2D<float4> gtxtTerrainDetailTextures[3] : register(t2); //t2, t3, t4
//Texture2D<float> gtxtTerrainAlphaTexture : register(t5);
Texture2D<float4> gtxtTerrainAlphaTexture : register(t5);

struct VS_TERRAIN_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.color = input.color;
	output.uv0 = input.uv0;
	output.uv1 = input.uv1;

	return(output);
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
	float4 cBaseTexColor = gtxtTerrainBaseTexture.Sample(gSamplerState, input.uv0);
//	float fAlpha = gtxtTerrainAlphaTexture.Sample(gSamplerState, input.uv0);

	float4 cDetailTexColors[3];
	cDetailTexColors[0] = gtxtTerrainDetailTextures[0].Sample(gSamplerState, input.uv1 * 2.0f);
	cDetailTexColors[1] = gtxtTerrainDetailTextures[1].Sample(gSamplerState, input.uv1 * 0.125f);
	cDetailTexColors[2] = gtxtTerrainDetailTextures[2].Sample(gSamplerState, input.uv1);

	float4 cColor = cBaseTexColor * cDetailTexColors[0];
	float fAlpha = gtxtTerrainAlphaTexture.Sample(gSamplerState, input.uv0).w;
	cColor += lerp(cDetailTexColors[1] * 0.25f, cDetailTexColors[2], 1.0f - fAlpha);
/* 
	cColor = lerp(cDetailTexColors[0], cDetailTexColors[2], 1.0f - fAlpha) ;
	cColor = lerp(cBaseTexColor, cColor, 0.3f) + cDetailTexColors[1] * (1.0f - fAlpha);
*/
/*
	if (fAlpha < 0.35f) cColor = cDetailTexColors[2];
	else if (fAlpha > 0.8975f) cColor = cDetailTexColors[0];
	else cColor = cDetailTexColors[1];
*/
	return(cColor);
}



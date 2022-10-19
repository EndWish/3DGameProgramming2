cbuffer cbPlayerInfo : register(b0)
{
	matrix		gmtxPlayerWorld : packoffset(c0);
};

cbuffer cbCameraInfo : register(b1)
{
	matrix		gmtxView : packoffset(c0);
	matrix		gmtxProjection : packoffset(c4);
	float3		gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix		gmtxGameObject : packoffset(c0);
	uint		gnObjectID : packoffset(c4);
	uint		gnMaterialID : packoffset(c8);
};

#include "Light.hlsl"

Texture2DArray gtxtTextureArray : register(t0);
SamplerState gssDefaultSamplerState : register(s0);

cbuffer cbDrawOptions : register(b5)
{
	int4 gvDrawOptions : packoffset(c0);
};

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

VS_DIFFUSED_OUTPUT VSDiffused(VS_DIFFUSED_INPUT input)
{
	VS_DIFFUSED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.color = input.color;

	return(output);
}

float4 PSDiffused(VS_DIFFUSED_OUTPUT input): SV_TARGET
{
	return(input.color);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
VS_DIFFUSED_OUTPUT VSPlayer(VS_DIFFUSED_INPUT input)
{
	VS_DIFFUSED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxPlayerWorld), gmtxView), gmtxProjection);
	output.color = input.color;

	return(output);
}

float4 PSPlayer(VS_DIFFUSED_OUTPUT input): SV_TARGET
{
	return(input.color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
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

float4 PSTextured(VS_TEXTURED_OUTPUT input, uint nPrimitiveID : SV_PrimitiveID): SV_TARGET
{
	float3 uvw = float3(input.uv, nPrimitiveID / 2);
	float4 cColor = gtxtTextureArray.Sample(gssDefaultSamplerState, uvw);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_TEXTURED_LIGHTING_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VS_TEXTURED_LIGHTING_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normalW : NORMAL;
	float2 uv : TEXCOORD;
};

VS_TEXTURED_LIGHTING_OUTPUT VSTexturedLighting(VS_TEXTURED_LIGHTING_INPUT input)
{
	VS_TEXTURED_LIGHTING_OUTPUT output;

	output.normalW = mul(input.normal, (float3x3)gmtxGameObject);
	output.positionW = (float3)mul(float4(input.position, 1.0f), gmtxGameObject);
	output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}

float4 PSTexturedLighting(VS_TEXTURED_LIGHTING_OUTPUT input, uint nPrimitiveID : SV_PrimitiveID): SV_TARGET
{
	float3 uvw = float3(input.uv, nPrimitiveID / 2);
	float4 cColor = gtxtTextureArray.Sample(gssDefaultSamplerState, uvw);
	input.normalW = normalize(input.normalW);
	float4 cIllumination = Lighting(input.positionW, input.normalW);

	return(cColor * cIllumination);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct PS_MULTIPLE_RENDER_TARGETS_OUTPUT
{
	float4 color : SV_TARGET0;

	float4 cTexture : SV_TARGET1;
	float4 cIllumination : SV_TARGET2;
	float4 normal : SV_TARGET3;
	float zDepth : SV_TARGET4;
};

PS_MULTIPLE_RENDER_TARGETS_OUTPUT PSTexturedLightingToMultipleRTs(VS_TEXTURED_LIGHTING_OUTPUT input, uint nPrimitiveID : SV_PrimitiveID)
{
	PS_MULTIPLE_RENDER_TARGETS_OUTPUT output;

	float3 uvw = float3(input.uv, nPrimitiveID / 2);
	output.cTexture = gtxtTextureArray.Sample(gssDefaultSamplerState, uvw);

	input.normalW = normalize(input.normalW);
	output.cIllumination = Lighting(input.positionW, input.normalW);

	output.color = output.cIllumination * output.cTexture;

	output.normal = float4(input.normalW.xyz * 0.5f + 0.5f, 1.0f);

	output.zDepth = input.position.z;

	return(output);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
float4 VSPostProcessing(uint nVertexID : SV_VertexID): SV_POSITION
{
	if (nVertexID == 0)	return(float4(-1.0f, +1.0f, 0.0f, 1.0f));
	if (nVertexID == 1)	return(float4(+1.0f, +1.0f, 0.0f, 1.0f));
	if (nVertexID == 2)	return(float4(+1.0f, -1.0f, 0.0f, 1.0f));

	if (nVertexID == 3)	return(float4(-1.0f, +1.0f, 0.0f, 1.0f));
	if (nVertexID == 4)	return(float4(+1.0f, -1.0f, 0.0f, 1.0f));
	if (nVertexID == 5)	return(float4(-1.0f, -1.0f, 0.0f, 1.0f));

	return(float4(0, 0, 0, 0));
}

float4 PSPostProcessing(float4 position : SV_POSITION): SV_Target
{
	return(float4(0.0f, 0.0f, 0.0f, 1.0f));
}

///////////////////////////////////////////////////////////////////////////////
//
struct VS_SCREEN_RECT_TEXTURED_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_SCREEN_RECT_TEXTURED_OUTPUT VSScreenRectSamplingTextured(uint nVertexID : SV_VertexID)
{
	VS_SCREEN_RECT_TEXTURED_OUTPUT output = (VS_TEXTURED_OUTPUT)0;

	if (nVertexID == 0) { output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f); output.uv = float2(0.0f, 0.0f); }
	else if (nVertexID == 1) { output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f); output.uv = float2(1.0f, 0.0f); }
	else if (nVertexID == 2) { output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f); output.uv = float2(1.0f, 1.0f); }
							 
	else if (nVertexID == 3) { output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f); output.uv = float2(0.0f, 0.0f); }
	else if (nVertexID == 4) { output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f); output.uv = float2(1.0f, 1.0f); }
	else if (nVertexID == 5) { output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f); output.uv = float2(0.0f, 1.0f); }

	return(output);
}

Texture2D<float4> gtxtTextureTexture : register(t1);
Texture2D<float4> gtxtIlluminationTexture : register(t2);
Texture2D<float4> gtxtNormalTexture : register(t3);

Texture2D<float> gtxtzDepthTexture : register(t4);
Texture2D<float> gtxtDepthTexture : register(t5);

float4 GetColorFromDepth(float fDepth)
{
	float4 cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	if (fDepth >= 1.0f) cColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
	else if (fDepth < 0.00625f) cColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
	else if (fDepth < 0.0125f) cColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
	else if (fDepth < 0.025f) cColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
	else if (fDepth < 0.05f) cColor = float4(1.0f, 1.0f, 0.0f, 1.0f);
	else if (fDepth < 0.075f) cColor = float4(0.0f, 1.0f, 1.0f, 1.0f);
	else if (fDepth < 0.1f) cColor = float4(1.0f, 0.5f, 0.5f, 1.0f);
	else if (fDepth < 0.4f) cColor = float4(0.5f, 1.0f, 1.0f, 1.0f);
	else if (fDepth < 0.6f) cColor = float4(1.0f, 0.0f, 1.0f, 1.0f);
	else if (fDepth < 0.8f) cColor = float4(0.5f, 0.5f, 1.0f, 1.0f);
	else if (fDepth < 0.9f) cColor = float4(0.5f, 1.0f, 0.5f, 1.0f);
	else cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	return(cColor);
}

float4 PSScreenRectSamplingTextured(VS_TEXTURED_OUTPUT input): SV_Target
{
	float4 cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	switch (gvDrawOptions.x)
	{
		case 84: //'T'
		{
			cColor = gtxtTextureTexture.Sample(gssDefaultSamplerState, input.uv);
			break;
		}
		case 76: //'L'
		{
			cColor = gtxtIlluminationTexture.Sample(gssDefaultSamplerState, input.uv);
			break;
		}
		case 78: //'N'
		{
			cColor = gtxtNormalTexture.Sample(gssDefaultSamplerState, input.uv);
			break;
		}
		case 68: //'D'
		{
			float fDepth = gtxtDepthTexture.Load(uint3((uint)input.position.x, (uint)input.position.y, 0));
			cColor = fDepth;
//			cColor = GetColorFromDepth(fDepth);
			break;
		}
		case 90: //'Z'
		{
			float fzDepth = gtxtzDepthTexture.Load(uint3((uint)input.position.x, (uint)input.position.y, 0));
			cColor = fzDepth;
//			cColor = GetColorFromDepth(fDepth);
			break;
		}
	}
	return(cColor);
}

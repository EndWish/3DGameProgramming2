cbuffer cbCameraInfo : register(b1) {
	matrix view : packoffset(c0);
	matrix projection : packoffset(c4);
	float3 cameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2) {
	matrix worldTransform : packoffset(c0);
};

cbuffer cbFrameworkInfo : register(b5)
{
    float currentTime : packoffset(c0.x);
    float elapsedTime : packoffset(c0.y);
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

struct OBJColors
{
    float4 diffuse;
    float4 specular;
    float4 emission;
};

float4 CalculateLight2(float3 _Position, float3 _Normal, OBJColors _objColors);
float4 DirectionalLight2(int _nIndex, float3 _normal, float3 _toCamera, OBJColors _objColors);
float4 PointLight2(int _nIndex, float3 _position, float3 _normal, float3 _toCamera, OBJColors _objColors);
float4 SpotLight2(int _nIndex, float3 _position, float3 _normal, float3 _toCamera, OBJColors _objColors);

float3 RandomDirection(float seedOffset);
float RandomFloat(float2 co);

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

	// ���� ����� ���� ������ǥ�������� �����ǰ��� ����� ���� ����
	output.positionW = (float3)mul(float4(input.position, 1.0f), worldTransform);

	output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
    output.uv = input.uv;
	return output;
}

[earlydepthstencil]
float4 DefaultPixelShader(VS_OUTPUT input) : SV_TARGET
{
    OBJColors objColors;
    objColors.diffuse = textureType & MATERIAL_ALBEDO_MAP ? gtxtAlbedoTexture.Sample(gssWrap, input.uv) * materialDiffuse : materialDiffuse;
    objColors.specular = textureType & MATERIAL_SPECULAR_MAP ? gtxtSpecularTexture.Sample(gssWrap, input.uv) * materialSpecular : materialSpecular;
    objColors.emission = textureType & MATERIAL_EMISSION_MAP ? gtxtEmissionTexture.Sample(gssWrap, input.uv) * materialEmissive : materialEmissive;
    
    float4 textureNormal = textureType & MATERIAL_NORMAL_MAP ? gtxtNormalTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 textureMetallic = textureType & MATERIAL_METALLIC_MAP ? gtxtMetallicTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    return CalculateLight2(input.positionW, input.normal, objColors);
}

///////////////////////////////////////////////////////////////////////////////
/// AlphaBlending

// �Ű����� ����ü�� Basic�� �����Ѵ�.
VS_OUTPUT AlphaBlendingVertexShader(VS_INPUT input)
{
    VS_OUTPUT output;

    output.normal = mul(input.normal, (float3x3) worldTransform);
    output.normal = normalize(output.normal);

	// ���� ����� ���� ������ǥ�������� �����ǰ��� ����� ���� ����
    output.positionW = (float3) mul(float4(input.position, 1.0f), worldTransform);

    output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
    output.uv = input.uv;
    return output;
}

float4 AlphaBlendingPixelShader(VS_OUTPUT input) : SV_TARGET
{
    OBJColors objColors;
    objColors.diffuse = textureType & MATERIAL_ALBEDO_MAP ? gtxtAlbedoTexture.Sample(gssWrap, input.uv) * materialDiffuse : materialDiffuse;
    objColors.specular = textureType & MATERIAL_SPECULAR_MAP ? gtxtSpecularTexture.Sample(gssWrap, input.uv) * materialSpecular : materialSpecular;
    objColors.emission = textureType & MATERIAL_EMISSION_MAP ? gtxtEmissionTexture.Sample(gssWrap, input.uv) * materialEmissive : materialEmissive;
    
    float4 textureNormal = textureType & MATERIAL_NORMAL_MAP ? gtxtNormalTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 textureMetallic = textureType & MATERIAL_METALLIC_MAP ? gtxtMetallicTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    return CalculateLight2(input.positionW, input.normal, objColors);
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

	// ���� ����� ���� ������ǥ�������� �����ǰ��� ����� ���� ����
    output.positionW = (float3) mul(float4(input.position, 1.0f), worldTransform);

    output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
    output.uv = input.uv;
    output.uv2 = input.uv2;
    

    
    return output;
}

[earlydepthstencil]
float4 TerrainPixelShader(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
    OBJColors objColors;
    objColors.diffuse = textureType & MATERIAL_ALBEDO_MAP ? gtxtAlbedoTexture.Sample(gssWrap, input.uv) * materialDiffuse : materialDiffuse;
    objColors.specular = textureType & MATERIAL_SPECULAR_MAP ? gtxtSpecularTexture.Sample(gssWrap, input.uv) * materialSpecular : materialSpecular;
    objColors.emission = textureType & MATERIAL_EMISSION_MAP ? gtxtEmissionTexture.Sample(gssWrap, input.uv) * materialEmissive : materialEmissive;
    
    float4 textureNormal = textureType & MATERIAL_NORMAL_MAP ? gtxtNormalTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 textureMetallic = textureType & MATERIAL_METALLIC_MAP ? gtxtMetallicTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
	
    if (textureType & MATERIAL_DETAIL_ALBEDO_MAP)
        objColors.diffuse += gtxtDetailAlbedoTexture.Sample(gssWrap, input.uv2);
	
    return CalculateLight2(input.positionW, input.normal, objColors);
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
    
    float3 rightVector = normalize(cross(float3(0, 1, 0), input[0].normal)); // Y��� look���� �����ؼ� rightVector�� ��´�.
    float3 upVector = normalize(cross(input[0].normal, rightVector)); //
    
    float3 dxOffset = rightVector * input[0].boardSize.x / 2.0f;
    float3 dyOffset = upVector * input[0].boardSize.y / 2.0f;
    
     // �ð������ ����
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
    OBJColors objColors;
    objColors.diffuse = textureType & MATERIAL_ALBEDO_MAP ? gtxtAlbedoTexture.Sample(gssWrap, input.uv) * materialDiffuse : materialDiffuse;
    objColors.specular = textureType & MATERIAL_SPECULAR_MAP ? gtxtSpecularTexture.Sample(gssWrap, input.uv) * materialSpecular : materialSpecular;
    objColors.emission = textureType & MATERIAL_EMISSION_MAP ? gtxtEmissionTexture.Sample(gssWrap, input.uv) * materialEmissive : materialEmissive;
    
    float4 textureNormal = textureType & MATERIAL_NORMAL_MAP ? gtxtNormalTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 textureMetallic = textureType & MATERIAL_METALLIC_MAP ? gtxtMetallicTexture.Sample(gssWrap, input.uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float4 color;
    if (textureType & MATERIAL_NORMAL_MAP)  // ��ָ��� ���� ���
        color = CalculateLight2(input.positionW, input.normal, objColors);
    else    // ��ָ��� ���� ���
    {
        color = objColors.diffuse + objColors.specular + objColors.emission;
        color.a = objColors.diffuse.a;
    }
    
    if (color.a < 0.1f)
        discard;
    return color; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ParticleStreamOut
#define GRAVITY 98

#define PARTICLES_TEXTURE_ROW 5
#define PARTICLES_TEXTURE_COLUMN 5

#define PARTICLE_TYPE_WRECK		0
#define PARTICLE_TYPE_SPARK		1

#define SPARK_LIFETIME 2.f
#define SPARK_BOARDSIZE float2(8.0, 8.0)

struct VS_PARTICLE_INPUT
{
    float3 position : POSITION;
    float3 velocity : VELOCITY;
    float2 boardSize : BOARDSIZE;
    float lifetime : LIFETIME;
    uint type : PARTICLETYPE;
};

VS_PARTICLE_INPUT ParticleStreamOutVertexShader(VS_PARTICLE_INPUT input)
{
    return input;
}

void WreckParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> outStream);
void SparkParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> outStream);



[maxvertexcount(20)] // sizeof(VS_PARTICLE_INPUT) �� maxvertexcount �� ���ؼ� 1024����Ʈ�� ������ �ȵȴ�.
void ParticleStreamOutGeometryShader(point VS_PARTICLE_INPUT input[1], inout PointStream<VS_PARTICLE_INPUT> outStream)
{
    VS_PARTICLE_INPUT particle = input[0];
    if (particle.type == PARTICLE_TYPE_WRECK)
    {
        WreckParticles(particle, outStream);
    }
    else if (particle.type == PARTICLE_TYPE_SPARK)
    {
        SparkParticles(particle, outStream);
    }
        
    
}

void WreckParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> outStream)
{
    // ������ ���� ������ �̵���Ű��(�߷��� ������ �޵��� �Ѵ�.), 
    if (input.lifetime > 0.0f)  // ������ �������� ���
    {
        input.lifetime -= elapsedTime;
        input.position += input.velocity * elapsedTime;
        input.velocity.y += -GRAVITY * elapsedTime; // �߷°��ӵ��� �ð��� ���ؼ� y�ӵ��� �����ش�.
        outStream.Append(input);
    }
    else    // ������ ������ ���
    {
        //������ ���ϸ� ���ο� ��ƼŬ ���� �߰��Ѵ�.
        VS_PARTICLE_INPUT newParticle;
        newParticle.type = PARTICLE_TYPE_SPARK;
        newParticle.boardSize = SPARK_BOARDSIZE;
        newParticle.lifetime = SPARK_LIFETIME;
        newParticle.position = input.position;
        for (int i = 0; i < 10; ++i)
        {
            newParticle.velocity = input.velocity + RandomDirection(i) * 100.0f;
            outStream.Append(newParticle);
        }
    }
    
}
void SparkParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> outStream)
{
     // ������ ���� ������ �̵���Ű��(�߷��� ������ �޵��� �Ѵ�.), 
    if (input.lifetime > 0.0f)  // ������ �������� ���
    {
        input.lifetime -= elapsedTime;
        input.boardSize = SPARK_BOARDSIZE * (input.lifetime / SPARK_LIFETIME);
        input.position += input.velocity * elapsedTime;
        input.velocity.y += -GRAVITY * elapsedTime; // �߷°��ӵ��� �ð��� ���ؼ� y�ӵ��� �����ش�.
        outStream.Append(input);
    }
}

float3 RandomDirection(float seedOffset)
{
    float2 seed = float2(currentTime + seedOffset, currentTime - seedOffset);
    float3 direction;
    
    direction.x = RandomFloat(seed) - 0.5f;
    direction.y = RandomFloat(seed + float2(0.435, 0.12)) - 0.5f;
    direction.z = RandomFloat(seed + float2(0.0345, 0.74)) - 0.5f;
    direction = normalize(direction);
    return direction;
}

float RandomFloat(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ParticleDraw

struct VS_PARTICLE_OUTPUT
{
    float3 positionW : POSITION;
    float2 boardSize : BOARDSIZE;
    float3 normal : NORMAL;
    uint type : PARTICLETYPE;
    float lifetime : LIFETIME;
};

struct GS_PARTICLE_OUTPUT
{
    float3 positionW : POSITION;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    uint type : PARTICLETYPE;
    float lifetime : LIFETIME;
};

VS_PARTICLE_OUTPUT ParticleDrawVertexShader(VS_PARTICLE_INPUT input)
{
    VS_PARTICLE_OUTPUT output;
    
    output.positionW = input.position;
    output.boardSize = input.boardSize;

    output.normal = cameraPosition - output.positionW;
    output.normal = normalize(output.normal);
    
    output.type = input.type;
    output.lifetime = input.lifetime;
    
    return output;
}

[maxvertexcount(4)]
void ParticleDrawGeometryShader(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
    GS_PARTICLE_OUTPUT output;
    output.normal = input[0].normal;
    output.type = input[0].type;
    output.lifetime = input[0].lifetime;
    
    float3 rightVector = normalize(cross(float3(0, 1, 0), input[0].normal)); // Y��� look���� �����ؼ� rightVector�� ��´�.
    float3 upVector = normalize(cross(input[0].normal, rightVector)); //
    
    float3 dxOffset = rightVector * input[0].boardSize.x / 2.0f;
    float3 dyOffset = upVector * input[0].boardSize.y / 2.0f;
    
     // �ð������ ����
    float3 dx[4] = { -dxOffset, dxOffset, -dxOffset, dxOffset };
    float3 dy[4] = { -dyOffset, -dyOffset, dyOffset, dyOffset };
    
    float2 startUV = float2(output.type % PARTICLES_TEXTURE_COLUMN / (float) (PARTICLES_TEXTURE_COLUMN), output.type / PARTICLES_TEXTURE_ROW / (float) (PARTICLES_TEXTURE_ROW));
    float2 widUV = float2(1.0f / PARTICLES_TEXTURE_COLUMN, 1.0f / PARTICLES_TEXTURE_ROW);
    
    float2 uv[4] = { startUV + widUV, startUV + float2(0, widUV.y), startUV + float2(widUV.x, 0), startUV }; // ��������, ������, �����ʾƷ�, ���ʾƷ�
    for (int i = 0; i < 4; ++i)
    {
        output.positionW = input[0].positionW + dx[i] + dy[i];
        output.position = mul(mul(float4(output.positionW, 1.0f), view), projection);
        output.uv = uv[i];
        outStream.Append(output);
    }
}

float4 ParticleDrawPixelShader(GS_PARTICLE_OUTPUT input) : SV_TARGET
{
    // type�� �̿��Ͽ� ���ϴ� �ؽ�ó�� ����
    float4 color = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    
    if (color.a < 0.1f)
        discard;
    return color;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lighting
float4 CalculateLight2(float3 _Position, float3 _Normal, OBJColors _objColors)
{
    float3 toCamera = normalize(cameraPosition - _Position);    // ��ü�� ��ġ���� ī�޶���� ����
    
    float4 color = _objColors.diffuse * globalAmbient;  // ������Ʈ�� ���� ���� global �ֺ����� ������ŭ �����ش�.
    color += _objColors.emission;   // �߻걤�� �����ش�. (���� ���� ���� ���̱� ������ �ٸ� ���� ������ ���� �ʿ䰡 ����.)
    
	// ������ unroll�Ͽ� ���� ���. max light��ŭ unroll�� ���̴� �����Ͻÿ� �ð��� �ʹ� �����ɷ� 10%�� Ÿ��
	[unroll(MAX_LIGHTS / 10)]
    for (int i = 0; i < nLight; i++)
    {
        if (lights[i].enable)
        {
            if (lights[i].lightType == DIRECTIONAL_LIGHT)
            {
                color += DirectionalLight2(i, _Normal, toCamera, _objColors);
            }
            else if (lights[i].lightType == POINT_LIGHT)
            {
                color += PointLight2(i, _Position, _Normal, toCamera, _objColors);
            }
            else if (lights[i].lightType == SPOT_LIGHT)
            {
                color += SpotLight2(i, _Position, _Normal, toCamera, _objColors);
            }
        }
        if (1 <= color.r && 1 <= color.g &&  1 <= color.b)
            break;
    }
    
    color.a = _objColors.diffuse.a;
    return color;
}
float4 DirectionalLight2(int _nIndex, float3 _normal, float3 _toCamera, OBJColors _objColors)
{

	// ���� ����
    float3 toLight = -lights[_nIndex].direction;
	
	// ���� ����� ������ �������� ���� ���
    float diffuseFactor = dot(toLight, _normal);
    if (diffuseFactor > EPSILON)    // ���� ��ü�� ���ֺ� ���
    {
		// �ݻ纤�͸� ���� �ü����Ϳ� �����Ͽ� ���� �� ���
        float3 reflectVector = reflect(-toLight, _normal);
        float specularFactor = pow(max(dot(reflectVector, _toCamera), 0.0f), materialSpecular.a);
        
        // �� ��ǻ�� * ��ü ��ǻ�� * ���� + �� ����ŧ�� * ��ü ����ŧ�� * ����ŧ�� ����
        return (lights[_nIndex].diffuse * _objColors.diffuse * diffuseFactor) + (lights[_nIndex].specular * _objColors.specular * specularFactor);
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
    return float4(0, 0, 0, 0);
}
float4 PointLight2(int _nIndex, float3 _position, float3 _normal, float3 _toCamera, OBJColors _objColors)
{
	// ���� �������� ���ͷ� �Ÿ� ���
    float3 toLight = lights[_nIndex].position - _position;
    float distance = length(toLight);
	
	// ������ range������ ����� ���
    if (distance <= lights[_nIndex].range)
    {
        
        toLight /= distance;
		
        float diffuseFactor = dot(toLight, _normal);
        if (diffuseFactor > EPSILON)
        {
            float3 reflectVec = reflect(-toLight, _normal);
            float specularFactor = pow(max(dot(reflectVec, _toCamera), 0.0f), materialSpecular.a);
            // 1/(x+y*d+z*d*d). distance = 0�� ��� 1/x
            float attenuationFactor = 1.0f / dot(lights[_nIndex].attenuation, float3(1.0f, distance, distance * distance));
            return ((lights[_nIndex].diffuse * _objColors.diffuse * diffuseFactor) + (lights[_nIndex].specular * _objColors.specular * specularFactor)) * attenuationFactor;
        }
        else
        {
            return float4(0, 0, 0, 0);
        }
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
    return float4(0, 0, 0, 0);
}
float4 SpotLight2(int _nIndex, float3 _position, float3 _normal, float3 _toCamera, OBJColors _objColors)
{
    float3 toLight = lights[_nIndex].position - _position;
    float fDistance = length(toLight);
	
    if (fDistance <= lights[_nIndex].range)
    {
        toLight /= fDistance;
        float diffuseFactor = dot(toLight, _normal);
        if (diffuseFactor > EPSILON)
        {
            float3 vReflect = reflect(-toLight, _normal);
            float specularFactor = pow(max(dot(vReflect, _toCamera), 0.0f), materialSpecular.a);
            // phi = ���� ���� �׸��� ���� cos��, theta - �ܺ� ���� �׸��� ���� cos��
		    // falloff = �������.		
		    // ���� ������ ������ ���� ������ ����
            float alpha = max(dot(-toLight, lights[_nIndex].direction), 0.0f);
				// ���� ���� spot����� ���.
            float spotFactor = pow(max(((alpha - lights[_nIndex].phi) / (lights[_nIndex].theta - lights[_nIndex].phi)), 0.0f), lights[_nIndex].falloff);
                // �Ÿ��� ���� �������� ���.
            float attenuationFactor = 1.0f / dot(lights[_nIndex].attenuation, float3(1.0f, fDistance, fDistance * fDistance));

            // �� ����� ���� ���� ���� ��
            return ((lights[_nIndex].diffuse * _objColors.diffuse * diffuseFactor) + (lights[_nIndex].specular * _objColors.specular * specularFactor)) * attenuationFactor * spotFactor;
        }
        else
        {
            return float4(0, 0, 0, 0);
        }
    }
    else
    {
        return float4(0, 0, 0, 0);
    }
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

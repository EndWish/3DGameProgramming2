

#define MAX_LIGHTS			100


#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

#define EPSILON				1.0e-5

struct LIGHT
{
	float4 ambient;
	float4 diffuse;
	float4 specular;

	float3 position;
	float range;

	float3 offset;	// 빛을 내는 물체의 중심으로부터 떨어진 값
	float theta; //cos(m_fTheta), 스포트라이트에서 사용
	float3 attenuation;
	float phi; //cos(m_fPhi), 스포트라이트에서 사용
	float3 direction;
    float falloff;

	// 이 빛을 내고 있는 오브젝트의 포인터 (미사용)
	float4 object;

	// 1 = 점, 2 = 스포트, 3 = 직접
	int lightType;

	// 이 빛이 켜져있는 상태인지 확인
	bool enable;

    float2 padding;
};

cbuffer cbLightInfo : register(b3) {
	LIGHT lights[MAX_LIGHTS];
	float4 globalAmbient;
	int	nLight;
}

cbuffer cbMaterialInfo : register(b4) {
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 emissive;
    uint textureType;
}



float4 DirectionalLight(int _nIndex, float3 _normal, float3 _toCamera)
{

	// 빛의 방향
	float3 toLight = -lights[_nIndex].direction;
	
	// 빛의 방향과 정점의 법선으로 각도 계산
	float diffuseFactor = dot(toLight, _normal);
	float specularFactor = 0.0f;
	if (diffuseFactor > EPSILON)	{
		// 반사벡터를 구해 시선벡터와 내적하여 빛의 양 계산
		float3 reflectVector = reflect(-toLight, _normal);
		specularFactor = pow(max(dot(reflectVector, _toCamera), 0.0f), specular.a);
	}
	
    return ((lights[_nIndex].ambient * ambient) + (lights[_nIndex].diffuse * diffuseFactor * diffuse) + (lights[_nIndex].specular * specularFactor * specular));
}


float4 PointLight(int _nIndex, float3 _position, float3 _normal, float3 _toCamera)
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	// 빛과 정점사이 벡터로 거리 계산
	float3 toLight = lights[_nIndex].position - _position;
	float distance = length(toLight);
	
	// 설정한 range값보다 가까울 경우
	if (distance <= lights[_nIndex].range)	{
		float specularFactor = 0.0f;
		toLight /= distance;
		
		float diffuseFactor = dot(toLight, _normal);
        if (diffuseFactor > EPSILON) {
			if (specular.a != 0.0f)	{
				float3 reflectVec = reflect(-toLight, _normal);
                specularFactor = pow(max(dot(reflectVec, _toCamera), 0.0f), specular.a);
            }
		}
		// 1/(x+y*d+z*d*d). distance = 0일 경우 1/x
        float attenuationFactor = 1.0f / dot(lights[_nIndex].attenuation, float3(1.0f, distance, distance*distance));
        color = ((lights[_nIndex].ambient * ambient) + (lights[_nIndex].diffuse * diffuseFactor * diffuse) + (lights[_nIndex].specular * specularFactor * specular)) * attenuationFactor;
    }
	return color;
}


float4 SpotLight(int _nIndex, float3 _position, float3 _normal, float3 _toCamera) {
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 toLight = lights[_nIndex].position - _position;
    float fDistance = length(toLight);
	
    if (fDistance <= lights[_nIndex].range) {
        float fSpecularFactor = 0.0f;
        toLight /= fDistance;
        float fDiffuseFactor = dot(toLight, _normal);
		
        if (fDiffuseFactor > EPSILON) {
            if (specular.a != 0.0f) {
                float3 vReflect = reflect(-toLight, _normal);
                fSpecularFactor = pow(max(dot(vReflect, _toCamera), 0.0f), specular.a);
            }
        }
		// phi = 내부 원을 그리는 각의 cos값, theta - 외부 원을 그리는 각의 cos값
		// falloff = 감쇠비율.		
		// 빛과 점사이 각도와 빛의 방향을 내적
        float alpha = max(dot(-toLight, lights[_nIndex].direction), 0.0f);
				// 각에 따른 spot계수를 계산.
        float spotFactor = pow(max(((alpha - lights[_nIndex].phi) / (lights[_nIndex].theta - lights[_nIndex].phi)), 0.0f), lights[_nIndex].falloff);
                // 거리에 따른 감쇠계수를 계산.
        float attenuationFactor = 1.0f / dot(lights[_nIndex].attenuation, float3(1.0f, fDistance, fDistance * fDistance));
				
				// 각 계수를 구한 빛에 대해 곱
        color = ((lights[_nIndex].ambient * ambient) + (lights[_nIndex].diffuse * fDiffuseFactor * diffuse) + (lights[_nIndex].specular * fSpecularFactor * specular)) * attenuationFactor * spotFactor;
    }
    return color;
}


float4 CalculateLight(float3 _Position, float3 _Normal) {
	
    float3 toCamera = normalize(cameraPosition - _Position);
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
		//color = float4(1.0f, 1.0f, 1.0f, 0.0f);
	
	// 루프를 unroll하여 성능 향상. max light만큼 unroll시 셰이더 컴파일시에 시간이 너무 오래걸려 10%로 타협
	[unroll(MAX_LIGHTS / 10)]
    for (int i = 0; i < nLight; i++) {
            if (lights[i].enable) {
                if (lights[i].lightType == DIRECTIONAL_LIGHT) {
                    color += DirectionalLight(i, _Normal, toCamera);
                }
                else if (lights[i].lightType == POINT_LIGHT) {
                    color += PointLight(i, _Position, _Normal, toCamera);
                }
                else if (lights[i].lightType == SPOT_LIGHT) {
                    color += SpotLight(i, _Position, _Normal, toCamera);
                }
            }
    }
	//color += (gcGlobalAmbientLight * gm_cAmbient);
    color += globalAmbient;
    color.a = 1;
	//color.a = gm_cDiffuse.a;
    return color;
}


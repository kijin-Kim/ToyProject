cbuffer LightData : register(b1, space0)
{
    float4 LightDirection;
	float4 LightColor;
    float4 CameraPosition;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
    float3 WorldPosition : WORLDPOSITION;
    float3 WorldNormal : WORLDNORMAL;
    float2 TexCoord : TEXCOORD;
};

Texture2D albedoTexture : register(t0, space0);
SamplerState defaultSampler : register(s0, space0);

float4 main(PS_INPUT input) : SV_TARGET
{

    const float4 albedo = albedoTexture.Sample(defaultSampler, input.TexCoord);
    const float ambientStrength = 0.5f;
    const float3 ambient = saturate(LightColor * ambientStrength).xyz;
	
   
    const float diffuseStrength = 0.5f;
    const float3 n = normalize(input.WorldNormal);
    const float3 l = -LightDirection.xyz; // FROMLIGHT to TOLIGHT
    const float nDotL = dot(n, l);
    const float3 diffuse = saturate(max(nDotL, 0.0f) * LightColor * diffuseStrength).rgb;
    
    const float3 v = normalize(CameraPosition.xyz - input.WorldPosition);
    const float3 h = normalize(l + v); 
    const float hDotN = dot(h, n);
	const float specularPower = 32.0f * 4.0f;
    const float3 specular = saturate(pow(max(hDotN, 0.0f), specularPower) * LightColor).rgb;
    return float4(albedo.rgb * saturate(ambient + diffuse + specular), 1.0f);
}
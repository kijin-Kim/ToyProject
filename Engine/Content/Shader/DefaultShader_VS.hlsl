cbuffer PerFrame : register(b0, space0)
{
	float4x4 MatGeo;
	float4x4 MatGeoInvert;
    float4x4 MatVP;
};

struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float3 WorldPosition : WORLDPOSITION;
	float3 WorldNormal : WORLDNORMAL;
	float2 TexCoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.WorldPosition = mul(float4(input.Position, 1.0f), MatGeo).xyz;
	output.Position = mul(float4(output.WorldPosition, 1.0f), MatVP);
	float3x3 InvTransposeGeo = (float3x3)transpose(MatGeoInvert);
	output.WorldNormal = normalize(mul(input.Normal, (float3x3)InvTransposeGeo));
	output.TexCoord = input.TexCoord;
	return output;
}
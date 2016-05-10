cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
}

cbuffer cbLightColor : register(b1)
{
	float4 ambientColor;
	float4 surface; //[ka, kd, ks, m]
	float4 lightColor;
}

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 viewVec : TEXCOORD0;
	float3 lightVec : TEXCOORD1;
	float3 light2Vec : TEXCOORD2;
};

static const float4 light2Color = { 0.0, 0.0, 0.0, 1.0 };

float4 main(PSInput i) : SV_TARGET
{
	float3 viewVec = normalize(i.viewVec);
	float3 normal = normalize(i.norm);
	float3 lightVec = normalize(i.lightVec);
	float3 light2Vec = normalize(i.light2Vec);
	float3 halfVec = normalize(viewVec + lightVec);
	float3 half2Vec = normalize(viewVec + light2Vec);

	float3 color = surfaceColor.rgb * ambientColor;
	color += lightColor * surfaceColor.xyz * surface.y * saturate(dot(normal, lightVec)); //diffuse Color
	color += lightColor * surface.z * pow(saturate(dot(normal, halfVec)), surface.w); //specular Color

	color += light2Color * surfaceColor.xyz * surface.y * saturate(dot(normal, light2Vec)); //diffuse Color
	color += light2Color * surface.z * pow(saturate(dot(normal, half2Vec)), surface.w); //specular Color

	if (!surfaceColor.a)
		discard;
	return float4(saturate(color), surfaceColor.a);
}
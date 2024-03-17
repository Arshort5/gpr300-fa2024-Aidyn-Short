#version 450
out vec4 FragColor; //The color of this fragment
in Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
}fs_in;


 vec4 lightSpacePos;

in vec2 UV;

uniform sampler2D _MainTex; 
uniform sampler2D _ShadowMap;

uniform float minBias = 0.005;
uniform float maxBias = 0.015;


uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;


uniform layout(binding = 0) sampler2D _gPositions;

uniform layout(binding = 1) sampler2D _gNormals;

uniform layout(binding = 2) sampler2D _gAlbedo;




float calcShadow(sampler2D shadowMap, vec4 lightSpacePos,float bias)
{
	vec3 sampleCoord = lightSpacePos.xyz / lightSpacePos.w;
	sampleCoord = sampleCoord * 0.5 + 0.5;

	

	float myDepth = sampleCoord.z - bias;



	float totalShadow = 0;
	vec2 texelOffset = 1.0 / textureSize(_ShadowMap,0);

	for(int y = -1; y <=1; y++)
	{
		for(int x = -1; x <=1; x++)
		{
			vec2 uv = sampleCoord.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow+=step(texture(_ShadowMap,uv).r,myDepth);
		}
	}

  totalShadow /= 9.0;

	return totalShadow;
}













void main(){


	vec3 normal = texture(_gNormals, UV).xyz;
	vec3 worldPos = texture(_gPositions, UV).xyz;
	vec3 albedo = texture(_gAlbedo, UV).xyz;


	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);


	vec3 toEye = normalize(_EyePos - worldPos);


	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);


	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;



	//lightColor+=_AmbientColor * _Material.Ka;

	float bias = max(maxBias * (1.0 - dot(normal,toLight)),minBias);
	float shadow = calcShadow(_ShadowMap, lightSpacePos, bias);

	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;

	//vec3 light = lightColor * (1.0 -shadow);
	
	vec3 light = lightColor * (1.0-shadow);
	light += _AmbientColor * _Material.Ka;
	
	
	FragColor = vec4(albedo * light,1.0);
}


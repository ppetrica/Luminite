#version 330


in vec3 normal;
in vec3 pos;


out vec4 fragColor;


uniform vec3 u_lightpos;
uniform vec3 u_viewpos;


vec3 lighting(vec3 objectColor, vec3 pos, vec3 normal, vec3 lightPos, vec3 viewPos,
				vec3 ambient, vec3 lightColor, vec3 specular, float specPower)
{
	vec3 ambientColor = ambient * lightColor;
	
	vec3 L = normalize(lightPos - pos);
	vec3 V = normalize(viewPos - pos);
	vec3 N = normalize(normal);
	vec3 R = reflect(-L, N);

	float diffCoef = dot(L, N);
	float specCoef = pow(dot(R, V), specPower);

	vec3 diffuseColor = diffCoef * lightColor;
	vec3 specularColor = specCoef * specular * lightColor;

	vec3 col = min(ambientColor + diffuseColor + specularColor, 1.0); 

	return clamp(col, 0, 1);
}


void main() {
	vec3 objectColor = vec3(1.0, 1.0, 1.0);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	vec3 ambient = vec3(0.1);
	vec3 specular = vec3(0.8);
	float specPower = 32;
	
	vec3 color = lighting(objectColor, pos, normal, u_lightpos, u_viewpos, ambient, lightColor, specular, specPower);
	//vec3 color = (pos + 1.0f) / 2.0f;

	fragColor = vec4(color, 1.0);
}
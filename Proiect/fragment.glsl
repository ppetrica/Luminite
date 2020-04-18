#version 330


in vec3 normal;
in vec3 pos;


out vec4 fragColor;


uniform vec3 u_lightdir;
uniform vec3 u_cubecolor;
uniform vec3 u_viewpos;


struct point_light {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};


void main() {
	point_light light;
	
	light.position = u_lightdir;
	light.constant = 1.0f;
	light.linear = 0.09f;
	light.quadratic = 0.032f;

	float distance = length(light.position - pos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	vec3 ambient = vec3(0.3);

	vec3 ambient_component = ambient;

	vec3 L = normalize(light.position - pos);

	vec3 viewdir = normalize(u_viewpos - pos);

	vec3 reflectdir = normalize(reflect(-light.position, normal));

	float spec = pow(max(dot(viewdir, reflectdir), 0.0), 32);

	float diffuse_component = max(dot(L, normal), 0.0);

	float specular_component = max(0.5 * spec, 0.0);

	vec3 final_color = (ambient_component + diffuse_component + specular_component) * u_cubecolor * attenuation;

	vec3 color = min(final_color, 1.0);

	fragColor = vec4(color, 1.0);
}
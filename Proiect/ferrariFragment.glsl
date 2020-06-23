#version 330


in vec3 normal;
in vec3 pos;
in vec2 uv_coords;


out vec4 fragColor;


uniform vec3 u_lightpos;
uniform vec3 u_viewpos;

uniform sampler2D u_ferrari_tex;
uniform sampler2D u_normal_tex;
uniform sampler2D u_specular_tex;


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
	
	light.position = u_lightpos;
	light.constant = 1.0f;
	light.linear = 0.2f;
	light.quadratic = 0.010f;

	vec3 normalFromMap = texture(u_normal_tex, uv_coords).rgb;

	normalFromMap.g = 1 - normalFromMap.g;
	normalFromMap = normalFromMap * 2 - 1;

	vec3 ambient = vec3(0.5);

	vec3 ambient_component = ambient;

	vec3 L = normalize(light.position - pos);

	vec3 viewdir = normalize(u_viewpos - pos);

	vec3 reflectdir = normalize(reflect(-light.position, -normal));

	float spec = pow(max(dot(viewdir, reflectdir), 0.0), 32);

	float distance = length(light.position - pos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

	float diffuse_component = max(-dot(L, -normal), 0.0);

	//float specular_component = max(spec, 0.0);
	vec3 specular_component = texture(u_specular_tex, uv_coords).rgb * spec;

	vec3 final_color = (ambient_component + diffuse_component + specular_component) * texture(u_ferrari_tex, uv_coords).rgb * attenuation;

	vec3 color = min(final_color, 1.0);

	fragColor = vec4(color, 1.0);
}
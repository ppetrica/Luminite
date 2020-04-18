#version 330


in vec3 normal;
in vec3 pos;


out vec4 fragColor;


uniform vec3 u_lightdir;
uniform vec3 u_viewpos;


void main() {
	vec3 object_color = vec3(0.8, 0.2, 0.2);
	vec3 ambient = vec3(0.3);
	
	vec3 ambient_component = ambient * object_color;
	
	vec3 L = normalize(u_lightdir - pos);

	vec3 viewdir = normalize(u_viewpos - pos);

	vec3 reflectdir = normalize(reflect(-u_lightdir, normal));

	float spec = pow(max(dot(viewdir, reflectdir), 0.0), 8);

	float diffuse_component = max(dot(L, normal), 0.0);

	float specular_component = max(0.5 * spec, 0.0);

	vec3 final_color = (ambient_component + diffuse_component + specular_component) * object_color;

	vec3 color = min(final_color, 1.0); 

	fragColor = vec4(color, 1.0);
}
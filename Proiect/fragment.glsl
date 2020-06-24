#version 330


in vec3 normal;
in vec3 pos;
in vec2 uv_coords;


out vec4 fragColor;


// type 0 = textured
// type 1 = colored
// type 3 = light
uniform int u_type;

uniform vec3 u_color;
uniform int u_n_lights;


uniform vec3 u_viewpos;

uniform sampler2D u_tex;


struct point_light {
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};


vec3 calculate_point_light(point_light light, vec3 view_pos, vec3 object_pos, vec3 object_normal) {
	vec3 ambient_component = light.ambient;

	vec3 L = normalize(light.position - object_pos);

	vec3 viewdir = normalize(view_pos - object_pos);

	vec3 reflectdir = normalize(reflect(-light.position, object_normal));

	float spec = pow(max(dot(viewdir, reflectdir), 0.0), 32);

	float distance = length(light.position - object_pos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	float diffuse_component = max(dot(L, object_normal), 0.0);

	float specular_component = max(spec, 0.0);

	return (ambient_component + diffuse_component + specular_component) * attenuation;
}


uniform point_light u_light[256];


uniform vec3 u_light_color;


void main() {
	vec3 light = vec3(0.0f);
	for (int i = 0; i < u_n_lights; ++i) {
		light += calculate_point_light(u_light[i], u_viewpos, pos, normal);
	}

	light *= u_light_color;
	
	vec3 color;
	if (u_type == 0) {
		color = min(light * texture(u_tex, uv_coords).rgb, 1.0f);
	} else if (u_type == 1) {
		color = min(light * u_color, 1.0f);
	} else if (u_type == 2) {
		color = u_light_color;
	}

	fragColor = vec4(color, 1.0);
}

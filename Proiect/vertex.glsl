#version 330


in vec3 v_pos;
in vec3 v_normal;


uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;


uniform mat4 u_normal;


out vec3 normal;
out vec3 pos;


void main() {
    gl_Position = u_proj * u_view * u_model * vec4(v_pos, 1.0f);

    // TODO: these normal values seem to be wrong
    normal = vec3(u_normal * vec4(v_normal, 1));
    pos = v_pos;
};

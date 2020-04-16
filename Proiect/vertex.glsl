#version 330


in vec2 pos;


uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;


void main() {
    gl_Position = u_proj * u_view * u_model * vec4(pos, 0.0f, 1.0f);
};

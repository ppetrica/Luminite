#version 330


in vec2 pos;


uniform mat4 u_model;


void main() {
    gl_Position = u_model * vec4(pos, 0.0f, 1.0f);
};

#include <stdio.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <glfw/glfw3.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#define GLCALL(call)\
    while (glGetError() != GL_NO_ERROR);\
    call;\
    glTreatError(#call, __LINE__)


void glTreatError(const char* func, int line) {
    int res = glGetError(); 
    if (res != GL_NO_ERROR) {
        fprintf(stderr, "%s failed on line %d with code 0x%x\n", func, line, res);
    }
}


bool read_file(const char* filepath, char **buffer) {
    if (!filepath || !buffer) return false;

    int length;
    int total_read = 0;
    int read;
    char *buf;

    FILE *file = fopen(filepath, "r");
    if (!file)
        return false;

    bool ret = true;
    if (fseek(file, 0, SEEK_END) < 0) {
        ret = false;

        goto close_file;
    }

    length = ftell(file);
    if (length <= 0) {
        ret = false;

        goto close_file;
    }

    rewind(file);

    buf = (char *)malloc(length + 1);
    if (!buf) {
        ret = false;

        goto close_file;
    }

    while (read = fread(buf + total_read, 1, length - total_read, file))
        total_read += read;

    buf[length] = '\0';

    *buffer = buf;

close_file:
    fclose(file);

    return ret;
}


bool create_shader(const char *filepath, uint32_t type, uint32_t *out_shader) {
    if (!filepath || !out_shader) return false;

    char *source;
    if (!read_file(filepath, &source)) return false;

    uint32_t shader;
    GLCALL(shader = glCreateShader(type));

    GLCALL(glShaderSource(shader, 1, &source, NULL));

    GLCALL(glCompileShader(shader));

    *out_shader = shader;
 
    free(source);

    return true;
}


int main() {
    const struct {
        float x;
        float y;
    } coords[] = {
        {-0.5f, -0.5f},
        { 0.0f,  0.5f},
        { 0.5f, -0.5f},
    };

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize glfw library.\n");

        return 1;
    }
    
    GLenum err;

    uint32_t vertex_shader;
    uint32_t fragment_shader;
    uint32_t program;

    int length;
    char error_log[1024];

    GLFWwindow *window = glfwCreateWindow(640, 480, "Hello OpenGL", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create glfw window.\n");

        const char *description;
        if (glfwGetError(&description) != GLFW_NO_ERROR) {
            fprintf(stderr, "%s\n", description);
        }

        goto uninit_glfw;
    }

    glfwMakeContextCurrent(window);

    err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Failed to initialize glew: %s", glewGetErrorString(err));

        goto destroy_window;
    }

    uint32_t vao;
    GLCALL(glGenVertexArrays(1, &vao));
    GLCALL(glBindVertexArray(vao));

    uint32_t vbo;
    GLCALL(glGenBuffers(1, &vbo));
    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));

    GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(coords), &coords[0], GL_STATIC_DRAW));

    GLCALL(glEnableVertexAttribArray(0));
    GLCALL(glVertexAttribPointer(0, 2, GL_FLOAT, 0, sizeof(coords[0]), NULL));

    if (!create_shader("vertex.glsl", GL_VERTEX_SHADER, &vertex_shader)) {
        fprintf(stderr, "Failed to read vertex shader.\n");

        goto delete_buffers;
    }

    GLCALL(glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length));
    if (length) {
        GLCALL(glGetShaderInfoLog(vertex_shader, sizeof(error_log), NULL, &error_log[0]));
        fprintf(stderr, "Failed to compile vertex shader: %s", error_log);
        goto delete_vertex;
    }

    if (!create_shader("fragment.glsl", GL_FRAGMENT_SHADER, &fragment_shader)) {
        fprintf(stderr, "Failed to read fragment shader.\n");

        goto delete_vertex;
    }

    GLCALL(glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length));
    if (length) {
        GLCALL(glGetShaderInfoLog(fragment_shader, sizeof(error_log), NULL, &error_log[0]));
        fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
        goto delete_fragment;
    }

    GLCALL(program = glCreateProgram());

    GLCALL(glAttachShader(program, vertex_shader));
    GLCALL(glAttachShader(program, fragment_shader));

    GLCALL(glLinkProgram(program));

    GLCALL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
    if (length) {
        GLCALL(glGetProgramInfoLog(program, sizeof(error_log), NULL, &error_log[0]));
        fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
        goto delete_program;
    }

    GLCALL(glUseProgram(program));

    while (!glfwWindowShouldClose(window)) {
        glDrawArrays(GL_TRIANGLES, 0, sizeof(coords) / sizeof(coords[0]));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

delete_program:
    glDeleteProgram(program);

delete_fragment:
    glDeleteShader(fragment_shader);

delete_vertex:
    glDeleteShader(vertex_shader);

delete_buffers:

    glDeleteBuffers(1, &vbo);

    glDeleteVertexArrays(1, &vao);

destroy_window:
    glfwDestroyWindow(window);

uninit_glfw:
    glfwTerminate();

    return 0;
}
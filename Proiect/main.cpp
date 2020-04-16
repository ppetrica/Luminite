#include <stdio.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <glfw/glfw3.h>
#include <assert.h>
#include <stdint.h>


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


int main() {
    const char *vertex_source = 
        "#version 330\n"
        "\n"
        "in vec2 pos;\n"
        "\n"
        "void main() {\n"
        "    gl_Position = vec4(pos, 0.0f, 1.0f);\n"
        "}\n";

    const char *fragment_source = 
        "#version 330\n"
        "\n"
        "out vec4 color;\n"
        "\n"
        "void main() {\n"
        "    color = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
        "}\n";

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

    GLCALL(vertex_shader = glCreateShader(GL_VERTEX_SHADER));
    GLCALL(glShaderSource(vertex_shader, 1, &vertex_source, NULL));

    GLCALL(glCompileShader(vertex_shader));

    GLCALL(glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length));
    if (length) {
        GLCALL(glGetShaderInfoLog(vertex_shader, sizeof(error_log), NULL, &error_log[0]));
        fprintf(stderr, "Failed to compile vertex shader: %s", error_log);
        goto uninit_vbo;
    }

    GLCALL(fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));
    GLCALL(glShaderSource(fragment_shader, 1, &fragment_source, NULL));

    GLCALL(glCompileShader(fragment_shader));

    GLCALL(glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length));
    if (length) {
        GLCALL(glGetShaderInfoLog(fragment_shader, sizeof(error_log), NULL, &error_log[0]));
        fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
        goto uninit_vbo;
    }

    GLCALL(program = glCreateProgram());

    GLCALL(glAttachShader(program, vertex_shader));
    GLCALL(glAttachShader(program, fragment_shader));

    GLCALL(glLinkProgram(program));

    GLCALL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
    if (length) {
        GLCALL(glGetProgramInfoLog(program, sizeof(error_log), NULL, &error_log[0]));
        fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
        goto uninit_vbo;
    }

    GLCALL(glUseProgram(program));

    while (!glfwWindowShouldClose(window)) {
        glDrawArrays(GL_TRIANGLES, 0, sizeof(coords) / sizeof(coords[0]));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    printf("%s\n", glGetString(GL_VERSION));

    printf("Hello World!\n");

uninit_vbo:
    glDeleteBuffers(1, &vbo);

uninit_vao:
    glDeleteVertexArrays(1, &vao);

destroy_window:
    glfwDestroyWindow(window);

uninit_glfw:
    glfwTerminate();

    return 0;
}
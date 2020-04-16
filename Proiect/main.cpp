#include "shader.h"
#include "wrappers.h"
#include <cstdio>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <fstream>
#include <filesystem>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>


void run_main_loop(GLFWwindow *window, uint32_t program, uint32_t n_vertices);


#ifdef _DEBUG
static void APIENTRY debug_proc(GLenum source,
                         GLenum type,
                         GLuint id,
                         GLenum severity,
                         GLsizei length,
                         const GLchar* message,
                         const void* userParam) {
    fprintf(stderr, "[%d] %d generated error %x: %s", severity, source, type, message);
}
#endif // _DEBUG


int main() {
    try {
        glfw_t glfw;

        window_t window{ glfwCreateWindow(640, 480, "Hello OpenGL", NULL, NULL), glfwDestroyWindow };
        if (!window) {
            fprintf(stderr, "Failed to create glfw window.\n");

            const char* description;
            if (glfwGetError(&description) != GLFW_NO_ERROR) {
                fprintf(stderr, "%s\n", description);
            }
        }

#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        glfwMakeContextCurrent(window.get());

        GLenum err = glewInit();
        if (err != GLEW_OK) {
            fprintf(stderr, "Failed to initialize glew: %s", glewGetErrorString(err));

            return 1;
        }

#ifdef _DEBUG
        glDebugMessageCallback(debug_proc, NULL);
#endif

        uint32_t handle;
        glGenVertexArrays(1, &handle);
        vertex_array_t vao{ handle };

        glBindVertexArray(vao.get());

        glGenBuffers(1, &handle);
        buffer_t vbo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, vbo.get());

        std::vector<glm::vec2> coords = {
            {-0.5f, -0.5f},
            { 0.0f,  0.5f},
            { 0.5f, -0.5f}
        };

        glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(coords[0]), &coords[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, 0, sizeof(coords[0]), NULL);

        program_t program = load_program("vertex.glsl", "fragment.glsl");

        run_main_loop(window.get(), program.get(), coords.size());
    } catch (const std::exception &ex) {
        fprintf(stderr, "%s\n", ex.what());

        return 1;
    } catch (...) {
        fprintf(stderr, "An exception has occured.\n");
        
        return 1;
    }
}


void run_main_loop(GLFWwindow *window, uint32_t program, uint32_t n_vertices) {
    static const char *MODEL_UNIFORM = "u_odel";

    int model_location = glGetUniformLocation(program, MODEL_UNIFORM);
    if (model_location == -1)
        throw gl_uniform_not_found_exception(MODEL_UNIFORM);

    float angle = 0.0f;
    glm::mat4 model;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        model = glm::translate(glm::vec3{sin(angle) * 0.5f, 0.0f, 0.0f}) * glm::rotate(angle, glm::vec3{ 0.0f, 1.0f, 0.0f });

        angle += 0.001f;
        if (angle > 2 * glm::pi<float>())
            angle = 0.0f;

        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, n_vertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

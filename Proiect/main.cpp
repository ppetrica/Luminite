#include "shader.h"
#include "wrappers.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <chrono>


void run_main_loop(GLFWwindow *window, uint32_t program, uint32_t n_vertices);


#ifdef _DEBUG
static void APIENTRY debug_proc(GLenum source,
                         GLenum type,
                         GLuint id,
                         GLenum severity,
                         GLsizei length,
                         const GLchar* message,
                         const void* userParam) {
    spdlog::warn("[{}] {} generated error {}: {}", severity, source, type, message);
}
#endif // _DEBUG


static void window_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}


int main() {
    try {
#ifdef _DEBUG
        spdlog::set_level(spdlog::level::debug);
#endif

        glfw_t glfw;
        spdlog::info("Initialized GLFW");

        window_t window{ glfwCreateWindow(640, 480, "Hello OpenGL", NULL, NULL), glfwDestroyWindow };
        if (!window) {
            spdlog::error("Failed to create glfw window.\n");

            const char* description;
            if (glfwGetError(&description) != GLFW_NO_ERROR) {
                spdlog::error("{}\n", description);
            }
        }
        spdlog::info("Created main window");

#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        glfwMakeContextCurrent(window.get());
        spdlog::info("Initialized OpenGL context");

        glfwSetWindowSizeCallback(window.get(), window_size_callback);

        GLenum err = glewInit();
        if (err != GLEW_OK) {
            spdlog::error("Failed to initialize glew: {}", glewGetErrorString(err));

            return 1;
        }
        spdlog::info("Initialized GLEW");

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
        spdlog::info("Created main GLSL program");

        run_main_loop(window.get(), program.get(), coords.size());
    } catch (const std::exception &ex) {
        spdlog::error("{}\n", ex.what());

        return 1;
    } catch (...) {
        spdlog::error("An exception has occured.\n");
        
        return 1;
    }
}


void run_main_loop(GLFWwindow* window, uint32_t program, uint32_t n_vertices) {
    using namespace std::chrono_literals;

    static const char* MODEL_UNIFORM = "u_model";

    int model_location = glGetUniformLocation(program, MODEL_UNIFORM);
    if (model_location == -1)
        throw gl_uniform_not_found_exception(MODEL_UNIFORM);

    float angle = 0.0f;
    glm::mat4 model;

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    static const char* PROJ_UNIFORM = "u_proj";
    int proj_location = glGetUniformLocation(program, PROJ_UNIFORM);
    if (proj_location == -1)
        throw gl_uniform_not_found_exception(PROJ_UNIFORM);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(projection));

    static const char* VIEW_UNIFORM = "u_view";
    int view_location = glGetUniformLocation(program, VIEW_UNIFORM);
    if (view_location == -1)
        throw gl_uniform_not_found_exception(VIEW_UNIFORM);

    glm::mat4 view = glm::lookAt(glm::vec3{4, 3, 3}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

    glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

    std::chrono::microseconds dt = 0us;
    while (!glfwWindowShouldClose(window)) {
        auto start_frame_ts = std::chrono::high_resolution_clock::now();
        glClear(GL_COLOR_BUFFER_BIT);

        model = glm::translate(glm::vec3{sin(angle) * 3.0f, 0.0f, 0.0f}) * glm::rotate(angle, glm::vec3{ 0.0f, 1.0f, 0.0f });

        angle += 0.00001f * dt.count();
        if (angle > 2 * glm::pi<float>())
            angle = 0.0f;

        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, n_vertices);

        glfwSwapBuffers(window);
        glfwPollEvents();

        dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_frame_ts);
        spdlog::debug("Frame finished in {} microseconds.", dt.count());
    }
}

#include "shader.h"
#include "wrappers.h"
#include "loader.h"

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
    spdlog::info("Updating viewport to w: {} h: {}", width, height);

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
            spdlog::error("Failed to create glfw window.");

            const char* description;
            if (glfwGetError(&description) != GLFW_NO_ERROR) {
                spdlog::error("{}", description);
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
        auto [indices, vertices, uvs, normals] = load_asset("monkey.obj");

        uint32_t handle;
        glGenVertexArrays(1, &handle);
        vertex_array_t vao{ handle };

        glBindVertexArray(vao.get());

        glGenBuffers(1, &handle);
        buffer_t vbo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, vbo.get());

        vertices.reserve(vertices.size() + normals.size());

        vertices.insert(vertices.end(), normals.begin(), normals.end());

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(vertices[0]), NULL);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(normals[0]), (const void *)(normals.size() * sizeof(normals[0])));

        program_t program = load_program("vertex.glsl", "fragment.glsl");
        spdlog::info("Created main GLSL program");

        run_main_loop(window.get(), program.get(), vertices.size());
    } catch (const std::exception &ex) {
        spdlog::error("{}", ex.what());

        return 1;
    } catch (...) {
        spdlog::error("An exception has occured.");
        
        return 1;
    }
}


static inline int get_location(uint32_t program, const char *uniform_name) {
    int location = glGetUniformLocation(program, uniform_name);
    if (location == -1)
        throw gl_uniform_not_found_exception(uniform_name);
}


void run_main_loop(GLFWwindow* window, uint32_t program, uint32_t n_vertices) {
    using namespace std::chrono_literals;

    int model_location = get_location(program, "u_model");

    int normal_location = get_location(program, "u_normal");

    float angle = 0.0f;
    glm::mat4 model;
    glm::mat4 normal_matrix;

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    int proj_location = get_location(program, "u_proj");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(projection));

    int view_location = get_location(program, "u_view");

    int viewpos_location = get_location(program, "u_viewpos");
    
    glm::vec3 view_pos{4, 3, 3};
    
    glUniform3fv(viewpos_location, 1, glm::value_ptr(view_pos));

    int lightpos_location = get_location(program, "u_lightpos");

    glUniform3fv(lightpos_location, 1, glm::value_ptr(view_pos));

    glm::mat4 view = glm::lookAt(view_pos, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

    glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

    std::chrono::microseconds dt = 0us;
    while (!glfwWindowShouldClose(window)) {
        auto start_frame_ts = std::chrono::high_resolution_clock::now();
        glClear(GL_COLOR_BUFFER_BIT);

        model = glm::translate(glm::vec3{sin(angle) * 3.0f, 0.0f, 0.0f}) * glm::rotate(angle, glm::vec3{ 0.0f, 1.0f, 0.0f });

        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
        
        normal_matrix = glm::transpose(glm::inverse(model));
        
        glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

        glDrawArrays(GL_TRIANGLES, 0, n_vertices);

        glfwSwapBuffers(window);
        glfwPollEvents();

        dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_frame_ts);
        spdlog::debug("Frame finished in {} microseconds.", dt.count());

        angle += 0.00001f * dt.count();
        if (angle > 2 * glm::pi<float>())
            angle = 0.0f;
    }
}

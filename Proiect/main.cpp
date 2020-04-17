#include "shader.h"
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


struct key_callback_data {
    int view_location;
    int viewpos_location;

    glm::vec3 &viewpos;
};


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        spdlog::debug("Pressed key {}", key);

        struct key_callback_data &data = *(struct key_callback_data *)glfwGetWindowUserPointer(window);
        
        switch (key) {
        case GLFW_KEY_UP:
            data.viewpos.y += 1.0f;
            break;
        case GLFW_KEY_DOWN:
            data.viewpos.y -= 1.0f;
            break;
        case GLFW_KEY_RIGHT:
            data.viewpos.x += 1.0f;
            break;
        case GLFW_KEY_LEFT:
            data.viewpos.x -= 1.0f;
            break;
        case GLFW_KEY_PAGE_UP:
            data.viewpos.z += 1.0f;
            break;
        case GLFW_KEY_PAGE_DOWN:
            data.viewpos.z -= 1.0f;
            break;
        }

        glUniform3fv(data.viewpos_location, 1, glm::value_ptr(data.viewpos));

        glm::mat4 view = glm::lookAt(data.viewpos, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

        glUniformMatrix4fv(data.view_location, 1, GL_FALSE, glm::value_ptr(view));
    }
}


struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
};


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

            return 1;
        }

        spdlog::info("Created main window");

#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        glfwMakeContextCurrent(window.get());
        spdlog::info("Initialized OpenGL context");

        glfwSetWindowSizeCallback(window.get(), window_size_callback);

        glfwSetKeyCallback(window.get(), key_callback);

        GLenum err = glewInit();
        if (err != GLEW_OK) {
            spdlog::error("Failed to initialize glew: {}", glewGetErrorString(err));

            return 1;
        }
        spdlog::info("Initialized GLEW");

#ifdef _DEBUG
        glDebugMessageCallback(debug_proc, NULL);
#endif
        glEnable(GL_DEPTH_TEST);

        vertex vertices[] = {
            {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}},
            {{0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}},
            {{0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}},

            {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}},
            {{0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}},
            {{0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}},
            {{0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}},

            {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},

            {{0.5f,  0.5f,  0.5f},  {1.0f,  0.0f,  0.0f}},
            {{0.5f,  0.5f, -0.5f},  {1.0f,  0.0f,  0.0f}},
            {{0.5f, -0.5f, -0.5f},  {1.0f,  0.0f,  0.0f}},
            {{0.5f, -0.5f, -0.5f},  {1.0f,  0.0f,  0.0f}},
            {{0.5f, -0.5f,  0.5f},  {1.0f,  0.0f,  0.0f}},
            {{0.5f,  0.5f,  0.5f},  {1.0f,  0.0f,  0.0f}},

            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}},
            {{0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}},
            {{0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}},

            {{-0.5f,  0.5f, -0.5f},  {0.0f,  1.0f,  0.0f}},
            {{0.5f,  0.5f, -0.5f},  {0.0f,  1.0f,  0.0f}},
            {{0.5f,  0.5f,  0.5f},  {0.0f,  1.0f,  0.0f}},
            {{0.5f,  0.5f,  0.5f},  {0.0f,  1.0f,  0.0f}},
            {{-0.5f,  0.5f,  0.5f},  {0.0f,  1.0f,  0.0f}},
            {{-0.5f,  0.5f, -0.5f},  {0.0f,  1.0f,  0.0f}}
        };

        uint32_t handle;
        glGenVertexArrays(1, &handle);
        vertex_array_t vao{ handle };

        glBindVertexArray(vao.get());

        glGenBuffers(1, &handle);
        buffer_t vbo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, vbo.get());

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(vertex), NULL);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(vertex), (const void *)(sizeof(glm::vec3)));

        program_t program = load_program("vertex.glsl", "fragment.glsl");
        spdlog::info("Created main GLSL program");

        run_main_loop(window.get(), program.get(), sizeof(vertices) / (6 * sizeof(float)));
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
        spdlog::warn("Uniform {} was not found in the program", uniform_name);

    return location;
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

    glm::vec3 viewpos{-10.0f, 2.5f, 5.0};
    
    glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

    int lightpos_location = get_location(program, "u_lightpos");

    glm::vec3 light_pos{-2000.0f, 500.0f, 1000.0f};
    glUniform3fv(lightpos_location, 1, glm::value_ptr(light_pos));

    glm::mat4 view = glm::lookAt(viewpos, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

    glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

    struct key_callback_data key_data{view_location, viewpos_location, viewpos};

    glfwSetWindowUserPointer(window, &key_data);

    std::chrono::microseconds dt = 0us;
    while (!glfwWindowShouldClose(window)) {
        auto start_frame_ts = std::chrono::high_resolution_clock::now();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model = glm::rotate(glm::radians(180.0f), glm::vec3{ 1.0f, 0.0f, 0.0f }) * glm::rotate(glm::radians(180.0f), glm::vec3{ 0.0f, 0.0f, 1.0f });

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

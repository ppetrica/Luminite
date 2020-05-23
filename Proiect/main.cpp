#include "shader.h"
#include "loader.h"
#include "cube.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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


    }
}


struct imgui_context_t {
    imgui_context_t(GLFWwindow *window, const char *glsl_version) {
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, false);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    ~imgui_context_t() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};


int main() {
    try {
#ifdef _DEBUG
        spdlog::set_level(spdlog::level::debug);
#endif
        glfw_t glfw;
        spdlog::info("Initialized GLFW");

        window_t window{ glfwCreateWindow(1080, 720, "Hello OpenGL", NULL, NULL), glfwDestroyWindow };
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

        imgui_context_t imgui{window.get(), "#version 330"};

        ImGui::StyleColorsDark();

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

    glm::vec3 viewpos{5.0f, 2.5f, 10.0f};
    
    glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

    int lightdir_location = get_location(program, "u_lightpos");

    glm::vec3 lightpos{3.0, 2.5, 4.0};
    glUniform3fv(lightdir_location, 1, glm::value_ptr(lightpos));

    glm::mat4 view = glm::lookAt(viewpos, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

    glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

    struct key_callback_data key_data{view_location, viewpos_location, viewpos};

    int cubecolor_location = get_location(program, "u_cubecolor");

    glfwSetWindowUserPointer(window, &key_data);

    glm::vec3 clear_color {0.0f};
    glm::vec3 cube_color{ 1.0f };

    glm::vec3 cube_position { 0.0f };
    glm::vec3 cube_rotation { 0.0f };
    glm::vec3 cube_scale{ 1.0f };

    std::chrono::microseconds dt = 0us;
    while (!glfwWindowShouldClose(window)) {
        auto start_frame_ts = std::chrono::high_resolution_clock::now();

        glfwPollEvents();

        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Hello, world!");

        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        
        ImGui::DragFloat3("cube position", (float*)&cube_position);

        ImGui::DragFloat3("cube scale", (float*)&cube_scale);

        ImGui::DragFloat3("cube rotation", (float*)&cube_rotation);

        ImGui::ColorEdit3("cube color", (float*)&cube_color);

        ImGui::DragFloat3("light position", (float*)&lightpos);

        ImGui::DragFloat3("viewer position", (float*)&viewpos);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        glUniform3fv(lightdir_location, 1, glm::value_ptr(lightpos));

        glUniform3fv(cubecolor_location, 1, glm::value_ptr(cube_color));

        glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

        glm::mat4 view = glm::lookAt(viewpos, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

        glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

        /* First cube */
        {
            model = glm::translate(cube_position)
                * glm::rotate(glm::radians(cube_rotation.x), glm::vec3{ 1.0f, 0.0f, 0.0f })
                * glm::rotate(glm::radians(cube_rotation.y), glm::vec3{ 0.0f, 1.0f, 0.0f })
                * glm::rotate(glm::radians(cube_rotation.z), glm::vec3{ 0.0f, 1.0f, 1.0f })
                * glm::scale(cube_scale);

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

            normal_matrix = glm::transpose(glm::inverse(model));

            glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

            glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        }

        /* Second cube */
        {
            model = glm::translate(-cube_position)
                * glm::rotate(glm::radians(-cube_rotation.x), glm::vec3{ 1.0f, 0.0f, 0.0f })
                * glm::rotate(glm::radians(-cube_rotation.y), glm::vec3{ 0.0f, 1.0f, 0.0f })
                * glm::rotate(glm::radians(-cube_rotation.z), glm::vec3{ 0.0f, 1.0f, 1.0f })
                * glm::scale(cube_scale);

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

            normal_matrix = glm::transpose(glm::inverse(model));

            glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

            glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        }

        /* Light cube */
        {
            model = glm::translate(lightpos) * glm::scale(glm::vec3{ 0.3 });

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

            normal_matrix = glm::transpose(glm::inverse(model));

            glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

            glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        }
        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_frame_ts);
    }
}

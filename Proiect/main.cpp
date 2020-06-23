#include "shader.h"
#include "loader.h"
#include "cube.h"
#include "euler_angle.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdio>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <chrono>


void run_main_loop(GLFWwindow* window, uint32_t cube_vao, uint32_t program,
                   uint32_t light_program, uint32_t n_vertices, uint32_t ferrari_vao,
                   uint32_t ferrari_program, std::vector<unsigned int> &ferrari_indices);


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
    glm::vec3 &forward;

    bool mouse_enabled;
    glm::vec3 last_forward;

    double &last_xpos;
    double &last_ypos;
};


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        spdlog::debug("Pressed key {}", key);

        struct key_callback_data &data = *(struct key_callback_data *)glfwGetWindowUserPointer(window);
        
        glm::vec3 xvec{ 0.0f };
        glm::vec3 yvec{ 0.0f };
        glm::vec3 zvec{ 0.0f };

        switch (key) {
        case GLFW_KEY_SPACE:
            yvec = glm::vec3{ 0, 0.3f, 0 };
            break;
        case GLFW_KEY_X:
            yvec = glm::vec3{ 0, -0.3f, 0 };
            break;
        case GLFW_KEY_A:
            xvec = 0.1f * glm::normalize(glm::cross(data.forward, glm::vec3{ 0, 1, 0 }));
            break;
        case GLFW_KEY_D:
            xvec = -0.1f * glm::normalize(glm::cross(data.forward, glm::vec3{ 0, 1, 0 }));
            break;
        case GLFW_KEY_W:
            zvec = 0.3f * data.forward;
            break;
        case GLFW_KEY_S:
            zvec = -0.3f * data.forward;
            break;
        case GLFW_KEY_ESCAPE:
            if (data.mouse_enabled) {
                data.last_forward = data.forward;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwGetCursorPos(window, &data.last_xpos, &data.last_ypos);

                int width, height;
                glfwGetWindowSize(window, &width, &height);
                data.last_xpos /= width;
                data.last_ypos /= height;
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            data.mouse_enabled ^= 1;
        }

        glm::vec3 dif = yvec - xvec + zvec;
        data.viewpos += dif;
    }
}

texture_t load_texture(const std::string &path, unsigned int slot, bool invert) {
    GLuint ferrari_texture;

    glGenTextures(1, &ferrari_texture);

    glActiveTexture(slot);
    glBindTexture(GL_TEXTURE_2D, ferrari_texture);

    if (invert) {
        stbi_set_flip_vertically_on_load(1);
    }
    else {
        stbi_set_flip_vertically_on_load(0);
    }

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
            GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        throw std::exception("Failed to load texture");
    }
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIPMAP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIPMAP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture_t{ ferrari_texture };
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


struct transform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::mat4 to_model() const {
        return glm::translate(position)
            * glm::rotate(glm::radians(rotation.x), glm::vec3{ 1.0f, 0.0f, 0.0f })
            * glm::rotate(glm::radians(rotation.y), glm::vec3{ 0.0f, 1.0f, 0.0f })
            * glm::rotate(glm::radians(rotation.z), glm::vec3{ 0.0f, 1.0f, 1.0f })
            * glm::scale(scale);
    }
};

struct model {
    uint32_t vao;
    uint32_t shader;
    uint32_t n_points;
};


static inline int get_location(uint32_t program, const char *uniform_name) {
    int location = glGetUniformLocation(program, uniform_name);
    if (location == -1)
        spdlog::warn("Uniform {} was not found in the program", uniform_name);

    return location;
}


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
        vertex_array_t cube_vao{ handle };

        glBindVertexArray(cube_vao.get());

        glGenBuffers(1, &handle);
        buffer_t cube_vbo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo.get());

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(vertex), NULL);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(vertex), (const void *)(sizeof(glm::vec3)));

        program_t program = load_program("vertex.glsl", "fragment.glsl");
        spdlog::info("Created main GLSL program");

        program_t light_program = load_program("lightVertex.glsl", "lightFragment.glsl");
        spdlog::info("Created light program");

        /* Loading and creating ferrari model */
        auto [ferrari_vertices, ferrari_indices] = loader::load_asset("cube.obj");
        
        glGenVertexArrays(1, &handle);
        vertex_array_t ferrari_vao{ handle };

        glBindVertexArray(ferrari_vao.get());

        glGenBuffers(1, &handle);
        buffer_t ferrari_vbo{ handle };

        glGenBuffers(1, &handle);
        buffer_t ferrari_ibo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, ferrari_vbo.get());

        glBufferData(GL_ARRAY_BUFFER, ferrari_vertices.size() * sizeof(ferrari_vertices[0]), ferrari_vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(loader::vertex), NULL);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(loader::vertex), (const void *)(sizeof(glm::vec3)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, 0, sizeof(loader::vertex), (const void *)(2 * sizeof(glm::vec3)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ferrari_ibo.get());
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ferrari_indices.size() * sizeof(ferrari_indices[0]), ferrari_indices.data(), GL_STATIC_DRAW);

        program_t ferrari_program = load_program("ferrariVertex.glsl", "ferrariFragment.glsl");


        texture_t texture = load_texture("brickwall.jpg", GL_TEXTURE0, false);

        glUniform1i(get_location(ferrari_program.get(), "u_ferrari_tex"), 0);

        run_main_loop(window.get(), cube_vao.get(), program.get(), light_program.get(), sizeof(vertices) / (6 * sizeof(float)),
                      ferrari_vao.get(), ferrari_program.get(), ferrari_indices);
    } catch (const std::exception &ex) {
        spdlog::error("{}", ex.what());

        return 1;
    } catch (...) {
        spdlog::error("An exception has occured.");
        
        return 1;
    }
}


void run_main_loop(GLFWwindow* window, uint32_t cube_vao, uint32_t program, uint32_t light_program, uint32_t n_vertices, uint32_t ferrari_vao,
                   uint32_t ferrari_program, std::vector<unsigned int> &ferrari_indices) {
    using namespace std::chrono_literals;

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    int model_location = get_location(program, "u_model");
    int normal_location = get_location(program, "u_normal");
    int proj_location = get_location(program, "u_proj");
    int view_location = get_location(program, "u_view");
    int viewpos_location = get_location(program, "u_viewpos");
    int lightpos_location = get_location(program, "u_lightpos");
    int cubecolor_location = get_location(program, "u_cubecolor");

    int light_model_location = get_location(light_program, "u_model");
    int light_proj_location = get_location(light_program, "u_proj");
    int light_view_location = get_location(light_program, "u_view");

    int ferrari_model_location = get_location(ferrari_program, "u_model");
    int ferrari_proj_location = get_location(ferrari_program, "u_proj");
    int ferrari_view_location = get_location(ferrari_program, "u_view");
    int ferrari_viewpos_location = get_location(program, "u_viewpos");
    int ferrari_lightpos_location = get_location(program, "u_lightpos");
    int ferrari_normal_location = get_location(program, "u_normal");

    glUseProgram(ferrari_program);

    glUniformMatrix4fv(ferrari_proj_location, 1, GL_FALSE, glm::value_ptr(projection));
    
    glUseProgram(light_program);

    glUniformMatrix4fv(light_proj_location, 1, GL_FALSE, glm::value_ptr(projection));
    
    glUseProgram(program);

    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 viewpos{0.0f, 2.0f, -13.0f};
    
    glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

    glm::vec3 lightpos{3.0, -0.5, 3.0};
    
    glUniform3fv(lightpos_location, 1, glm::value_ptr(lightpos));

    double last_xpos, last_ypos;
    glfwGetCursorPos(window, &last_xpos, &last_ypos);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glm::vec3 forward{ 1, 0, 0 };
    struct key_callback_data key_data{view_location, viewpos_location, viewpos, forward, false, forward, last_xpos, last_ypos};

    glfwSetWindowUserPointer(window, &key_data);

    glm::vec3 clear_color{ 0.0f };
    glm::vec3 cube_color{ 1.0f, 0.5f, 0.0f };

    transform cube{ glm::vec3{0.5f}, glm::vec3{0.0f}, glm::vec3{1.0f} };
    transform ferrari{ glm::vec3{0.0f, -4.0f, 0.0f}, glm::vec3{0.0f, 67.0f, 0.0f}, glm::vec3{0.015f} };

    last_xpos /= width;
    last_ypos /= height;

    euler_angle eangle{ 0.0f, 90.0f, 0.0f};

    const float rotationSensitivity = 0.02;

    std::chrono::microseconds dt = 0us;
    while (!glfwWindowShouldClose(window)) {
        auto start_frame_ts = std::chrono::high_resolution_clock::now();
        
        if (!key_data.mouse_enabled) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            xpos /= width;
            ypos /= height;

            double dx = xpos - last_xpos;
            double dy = -(ypos - last_ypos);

            last_xpos = xpos;
            last_ypos = ypos;

            eangle.pitch += dy * rotationSensitivity * dt.count();
            eangle.yaw += dx * rotationSensitivity * dt.count();

            eangle.normalize();

            forward = eangle.to_vector();
        }
        
        glm::mat4 view = glm::lookAt(viewpos, viewpos + forward, glm::vec3{ 0, 1, 0 });

        glfwPollEvents();

        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Hello, world!");

        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        
        ImGui::DragFloat3("cube position", (float*)&cube.position, 0.1f);

        ImGui::DragFloat3("cube scale", (float*)&cube.scale, 0.1f);

        ImGui::DragFloat3("cube rotation", (float*)&cube.rotation);

        ImGui::ColorEdit3("cube color", (float*)&cube_color);

        ImGui::DragFloat3("light position", (float*)&lightpos, 0.1f);

        ImGui::DragFloat3("viewer position", (float*)&viewpos, 0.1f);
        
        ImGui::DragFloat3("ferrari position", (float*)&ferrari.position, 0.1f);

        ImGui::DragFloat3("ferrari scale", (float*)&ferrari.scale, 0.01f);

        ImGui::DragFloat3("ferrari rotation", (float*)&ferrari.rotation);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        glBindVertexArray(cube_vao);

        glUniform3fv(lightpos_location, 1, glm::value_ptr(lightpos));

        glUniform3fv(cubecolor_location, 1, glm::value_ptr(cube_color));

        glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

        glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

        /* First cube */
        {
            auto model = cube.to_model();

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

            auto normal_matrix = glm::transpose(glm::inverse(model));

            glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

            glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        }

        /* Second cube */
        {
            transform cube2 {-cube.position, -cube.rotation, cube.scale };
            glm::vec3 cube_color2 {1.0 - cube_color.r, 1.0 - cube_color.g, 1.0 - cube_color.g};
            glUniform3fv(cubecolor_location, 1, glm::value_ptr(cube_color2));
            
            auto model = cube2.to_model();

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

            auto normal_matrix = glm::transpose(glm::inverse(model));

            glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

            glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        }

        glUseProgram(light_program);
        /* Light cube */
        {
            glUniformMatrix4fv(light_view_location, 1, GL_FALSE, glm::value_ptr(view));

            auto model = glm::translate(lightpos) * glm::scale(glm::vec3{ 0.2f });

            glUniformMatrix4fv(light_model_location, 1, GL_FALSE, glm::value_ptr(model));

            glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        }

        glUseProgram(ferrari_program);
        
        glUniform3fv(ferrari_lightpos_location, 1, glm::value_ptr(lightpos));

        glUniform3fv(ferrari_viewpos_location, 1, glm::value_ptr(viewpos));

        glUniformMatrix4fv(ferrari_view_location, 1, GL_FALSE, glm::value_ptr(view));

        /* ferrari */
        {
            glBindVertexArray(ferrari_vao);

            glUniformMatrix4fv(ferrari_view_location, 1, GL_FALSE, glm::value_ptr(view));

            auto model = ferrari.to_model();

            glUniformMatrix4fv(ferrari_model_location, 1, GL_FALSE, glm::value_ptr(model));

            auto normal_matrix = glm::transpose(glm::inverse(model));

            glUniformMatrix4fv(ferrari_normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

            glDrawElements(GL_TRIANGLES, ferrari_indices.size(), GL_UNSIGNED_INT, NULL);
        }
        glUseProgram(program);
        
        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_frame_ts);
    }
}

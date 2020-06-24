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


void run_main_loop(GLFWwindow* window, uint32_t cube_vao, uint32_t vao,
                   uint32_t program, std::vector<unsigned int> &indices);


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
    std::unordered_map<int, bool> keys;

    int view_location;
    int viewpos_location;

    glm::vec3 &viewpos;
    glm::vec3 &forward;

    bool mouse_enabled;
    glm::vec3 last_forward;

    double &last_xpos;
    double &last_ypos;
    
    std::chrono::microseconds &dt;
};


static void process_keypresses(GLFWwindow *window, key_callback_data &data) {
    glm::vec3 xvec{ 0.0f };
    glm::vec3 yvec{ 0.0f };
    glm::vec3 zvec{ 0.0f };

    if (data.keys[GLFW_KEY_SPACE])
        yvec += glm::vec3{ 0, 0.03f, 0 };

    if (data.keys[GLFW_KEY_X])
        yvec = glm::vec3{ 0, -0.03f, 0 };

    if (data.keys[GLFW_KEY_A])
        xvec += 0.01f * glm::normalize(glm::cross(data.forward, glm::vec3{ 0, 1, 0 }));
    if (data.keys[GLFW_KEY_D])
        xvec -= 0.01f * glm::normalize(glm::cross(data.forward, glm::vec3{ 0, 1, 0 }));

    if (data.keys[GLFW_KEY_W])
        zvec += 0.03f * data.forward;
    if (data.keys[GLFW_KEY_S])
        zvec -= 0.03f * data.forward;

    glm::vec3 dif = yvec - xvec + zvec;
    data.viewpos += dif;
}


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    using namespace std;
    struct key_callback_data &data = *(struct key_callback_data *)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        spdlog::debug("Pressed key {}", key);
        data.keys[key] = true;

        if (key == GLFW_KEY_ESCAPE) {
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
    } else if (action == GLFW_RELEASE) {
        spdlog::debug("Released key {}", key);
        data.keys[key] = false;
    }
}


static void process_mouse_movement(GLFWwindow *window, key_callback_data &key_data) {
    const float rotationSensitivity = 0.02f;

    static euler_angle eangle{ 0.0f, 90.0f, 0.0f};
    if (!key_data.mouse_enabled) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        xpos /= width;
        ypos /= height;

        double dx = xpos - key_data.last_xpos;
        double dy = -(ypos - key_data.last_ypos);

        key_data.last_xpos = xpos;
        key_data.last_ypos = ypos;

        eangle.pitch += (float)dy * rotationSensitivity * key_data.dt.count();
        eangle.yaw += (float)dx * rotationSensitivity * key_data.dt.count();

        eangle.normalize();

        key_data.forward = eangle.to_vector();
    }
}


texture_t load_texture(const std::string &path, unsigned int slot, bool invert) {
    GLuint texture;

    glGenTextures(1, &texture);

    glActiveTexture(slot);
    glBindTexture(GL_TEXTURE_2D, texture);

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

    return texture_t{ texture };
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

    transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) :
        position(position), rotation(rotation), scale(scale) {}

    glm::mat4 to_model() const {
        return glm::translate(position)
            * glm::rotate(glm::radians(rotation.x), glm::vec3{ 1.0f, 0.0f, 0.0f })
            * glm::rotate(glm::radians(rotation.y), glm::vec3{ 0.0f, 1.0f, 0.0f })
            * glm::rotate(glm::radians(rotation.z), glm::vec3{ 0.0f, 1.0f, 1.0f })
            * glm::scale(scale);
    }
};


void render_transform(const transform& transform, int model_location, int normal_location) {
    auto model = transform.to_model();

    glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

    auto normal_matrix = glm::transpose(glm::inverse(model));

    glUniformMatrix4fv(normal_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));
}


const size_t n_vertices = sizeof(vertices) / (6 * sizeof(float));


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

        window_t window{ glfwCreateWindow(1080, 720, "Proiect SPG", NULL, NULL), glfwDestroyWindow };
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

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, 0, sizeof(vertex), (const void *)sizeof(glm::vec3));

        /* Loading and creating ferrari model */
        auto [vertices, indices] = loader::load_asset("ferrari.obj");
        
        glGenVertexArrays(1, &handle);
        vertex_array_t vao{ handle };

        glBindVertexArray(vao.get());

        glGenBuffers(1, &handle);
        buffer_t vbo{ handle };

        glGenBuffers(1, &handle);
        buffer_t ibo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, vbo.get());

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(loader::vertex), NULL);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(loader::vertex), (const void *)(sizeof(glm::vec3)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, 0, sizeof(loader::vertex), (const void *)(2 * sizeof(glm::vec3)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.get());
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

        program_t program = load_program("vertex.glsl", "fragment.glsl");

        texture_t texture = load_texture("ferrari.png", GL_TEXTURE0, false);

        glUniform1i(get_location(program.get(), "u_tex"), 0);

        run_main_loop(window.get(), cube_vao.get(),
                      vao.get(), program.get(), indices);
    } catch (const std::exception &ex) {
        spdlog::error("{}", ex.what());

        return 1;
    } catch (...) {
        spdlog::error("An exception has occured.");
        
        return 1;
    }
}


class light {
public:
    light (uint32_t program, std::string name, glm::vec3 position = glm::vec3(0.0f))
        : program(program), name(name), position(position), ambient(0.3f), constant(2.0f), linear(0.2f), quadratic(0.01f) {
        position_loc = get_location(program, (name + ".position").c_str());
        
        ambient_loc = get_location(program, (name + ".ambient").c_str());

        constant_loc = get_location(program, (name + ".constant").c_str());
        linear_loc = get_location(program, (name + ".linear").c_str());
        quadratic_loc = get_location(program, (name + ".quadratic").c_str());
    }

    void update() {
        glUseProgram(program);

        glUniform3fv(position_loc, 1, glm::value_ptr(position));
        
        glUniform3fv(ambient_loc, 1, glm::value_ptr(ambient));
        
        glUniform1f(constant_loc, constant);
        glUniform1f(linear_loc, linear);
        glUniform1f(quadratic_loc, quadratic);
    }

    void draw(int model_location) {
        glUseProgram(program);
        
        auto model = glm::translate(position) * glm::scale(glm::vec3{ 0.05f });

        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, n_vertices);
    }

    const uint32_t program;
    const std::string name;

    glm::vec3 position;
    glm::vec3 ambient;

    float constant;
    float linear;
    float quadratic;

private:
    int position_loc;
    int ambient_loc;
    int constant_loc;
    int linear_loc;
    int quadratic_loc;
};


void run_main_loop(GLFWwindow* window, uint32_t cube_vao, uint32_t vao,
                   uint32_t program, std::vector<unsigned int> &indices) {
    using namespace std::chrono_literals;

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    int model_location = get_location(program, "u_model");
    int view_location = get_location(program, "u_view");
    int viewpos_location = get_location(program, "u_viewpos");
    int normal_location = get_location(program, "u_normal");
    
    int light_color_location = get_location(program, "u_light_color");

    int cubecolor_location = get_location(program, "u_color");

    glm::vec3 light_color{ 1.0f, 0.7f, 0.8f };

    std::vector<light> lights;
    std::vector<transform> ferraris;

    const int n_copies = 5;
    for (int i = 0; i < n_copies; ++i) {
        float degrees = i * 360 / n_copies;
        float radians = glm::radians(degrees);
        float dl = 10;
        float df = 20;

        float c = cos(radians);
        float s = sin(radians);
        lights.emplace_back(program, "u_light[" + std::to_string(i) + "]", glm::vec3{ dl * c, 1, dl * s });

        ferraris.emplace_back(glm::vec3{df * c, -4.0f, df * s}, glm::vec3{0.0f, -degrees, 0.0f}, glm::vec3{0.015f});
    }

    glUseProgram(program);

    /* Set up projection, it won't change for the rest of the program */
    int proj_location = get_location(program, "u_proj");
    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(projection));

    int type_location = get_location(program, "u_type");
    
    int n_lights_location = get_location(program, "u_n_lights");

    /* Initial viewer position */
    glm::vec3 viewpos{0.0f, 60.0f, 0.0f};
    glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

    double last_xpos, last_ypos;
    glfwGetCursorPos(window, &last_xpos, &last_ypos);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::chrono::microseconds dt = 0us;

    glm::vec3 forward{ 0, 0, 1 };
    struct key_callback_data key_data{
        {},
        view_location,
        viewpos_location,
        viewpos,
        forward,
        false,
        forward,
        last_xpos,
        last_ypos,
        dt
    };

    glfwSetWindowUserPointer(window, &key_data);

    glm::vec3 clear_color{ 0.0f };

    transform cube{ glm::vec3{0.5f}, glm::vec3{0.0f}, glm::vec3{1.0f} };
   
    last_xpos /= width;
    last_ypos /= height;

    while (!glfwWindowShouldClose(window)) {
        auto start_frame_ts = std::chrono::high_resolution_clock::now();

        glm::mat4 view = glm::lookAt(viewpos, viewpos + forward, glm::vec3{ 0, 1, 0 });

        glfwPollEvents();

        process_keypresses(window, key_data);
        process_mouse_movement(window, key_data);

        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("ImGui - best GUI library");

        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        
        ImGui::ColorEdit3("light color", (float*)&light_color, 0.1f);

        ImGui::DragFloat3("viewer position", (float*)&viewpos, 0.1f);

        ImGui::DragFloat3("viewer position", (float*)&viewpos, 0.1f);

        for (int i = 0; i < ferraris.size(); ++i) {
            std::string label = std::string{ "ferrari " } +std::to_string(i);
            ImGui::Text(label.c_str());

            ImGui::DragFloat3(("f_position_" + std::to_string(i)).c_str(), (float*)&ferraris[i].position, 0.1f);
            ImGui::DragFloat3(("f_scale_" + std::to_string(i)).c_str(), (float*)&ferraris[i].scale, 0.001f);
            ImGui::DragFloat3(("f_rotation_" + std::to_string(i)).c_str(), (float*)&ferraris[i].rotation);
        }

        for (int i = 0; i < lights.size(); ++i) {
            std::string label = std::string{ "light " } +std::to_string(i);
            ImGui::Text(label.c_str());

            ImGui::DragFloat3(("l_position_" + std::to_string(i)).c_str(), (float*)&lights[i].position, 0.1f);
            ImGui::DragFloat(("l_constant_" + std::to_string(i)).c_str(), &lights[i].constant, 0.01f);
            ImGui::DragFloat(("l_linear_" + std::to_string(i)).c_str(), &lights[i].linear, 0.001f);
            ImGui::DragFloat(("l_quadratic_" + std::to_string(i)).c_str(), &lights[i].quadratic, 0.0001f);
        }

        if (ImGui::Button("+ light")) {
            lights.push_back({
                program,
                std::string{"u_light[" + std::to_string(lights.size()) + "]"}
             });
        }

        if (ImGui::Button("- light")) {
            lights.pop_back();
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        glBindVertexArray(cube_vao);

        glUseProgram(program);
        
        glUniform3fv(light_color_location, 1, glm::value_ptr(light_color));

        glUniform1i(n_lights_location, lights.size());

        glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

        glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

        glUniform1i(type_location, 1);

        glm::vec3 platform_color{ 0.7f, 0.7f, 0.7f };
        glUniform3fv(cubecolor_location, 1, glm::value_ptr(platform_color));

        glBindVertexArray(cube_vao);
        transform platform{ {0.0f, -4.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {100.0f, 0.1f, 100.0f} };
        render_transform(platform, model_location, normal_location);
        glDrawArrays(GL_TRIANGLES, 0, n_vertices);

        glUniform1i(type_location, 2);

        for (int i = 0; i < lights.size(); ++i) {
            lights[i].update();
            lights[i].draw(model_location);
        }

        glUniform1i(type_location, 0);

        glBindVertexArray(vao);

        for (int i = 0; i < ferraris.size(); ++i) {
            render_transform(ferraris[i], model_location, normal_location);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL);
        }
        
        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_frame_ts);
    }
}

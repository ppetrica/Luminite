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


/* Codul asta nu e cel mai bun pe care l-am scris ... dar nici nu-i cel mai rau */

const size_t n_vertices = sizeof(vertices) / (6 * sizeof(float));


struct user_input_data {
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


static inline int get_location(uint32_t program, const char *uniform_name) {
    int location = glGetUniformLocation(program, uniform_name);
    if (location == -1)
        spdlog::warn("Uniform {} was not found in the program", uniform_name);

    return location;
}


class light {
public:
    light (uint32_t program, std::string name, glm::vec3 position = glm::vec3(0.0f), glm::vec3 color = glm::vec3(1.0f))
        : program(program), name(name), position(position), ambient(0.3f), color(color), constant(2.0f), linear(0.2f), quadratic(0.01f) {
        position_loc = get_location(program, (name + ".position").c_str());

        ambient_loc = get_location(program, (name + ".ambient").c_str());

        constant_loc = get_location(program, (name + ".constant").c_str());
        linear_loc = get_location(program, (name + ".linear").c_str());
        quadratic_loc = get_location(program, (name + ".quadratic").c_str());

        color_loc = get_location(program, (name + ".color").c_str());

        light_color_location = get_location(program, "u_light_color");
    }

    void update() {
        glUseProgram(program);

        glUniform3fv(position_loc, 1, glm::value_ptr(position));

        glUniform3fv(ambient_loc, 1, glm::value_ptr(ambient));

        glUniform3fv(color_loc, 1, glm::value_ptr(color));

        glUniform1f(constant_loc, constant);
        glUniform1f(linear_loc, linear);
        glUniform1f(quadratic_loc, quadratic);
    }

    void draw(int model_location) {
        glUseProgram(program);

        glUniform3fv(light_color_location, 1, glm::value_ptr(color));

        auto model = glm::translate(position) * glm::scale(glm::vec3{ 0.2f });

        glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, 0, n_vertices);
    }

    const uint32_t program;
    const std::string name;

    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 color;

    float constant;
    float linear;
    float quadratic;

private:
    int position_loc;
    int ambient_loc;
    int constant_loc;
    int linear_loc;
    int quadratic_loc;
    int color_loc;
    int light_color_location;
};


void run_main_loop(GLFWwindow* window, uint32_t program, uint32_t cube_vao, uint32_t vao,
                   uint32_t ibo, std::vector<unsigned int> &indices, uint32_t tree_vao, uint32_t tree_ibo, std::vector<unsigned int> &tree_ind);


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


static void process_keypresses(GLFWwindow *window, user_input_data &data) {
    glm::vec3 xvec{ 0.0f };
    glm::vec3 yvec{ 0.0f };
    glm::vec3 zvec{ 0.0f };

    if (data.keys[GLFW_KEY_SPACE])
        yvec += glm::vec3{ 0, 0.1f, 0 };

    if (data.keys[GLFW_KEY_X])
        yvec = glm::vec3{ 0, -0.1f, 0 };

    if (data.keys[GLFW_KEY_A])
        xvec += 0.1f * glm::normalize(glm::cross(data.forward, glm::vec3{ 0, 1, 0 }));
    if (data.keys[GLFW_KEY_D])
        xvec -= 0.1f * glm::normalize(glm::cross(data.forward, glm::vec3{ 0, 1, 0 }));

    if (data.keys[GLFW_KEY_W])
        zvec += 0.3f * data.forward;
    if (data.keys[GLFW_KEY_S])
        zvec -= 0.3f * data.forward;

    glm::vec3 dif = yvec - xvec + zvec;
    data.viewpos += dif * ((float)data.dt.count() / 10000);
}


static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    using namespace std;
    struct user_input_data &data = *(struct user_input_data *)glfwGetWindowUserPointer(window);

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


static void process_mouse_movement(GLFWwindow *window, user_input_data &key_data) {
    const float rotationSensitivity = 0.008f;

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


int main() {
    try {
#ifdef _DEBUG
        spdlog::set_level(spdlog::level::debug);
#endif
        srand(time(NULL));

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

        auto [tree_vert, tree_ind] = loader::load_asset("new_tree2.obj");

        glGenVertexArrays(1, &handle);
        vertex_array_t tree_vao{ handle };

        glBindVertexArray(tree_vao.get());

        glGenBuffers(1, &handle);
        buffer_t tree_vbo{ handle };

        glGenBuffers(1, &handle);
        buffer_t tree_ibo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, tree_vbo.get());

        glBufferData(GL_ARRAY_BUFFER, tree_vert.size() * sizeof(tree_vert[0]), tree_vert.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(loader::vertex), NULL);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(loader::vertex), (const void *)(sizeof(glm::vec3)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, 0, sizeof(loader::vertex), (const void *)(2 * sizeof(glm::vec3)));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tree_ibo.get());

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, tree_ind.size() * sizeof(tree_ind[0]), tree_ind.data(), GL_STATIC_DRAW);

        texture_t tree_tex = load_texture("tree.jpg", GL_TEXTURE1, false);

        run_main_loop(window.get(), program.get(), cube_vao.get(),
                      vao.get(), ibo.get(), indices, tree_vao.get(), tree_ibo.get(), tree_ind);
    } catch (const std::exception &ex) {
        spdlog::error("{}", ex.what());

        return 1;
    } catch (...) {
        spdlog::error("An exception has occured.");
        
        return 1;
    }
}


void run_main_loop(GLFWwindow* window, uint32_t program, uint32_t cube_vao, uint32_t vao,
    uint32_t ibo, std::vector<unsigned int> &indices, uint32_t tree_vao, uint32_t tree_ibo, std::vector<unsigned int> &tree_ind) {
    using namespace std::chrono_literals;

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

    int model_location = get_location(program, "u_model");
    int view_location = get_location(program, "u_view");
    int viewpos_location = get_location(program, "u_viewpos");
    int normal_location = get_location(program, "u_normal");
    
    int cubecolor_location = get_location(program, "u_color");

    transform tree{ glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.1f} };
    std::vector<float> langles;
    std::vector<light> lights;
    std::vector<float> fangles;
    std::vector<transform> ferraris;

    float center_light_pos_y = -3;

    const int n_copies = 5;
    for (int i = 0; i < n_copies; ++i) {
        float degrees = i * 360 / n_copies;
        float radians = glm::radians(degrees);
        float dl = 10;
        float df = 20;

        float c = cos(radians);
        float s = sin(radians);

        fangles.push_back(degrees);
        langles.push_back(degrees);

        lights.emplace_back(program, "u_light[" + std::to_string(i) + "]", glm::vec3{ dl * c, -3, dl * s }, glm::vec3{ (c + 1.5) / 2, (s + 1.5) / 2, 0.5f });

        ferraris.emplace_back(glm::vec3{df * c, -4.0f, df * s}, glm::vec3{0.0f, -degrees, 0.0f}, glm::vec3{0.015f});
    }

    lights.emplace_back(program, "u_light[" + std::to_string(n_copies) + "]", glm::vec3{0.0f, center_light_pos_y, 0.0f});

    float tree_bottom = -3;
    float tree_top = 10;
    float bottom_radius = 6;
    float top_radius = 0.5;
    int floors = 7;
    float total_y = tree_top - tree_bottom;
    float total_radius = bottom_radius - top_radius;
    int globes_per_floor = 8;
    for (int i = 0; i < floors; ++i) {
        float y = tree_bottom + i * total_y / floors;
        float radius = bottom_radius - i * total_radius / floors;

        for (int i = 0; i < 8; ++i) {
            float degrees = ((float)rand() / RAND_MAX) * 360.0f;
            float radians = glm::radians(degrees);

            float c = cos(radians);
            float s = sin(radians);

            light l{program, "u_light[" + std::to_string(lights.size()) + "]", glm::vec3{ radius * c, y, radius * s }, glm::vec3{ (c + 1.5) / 2, (s + 1.5) / 2, 0.5f }};
            l.constant = 0.0f;
            l.linear = 0.0f;
            l.quadratic = 5.0f;

            lights.push_back(l);
        }
    }

    glUseProgram(program);

    /* Set up projection, it won't change for the rest of the program */
    int proj_location = get_location(program, "u_proj");
    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(projection));

    int type_location = get_location(program, "u_type");
    
    int n_lights_location = get_location(program, "u_n_lights");

    /* Initial viewer position */
    glm::vec3 viewpos{4.0f, 54.0f, -48.0f};
    glUniform3fv(viewpos_location, 1, glm::value_ptr(viewpos));

    double last_xpos, last_ypos;
    glfwGetCursorPos(window, &last_xpos, &last_ypos);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::chrono::microseconds dt = 0us;

    glm::vec3 forward{ 0, 0, 1 };
    struct user_input_data key_data{
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

    bool start = true;

    float x = 0;

    int texture_location = get_location(program, "u_tex");

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
        
        ImGui::DragFloat3("viewer position", (float*)&viewpos, 0.1f);

        if (ImGui::Button("start / stop")) {
            start ^= 1;
        }

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

            ImGui::ColorEdit3(("l_color_" + std::to_string(i)).c_str(), (float*)&lights[i].color, 0.1f);
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

        for (int i = 0; i < lights.size(); ++i) {
            lights[i].update();
        }

        glUniform1i(type_location, 2);

        for (int i = 0; i < lights.size(); ++i) {
            lights[i].draw(model_location);
        }

        glUniform1i(type_location, 0);

        glBindVertexArray(vao);

        glUniform1i(texture_location, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

        for (int i = 0; i < ferraris.size(); ++i) {
            render_transform(ferraris[i], model_location, normal_location);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL);
        }

        glUniform1i(texture_location, 1);
        
        glBindVertexArray(tree_vao);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tree_ibo);

        render_transform(tree, model_location, normal_location);
        glDrawElements(GL_TRIANGLES, tree_ind.size(), GL_UNSIGNED_INT, NULL);

        if (start) {
            for (int i = 0; i < ferraris.size(); ++i) {
                fangles[i] += 0.03 * dt.count() / 10000;

                float radians = glm::radians(fangles[i]);

                float dl = 10;
                float df = 20;

                float c = cos(radians);
                float s = sin(radians);

                ferraris[i].position = glm::vec3{df * c, -4.0f, df * s};

                ferraris[i].rotation.y = -fangles[i];

                langles[i] -= 0.1 * dt.count() / 10000;

                radians = glm::radians(langles[i]);

                c = cos(radians);
                s = sin(radians);

                if (i < lights.size())
                    lights[i].position = glm::vec3{dl * c, -3.0f, dl * s};
            }

            if (lights.size() > ferraris.size()) {
                x += 0.000001f * dt.count();

                int i = ferraris.size();

                lights[i].position.y = center_light_pos_y + 30 * (sin(x) + 1);
            }
        }

        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_frame_ts);
    }
}

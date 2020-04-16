#include "wrappers.h"
#include <stdio.h>
#include <string>
#include <memory>
#include <optional>


#ifdef _DEBUG
void APIENTRY debug_proc(GLenum source,
                         GLenum type,
                         GLuint id,
                         GLenum severity,
                         GLsizei length,
                         const GLchar* message,
                         const void* userParam) {
    fprintf(stderr, "[%d] %d generated error %x: %s", severity, source, type, message);
}
#endif // _DEBUG


std::optional<std::string> read_file(const std::string &filepath) {
    std::unique_ptr<FILE, int(*)(FILE*)> file{ fopen(filepath.c_str(), "r"), fclose };
    if (!file)
        return std::nullopt;

    bool ret = true;
    if (fseek(file.get(), 0, SEEK_END) < 0) return std::nullopt;

    int length = ftell(file.get());
    if (length <= 0)
        return std::nullopt;

    rewind(file.get());

    std::string buf;
    buf.resize(length);

    int read;
    int total_read = 0;
    while (read = fread(buf.data() + total_read, 1, length - total_read, file.get()))
        total_read += read;

    return buf;
}


std::optional<shader_t> create_shader(const std::string &filepath, uint32_t type) {
    std::optional<std::string> source_opt = read_file(filepath);
    if (!source_opt.has_value()) return std::nullopt;

    std::string source = *source_opt;

    uint32_t handle = glCreateShader(type);

    shader_t shader{handle};

    const char *csource = source.c_str();

    glShaderSource(shader.get(), 1, (const char * const *)&csource, NULL);

    glCompileShader(shader.get());

    return shader;
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

        glDebugMessageCallback(debug_proc, NULL);

        uint32_t handle;
        glGenVertexArrays(1, &handle);
        vertex_array_t vao{ handle };

        glBindVertexArray(vao.get());

        glGenBuffers(1, &handle);
        buffer_t vbo{ handle };

        glBindBuffer(GL_ARRAY_BUFFER, vbo.get());

        glBufferData(GL_ARRAY_BUFFER, sizeof(coords), &coords[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, 0, sizeof(coords[0]), NULL);

        auto shader_opt = create_shader("vertex.glsl", GL_VERTEX_SHADER);
        if (!shader_opt.has_value()) {
            fprintf(stderr, "Failed to read vertex shader.\n");
            return 1;
        }

        auto vertex_shader = std::move(*shader_opt);

        int length;
        char error_log[1024];
        glGetShaderiv(vertex_shader.get(), GL_INFO_LOG_LENGTH, &length);
        if (length) {
            glGetShaderInfoLog(vertex_shader.get(), sizeof(error_log), NULL, &error_log[0]);
            fprintf(stderr, "Failed to compile vertex shader: %s", error_log);
            return 1;
        }

        shader_opt = create_shader("fragment.glsl", GL_FRAGMENT_SHADER);
        if (!shader_opt) {
            fprintf(stderr, "Failed to read fragment shader.\n");
            return 1;
        }

        auto fragment_shader = std::move(*shader_opt);

        glGetShaderiv(fragment_shader.get(), GL_INFO_LOG_LENGTH, &length);
        if (length) {
            glGetShaderInfoLog(fragment_shader.get(), sizeof(error_log), NULL, &error_log[0]);
            fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
            return 1;
        }

        handle = glCreateProgram();
        program_t program{ handle };

        glAttachShader(program.get(), vertex_shader.get());
        glAttachShader(program.get(), fragment_shader.get());

        glLinkProgram(program.get());

        glGetProgramiv(program.get(), GL_INFO_LOG_LENGTH, &length);
        if (length) {
            glGetProgramInfoLog(program.get(), sizeof(error_log), NULL, &error_log[0]);
            fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
            return 1;
        }

        glUseProgram(program.get());

        while (!glfwWindowShouldClose(window.get())) {
            glDrawArrays(GL_TRIANGLES, 0, sizeof(coords) / sizeof(coords[0]));

            glfwSwapBuffers(window.get());
            glfwPollEvents();
        }
    } catch (const std::exception &ex) {
        fprintf(stderr, "%s\n", ex.what());
    } catch (...) {
        fprintf(stderr, "An exception has occured.\n");
    }
}

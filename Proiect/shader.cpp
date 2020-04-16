#include "shader.h"
#include <filesystem>
#include <fstream>


static std::string read_file(std::filesystem::path path) {
    const auto sz = std::filesystem::file_size(path);
    
    std::ifstream f{ path };

    std::string result((uint32_t)sz, ' ');

    f.read(result.data(), sz);

    return result;
}


static shader_t create_shader(const std::string &filepath, uint32_t type) {
    std::string source = read_file(filepath);

    shader_t shader{glCreateShader(type)};

    const char *csource = source.c_str();

    glShaderSource(shader.get(), 1, (const char * const *)&csource, NULL);

    glCompileShader(shader.get());

    return shader;
}


program_t load_program(const std::string &vertex_path, const std::string &fragment_path) {
    auto vertex_shader = create_shader(vertex_path, GL_VERTEX_SHADER);

    int length;
    char error_log[1024];
    glGetShaderiv(vertex_shader.get(), GL_INFO_LOG_LENGTH, &length);
    if (length) {
        glGetShaderInfoLog(vertex_shader.get(), sizeof(error_log), NULL, &error_log[0]);

        throw invalid_shader_exception(GL_VERTEX_SHADER,
            std::string{"Failed to compile vertex shader: "} + error_log);
    }

    auto fragment_shader = create_shader(fragment_path, GL_FRAGMENT_SHADER);

    glGetShaderiv(fragment_shader.get(), GL_INFO_LOG_LENGTH, &length);
    if (length) {
        glGetShaderInfoLog(fragment_shader.get(), sizeof(error_log), NULL, &error_log[0]);
        fprintf(stderr, "Failed to compile fragment shader: %s", error_log);
    }

    program_t program{ glCreateProgram()};

    glAttachShader(program.get(), vertex_shader.get());
    glAttachShader(program.get(), fragment_shader.get());

    glLinkProgram(program.get());

    glGetProgramiv(program.get(), GL_INFO_LOG_LENGTH, &length);
    if (length) {
        glGetProgramInfoLog(program.get(), sizeof(error_log), NULL, &error_log[0]);
        throw invalid_shader_exception(GL_FRAGMENT_SHADER,
            std::string{"Failed to compile fragment shader: "} + error_log);
    }

    glUseProgram(program.get());

    return program;
}

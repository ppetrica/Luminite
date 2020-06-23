#pragma once


#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <GL/GL.h>
#include <cstdint>
#include <memory>


class shader_t {
public:
    shader_t(uint32_t handle) : handle_(handle) {}
    ~shader_t() { glDeleteShader(handle_); }

    shader_t(const shader_t &other) = delete;
    shader_t &operator=(const shader_t &other) = delete;

    shader_t(shader_t &&other) noexcept { 
        handle_ = 0;
        std::swap(other.handle_, handle_);
    }

    shader_t& operator=(shader_t&& other) noexcept {
        glDeleteShader(handle_);
        handle_ = 0;

        std::swap(handle_, other.handle_);

        return *this;
    }

    uint32_t get() const { return handle_; }

private:
    uint32_t handle_;
};


using window_t = std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)>;


struct glfw_exception : std::exception {
    glfw_exception(const char* message): message_(message) {}

    const char *what() const noexcept { return message_; }

private:
    const char *message_;
};


struct glfw_t {
    glfw_t() {
        if (!glfwInit())
            throw glfw_exception("Failed to initialize GLFW");
    }

    ~glfw_t() {
        glfwTerminate();
    }
};


class buffer_t {
public:
    buffer_t(uint32_t handle) : handle_(handle) {}
    ~buffer_t() {
        glDeleteBuffers(1, &handle_);
    }

    uint32_t get() const { return handle_; }

private:
    uint32_t handle_;
};


class vertex_array_t {
public:
    vertex_array_t(uint32_t handle) : handle_(handle) {}
    ~vertex_array_t() {
        glDeleteVertexArrays(1, &handle_);
    }

    uint32_t get() const { return handle_; }

private:
    uint32_t handle_;
};


class program_t {
public:
    explicit program_t(uint32_t handle) : handle_(handle) {}
    ~program_t() {
        glDeleteProgram(handle_);
    }

    program_t(const program_t &other) = delete;
    program_t operator=(const program_t &other) = delete;

    program_t(program_t &&other) noexcept { 
        handle_ = 0;
        std::swap(handle_, other.handle_);
    }
    
    program_t operator=(program_t&& other) noexcept {
        glDeleteProgram(handle_);
        handle_ = 0;
        std::swap(handle_, other.handle_);
    }
    
    uint32_t get() const { return handle_; }

private:
    uint32_t handle_;
};

class texture_t {
public:
    explicit texture_t(uint32_t handle) : handle_(handle) {}
    ~texture_t() {
        glDeleteTextures(1, &handle_);
    }

    texture_t(const program_t &other) = delete;
    texture_t operator=(const program_t &other) = delete;

    texture_t(texture_t &&other) noexcept { 
        handle_ = 0;
        std::swap(handle_, other.handle_);
    }

    texture_t operator=(texture_t&& other) noexcept {
        glDeleteTextures(1, &handle_);
        handle_ = 0;
        std::swap(handle_, other.handle_);
    }

    uint32_t get() const { return handle_; }

private:
    uint32_t handle_;
};

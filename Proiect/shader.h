#pragma once


#include "wrappers.h"
#include <string>


struct gl_uniform_not_found_exception : public std::exception {
    gl_uniform_not_found_exception(std::string uniform_name)
        : uniform_name_(std::move(uniform_name)), message_("Uniform \"" + uniform_name_ + "\" was not found in the program") {}
    
    const char *what() const noexcept override { return message_.c_str(); }

    const std::string uniform_name_;

private:
    std::string message_;
};


struct invalid_shader_exception : public std::exception {
    invalid_shader_exception(uint32_t type, std::string message)
        : type(type), message_(std::move(message)) {}

    const char *what() const noexcept override { return message_.c_str(); }

    const uint32_t type;

private:
    std::string message_;
};


program_t load_program(const std::string &vertex_path, const std::string &fragment_path);

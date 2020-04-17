#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <stdexcept>


struct asset_error : public std::exception {
    asset_error(const std::string& path) : message_("Failed to laod asset: " + path) {}

    const char *what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};


std::tuple<std::vector<unsigned short>,
           std::vector<glm::vec3>,
           std::vector<glm::vec2>,
           std::vector<glm::vec3>> load_asset(const char *path);
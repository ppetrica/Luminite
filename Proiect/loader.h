#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <stdexcept>


namespace loader {

struct asset_error : public std::exception {
    asset_error(const std::string& path) : message_("Failed to laod asset: " + path) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uvs;
};

std::pair<std::vector<vertex>, std::vector<unsigned short>> load_asset(const char* path);

}
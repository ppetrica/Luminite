#pragma once


#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <GL/glew.h>


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

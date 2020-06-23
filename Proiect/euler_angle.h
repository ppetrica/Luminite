#pragma once

#include <glm/glm.hpp>


struct euler_angle {
	float pitch = 0;
	float yaw = 0;
	float roll = 0;

	euler_angle(float pitch, float yaw, float roll) : pitch(pitch), yaw(yaw), roll(roll) {}

	glm::vec3 to_vector() const;
	void normalize();
	void add(float pitch, float yaw, float roll);
};

inline glm::vec3 euler_angle::to_vector() const {
	glm::vec3 result;

	result.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	result.y = sin(glm::radians(pitch));
	result.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	return result;
}

inline void euler_angle::add(float pitch, float yaw, float roll) {
	this->pitch += pitch;
	this->yaw += yaw;
	this->roll += roll;
}

inline void euler_angle::normalize() {
	if (pitch > 89) {
		pitch = 89;
	} else if (pitch < -89) {
		pitch = -89;
	}

	while (yaw > 180) {
		yaw -= 360;
	}
	while (yaw < -180) {
		yaw += 360;
	}
}

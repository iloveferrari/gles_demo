#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <gles_include.h>

class Camera
{
public:
	Camera();
	~Camera();
	void update(ESContext *esContext, float detlaTime);
	void lookAt(ESContext *esContext, glm::vec3 eye, glm::vec3 center, glm::vec3 up);
	void normalizeAngles();
private:
	glm::mat4 m_cameraMatrix;
	float m_sensitivity;
	float m_horizontalAngle;
	float m_verticalAngle;
	float m_baseHorizontalAngle;
	float m_baseVerticalAngle;

	float view_zenith_angle_radians_;
	float view_azimuth_angle_radians_;

	float previous_mouse_x_;
	float previous_mouse_y_;

	glm::vec3 m_position;
	float m_moveSpeed;
	float m_fieldOfView;
};

#endif // CAMERA_H
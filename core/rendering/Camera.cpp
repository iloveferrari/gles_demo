#include "Camera.h"
#include <Input.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

static const float MaxVerticalAngle = 85.0f; //must be less than 90 to avoid gimbal lock

Camera::Camera()
{
	m_sensitivity = 0.1f;
	m_horizontalAngle = 0.0f;
	m_verticalAngle = 0.0f;
	m_moveSpeed = 2.0f;
	m_fieldOfView = 60.0f;
	m_baseHorizontalAngle = 0.0f;
	m_baseVerticalAngle = 0.0f;
}

Camera::~Camera()
{

}

void Camera::update(ESContext *esContext, float detlaTime)
{
	if (Input::getInstance()->getKeyState(RIGHT_CLICK))
	{
		int detalX = Input::getInstance()->getAxisX();
		int detalY = Input::getInstance()->getAxisY();

		m_verticalAngle += detalY * m_sensitivity;
		m_horizontalAngle += detalX * m_sensitivity;

		normalizeAngles();

		glm::mat4 orientation;
		orientation = glm::rotate(orientation, m_verticalAngle + m_baseVerticalAngle,   glm::vec3(1, 0, 0));
		orientation = glm::rotate(orientation, m_horizontalAngle + m_baseHorizontalAngle, glm::vec3(0, 1, 0));

		m_cameraMatrix = orientation * glm::translate(glm::mat4(), -m_position);
		esContext->mvp_matrix = esContext->perspective_matrix * m_cameraMatrix;
		esContext->camera_matrix = m_cameraMatrix;
	}

	DIRECTION dir = Input::getInstance()->getMoveDirection();

	if (dir != NOINPUT)
	{
		glm::mat4 orientation;
		orientation = glm::rotate(orientation, m_verticalAngle + m_baseVerticalAngle, glm::vec3(1, 0, 0));
		orientation = glm::rotate(orientation, m_horizontalAngle + m_baseHorizontalAngle, glm::vec3(0, 1, 0));
		glm::mat4 inverse_orientation = glm::inverse(orientation);

		float speed = m_moveSpeed;

		if (Input::getInstance()->getKeyState(ACCELERATE_CLICK))
		{
			speed = 10 * m_moveSpeed;
		}

		if (dir == FORWARD)
		{
			m_position += detlaTime * speed * glm::vec3(inverse_orientation * glm::vec4(0, 0, -1, 1));
		}
		else if (dir == BACK)
		{
			m_position += detlaTime * speed * glm::vec3(inverse_orientation * glm::vec4(0, 0, 1, 1));
		}
		else if (dir == LEFT)
		{
			m_position += detlaTime * speed * glm::vec3(inverse_orientation * glm::vec4(-1, 0, 0, 1));
		}
		else if (dir == RIGHT)
		{
			m_position += detlaTime * speed * glm::vec3(inverse_orientation * glm::vec4(1, 0, 0, 1));
		}
		else if (dir == UP)
		{
			m_position += detlaTime * speed * glm::vec3(inverse_orientation * glm::vec4(0, 1, 0, 1));
		}
		else if (dir == DOWN)
		{
			m_position += detlaTime * speed * glm::vec3(inverse_orientation * glm::vec4(0, -1, 0, 1));
		}
		
		m_cameraMatrix = orientation * glm::translate(glm::mat4(), -m_position);
		esContext->mvp_matrix = esContext->perspective_matrix * m_cameraMatrix;
		esContext->camera_matrix = m_cameraMatrix;
	}

	float scroll_dis = Input::getInstance()->getMouseWheelScroll();
	if (scroll_dis != 0)
	{
		m_fieldOfView += scroll_dis / 60;

		if (m_fieldOfView < 5.0f)
		{
			m_fieldOfView = 5.0f;
		}

		if (m_fieldOfView > 130.0f)
		{
			m_fieldOfView = 130.0f;
		}

		float    aspect;
		aspect = (GLfloat)esContext->width / (GLfloat)esContext->height;
		esContext->perspective_matrix = glm::perspective(m_fieldOfView, aspect, 0.01f, 100.0f);

		glm::mat4 orientation;
		orientation = glm::rotate(orientation, m_verticalAngle + m_baseVerticalAngle, glm::vec3(1, 0, 0));
		orientation = glm::rotate(orientation, m_horizontalAngle + m_baseHorizontalAngle, glm::vec3(0, 1, 0));

		m_cameraMatrix = orientation * glm::translate(glm::mat4(), -m_position);
		esContext->mvp_matrix = esContext->perspective_matrix * m_cameraMatrix;
		esContext->camera_matrix = m_cameraMatrix;
	}
}

void Camera::normalizeAngles() {
	m_horizontalAngle = fmodf(m_horizontalAngle, 360.0f);
	//fmodf can return negative values, but this will make them all positive
	if (m_horizontalAngle < 0.0f)
	{
		m_horizontalAngle += 360.0f;
	}

	if (m_verticalAngle > MaxVerticalAngle)
	{
		m_verticalAngle = MaxVerticalAngle;
	}
	else if (m_verticalAngle < -MaxVerticalAngle)
	{
		m_verticalAngle = -MaxVerticalAngle;
	}
}

void Camera::lookAt(ESContext *esContext, glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
	float    aspect;

	aspect = (GLfloat)esContext->width / (GLfloat)esContext->height;
	esContext->perspective_matrix = glm::perspective(m_fieldOfView, aspect, 0.01f, 1000.0f);
	m_position = eye;

	m_verticalAngle = 0;
	m_horizontalAngle = 0;

	normalizeAngles();

	m_cameraMatrix = glm::lookAt(eye, center, up);

	glm::vec3 z = glm::normalize(center - eye);

	glm::mat4 orientation;

	glm::vec3 originZAxis(0, 0, 1);
	glm::vec3 targetZAxis = glm::normalize(glm::vec3(m_cameraMatrix[0][2], 0, m_cameraMatrix[2][2]));
	float yAxisRotateRadian = acos(originZAxis.x * targetZAxis.x + originZAxis.y * targetZAxis.y + originZAxis.z * targetZAxis.z);
	m_baseHorizontalAngle = yAxisRotateRadian / 3.1415926 * 180;

	glm::vec3 rotateAxis = glm::normalize(glm::cross(originZAxis, targetZAxis));
	if (rotateAxis == glm::vec3(0, 1, 0))
	{
		m_baseHorizontalAngle = -m_baseHorizontalAngle;
	}

	orientation = glm::rotate(orientation, m_baseHorizontalAngle, glm::vec3(0, 1, 0));

	originZAxis = glm::normalize(glm::vec3(orientation[0][2], orientation[1][2], orientation[2][2]));
	targetZAxis = glm::normalize(glm::vec3(m_cameraMatrix[0][2], m_cameraMatrix[1][2], m_cameraMatrix[2][2]));

	float xAxisRotateRadian = acos(originZAxis.x * targetZAxis.x + originZAxis.y * targetZAxis.y + originZAxis.z * targetZAxis.z);
	m_baseVerticalAngle = xAxisRotateRadian / 3.1415926 * 180;

	rotateAxis = glm::normalize(glm::cross(originZAxis, targetZAxis));

	orientation = glm::rotate(orientation, m_baseVerticalAngle, rotateAxis);
	glm::vec3 resultZAxis = glm::vec3(orientation[0][2], orientation[1][2], orientation[2][2]);
	if (resultZAxis.x - targetZAxis.x > 0.0001 || resultZAxis.y - targetZAxis.y > 0.0001 || resultZAxis.z - targetZAxis.z > 0.0001)
	{
		m_baseVerticalAngle = -m_baseVerticalAngle;
	}

	esContext->mvp_matrix = esContext->perspective_matrix * m_cameraMatrix;
	esContext->camera_matrix = m_cameraMatrix;

	glViewport(0, 0, esContext->width, esContext->height);
}
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <gles_include.h>
#include <glm/glm.hpp>

class Triangle
{
public: 
	Triangle();
	~Triangle();

	GLboolean init();
	void draw(ESContext *esContext);
private:
	GLint m_program;
	GLint m_mvpLoc;
	glm::mat4 m_modelMatrix;
};

#endif TRIANGLE_H
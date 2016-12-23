#ifndef CUBE_H
#define CUBE_H

#include <gles_include.h>
#include <glm/glm.hpp>

class Cube
{
public:
	Cube();
	~Cube();

	GLboolean init();
	void draw(ESContext *esContext);
	int genCube(float scale, GLfloat **vertices, GLfloat **normals, GLfloat **texCoords, GLuint **indices);
private:
	GLuint m_program;
	GLuint m_texture;
	GLuint m_indicesIBO;
	GLuint m_positionVBO;
	GLuint m_normalsVBO;
	GLuint m_texCoordsVBO;
	int m_numIndices;
	GLint m_mvpLoc;
	GLint m_textureLoc;
	glm::mat4 m_modelMatrix;
};

#endif CUBE_H
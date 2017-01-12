#ifndef __SKY__
#define __SKY__


#include <gles_include.h>

class Sky
{
public:
	Sky();
	~Sky();

	bool init();
	void draw(ESContext *esContext);
	int genSkyModelInfo(float radius, GLfloat **vertices, GLfloat **texCood, GLint **indices);

private:
	float m_radius;
	int m_numIndices;

	GLint m_textureLoc;
	GLint m_mvpLoc;

	float m_theta;

	GLuint m_indicesVBO;
	GLuint m_verticesVBO;
	GLuint m_normalsVBO;
	GLuint m_texCoordsVBO;

	GLuint m_textureId;
	GLuint m_program;
};

#endif
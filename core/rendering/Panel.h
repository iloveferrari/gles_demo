#ifndef __PANEL__
#define __PANEL__

#include <gles_include.h>

class Panel
{
public:
	Panel();
	~Panel();

	bool init();
	void draw(ESContext *esContext);

	int genPanelModelInfo(GLfloat **vertices, GLuint **indices);

private:
	int m_width;
	int m_height;

	int m_numIndices;

	GLint m_mvpLoc;
	GLint m_colorLoc;

	GLuint m_indicesVBO;
	GLuint m_verticesVBO;

	GLuint m_program;
	
	glm::mat4 m_modelMatrix;
};


#endif
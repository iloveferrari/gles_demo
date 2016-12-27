#ifndef __LABEl_H__
#define __LABEl_H__

#include <string>
#include <gles_include.h>

class Label
{
public:
	Label();
	~Label();

	bool init();
	bool initWithString(const char *text, const char *fontName, float fontSize, int width = 0, int height = 0);
	void draw(ESContext *esContext);

private:
	GLuint m_textureId;
	GLuint m_program;
	GLint  m_textureLoc;
	GLint  m_mvpLoc;

	GLuint m_indicesVBO;
	GLuint m_positionVBO;
	GLuint m_texCoordsVBO;

	int m_width;
	int m_height;

	float m_vertexX;
	float m_vertexY;
};

#endif
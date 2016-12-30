#ifndef __LABEl_H__
#define __LABEl_H__

#include <string>
#include <gles_include.h>
#include <rendering/types.h>
#include <math/glm/glm.hpp>

class Label
{
public:
	Label();
	~Label();

	bool init();
	bool initWithString(const char *text, const char *fontName, float fontSize, int width = 0, int height = 0);
	void draw(ESContext *esContext);

	void setString(const char *text);
	void setColor(Color3B color);
	void setPosition(float x, float y);

private:
	GLuint m_textureId;

	GLuint m_program;

	GLint  m_textureLoc;
	GLint  m_mvpLoc;
	GLint  m_colorLoc;
	GLint  m_transformLoc;

	GLuint m_indicesVBO;
	GLuint m_positionVBO;
	GLuint m_texCoordsVBO;

	float m_vertexPos[12];

	glm::mat4 m_transform;

	Color3B m_color;

	int m_width;
	int m_height;

	float m_vertexX;
	float m_vertexY;

	float m_positionX;
	float m_positionY;

	bool m_isDirty;

	std::string m_text;

	FontDefinition m_textDefinition;
};

#endif
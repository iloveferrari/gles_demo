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
	bool initWithString(const char *text, const char *fontName, float fontSize);
	void draw(ESContext *esContext);

private:
	GLuint m_textureId;
	GLuint m_program;
	GLint  m_textureLoc;
	GLint  m_mvpLoc;
};

#endif
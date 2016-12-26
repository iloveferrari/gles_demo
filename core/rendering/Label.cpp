#include "Label.h"
#include <rendering/Texture.h>

Label::Label()
	:m_program(0)
{

}

Label::~Label()
{

}

bool Label::init()
{
	const char vShaderStr[] =
		"#version 300 es										  \n"
		"uniform mat4 u_mvpMatrix;								  \n"
		"layout(location = 0) in vec4 a_position;				  \n"
		"layout(location = 1) in vec2 texCoord;					  \n"
		"out vec2 v_texCoord;									  \n"
		"void main()											  \n"
		"{														  \n"
		"    v_texCoord = texCoord;                               \n"
		"    gl_Position = u_mvpMatrix * a_position;              \n"
		"}";


	const char fShaderStr[] =
		"#version 300 es									 \n"
		"precision mediump float;							 \n"
		"in vec2 v_texCoord;								 \n"
		"out vec4 o_fragColor;								 \n"
		"uniform sampler2D s_texture;                        \n"
		"void main()										 \n"
		"{													 \n"
		"    o_fragColor = texture( s_texture, v_texCoord ); \n"
		//"    o_fragColor = vec4(v_texCoord, 0, 1 ); \n"
		"}";                                                 

	// Create the program object
	m_program = esLoadProgram(vShaderStr, fShaderStr);
	m_textureLoc = glGetUniformLocation(m_program, "s_texture");
	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");

	return true;
}

bool Label::initWithString(const char *text, const char *fontName, float fontSize)
{
	init();

	FontDefinition textDefinition;
	textDefinition._fontName = fontName;
	textDefinition._fontSize = fontSize;
	textDefinition._dimensions.width = 100;
	textDefinition._dimensions.height = 50;
	textDefinition._alignment = TextHAlignment::CENTER;
	textDefinition._vertAlignment = TextVAlignment::CENTER;
	textDefinition._fontFillColor = Color3B(255,255,255);

	Texture *texture = new Texture();
	texture->initWithString(text, textDefinition);

	m_textureId = texture->getTextureId();

	return true;
}

void Label::draw(ESContext *esContext)
{
	GLfloat color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	// 3 vertices, with (x,y,z) per-vertex
	GLfloat vertexPos[3 * 4] =
	{
		-0.5f, -0.5f, 1.1f, // v0
		0.5f, -0.5f, 1.1f, // v1
		0.5f, 0.5f, 1.1f,  // v2
		-0.5f, 0.5f, 1.1f  // v3
	};

	GLfloat cubeTex[] =
	{
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	glUseProgram(m_program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_DST_ALPHA);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureId);

	// Set the texture sampler to texture unit to 0
	glUniform1i(m_textureLoc, 0);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertexPos);
	glEnableVertexAttribArray(0);
	//glVertexAttrib4fv(1, color);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, cubeTex);
	glEnableVertexAttribArray(1);
	// Load the MVP matrix
	glm::mat4 mvp = esContext->mvp_matrix;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &mvp[0][0]);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
}
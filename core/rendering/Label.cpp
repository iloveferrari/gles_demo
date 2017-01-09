#include "Label.h"
#include <rendering/Texture.h>
#include <math/glm/gtc/matrix_transform.hpp>

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

Label::Label()
	:m_program(0)
	, m_width(0)
	, m_height(0)
	, m_vertexX(1.0f)
	, m_vertexY(1.0f)
	, m_textureId(0)
	, m_color(Color3B(1.0f, 1.0f, 1.0f))
	, m_colorLoc(0)
	, m_indicesVBO(0)
	, m_positionVBO(0)
	, m_texCoordsVBO(0)
	, m_positionX(0)
	, m_positionY(0)
	, m_isDirty(false)
{

}

Label::~Label()
{
	if (m_textureId)
	{
		glDeleteBuffers(1, &m_textureId);
	}

	if (m_indicesVBO)
	{
		glDeleteBuffers(1, &m_indicesVBO);
	}

	if (m_positionVBO)
	{
		glDeleteBuffers(1, &m_positionVBO);
	}

	if (m_texCoordsVBO)
	{
		glDeleteBuffers(1, &m_texCoordsVBO);
	}
}

bool Label::init()
{
	const char vShaderStr[] =
		"#version 300 es										  \n"
		"uniform mat4 u_transform;								  \n"
		"layout(location = 0) in vec4 a_position;				  \n"
		"layout(location = 1) in vec2 texCoord;					  \n"
		"out vec2 v_texCoord;									  \n"
		"void main()											  \n"
		"{														  \n"
		"    v_texCoord = texCoord;                               \n"
		"    gl_Position = u_transform * a_position;              \n"
		"}";


	const char fShaderStr[] =
		"#version 300 es													  \n"
		"precision mediump float;											  \n"
		"in vec2 v_texCoord;												  \n"
		"out vec4 o_fragColor;												  \n"
		"uniform vec3 color;												  \n"
		"uniform sampler2D s_texture;										  \n"
		"void main()														  \n"
		"{													                  \n"
		"    o_fragColor = vec4(color, texture(s_texture, v_texCoord).a);     \n"
		"}";                                                 

	// Create the program object
	m_program = esLoadProgram(vShaderStr, fShaderStr);
	m_textureLoc = glGetUniformLocation(m_program, "s_texture");
	m_transformLoc = glGetUniformLocation(m_program, "u_transform");
	m_colorLoc = glGetUniformLocation(m_program, "color");

	return true;
}

bool Label::initWithString(const char *text, const char *fontName, float fontSize, int width, int height)
{
	init();

	m_textDefinition._fontName = fontName;
	m_textDefinition._fontSize = fontSize;
	m_textDefinition._dimensions.width = width;
	m_textDefinition._dimensions.height = height;
	m_textDefinition._alignment = TextHAlignment::CENTER;
	m_textDefinition._vertAlignment = TextVAlignment::CENTER;
	m_textDefinition._fontFillColor = Color3B(1.0f, 1.0f, 1.0f);

	m_width = width;
	m_height = height;

	setString(text);

	m_vertexX = m_width * 1.0f / g_winWidth;
	m_vertexY = m_height * 1.0f / g_winHeight;

	// ³õÊ¼Î»ÖÃÒÆµ½ÆÁÄ»×óÏÂ½Ç
	float offsetX = m_width / g_winWidth + 1.0f;
	float offsetY = m_height / g_winHeight + 1.0f;

	GLfloat vertexPos[] =
	{
		-m_vertexX - offsetX,  m_vertexY - offsetY, 0,      // v0
		-m_vertexX - offsetX, -m_vertexY - offsetY, 0,      // v1
		 m_vertexX - offsetX, -m_vertexY - offsetY, 0,      // v2
		 m_vertexX - offsetX,  m_vertexY - offsetY, 0,      // v3
	};

	GLfloat cubeTex[] =
	{
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	GLuint indices[4] = { 0, 1, 2, 3 };

	glGenBuffers(1, &m_indicesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_positionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof (GLfloat)* 4, vertexPos, GL_STATIC_DRAW);

	glGenBuffers(1, &m_texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof (GLfloat)* 2, cubeTex, GL_STATIC_DRAW);

	return true;
}

void Label::setPosition(float x, float y)
{
	m_positionX = x;
	m_positionY = y;

	float offsetX = x * 2 / g_winWidth;
	float offsetY = y * 2 / g_winHeight;

	glm::mat4 originMat;
	m_transform = glm::translate(originMat, glm::vec3(offsetX, offsetY, 0));
}

void Label::setString(const char *text)
{
	if (m_text.compare(text))
	{
		m_text = text;
		m_isDirty = true;
	}
}

void Label::setColor(Color3B color)
{
	m_color = color;
}

void Label::draw(ESContext *esContext)
{
	glUseProgram(m_program);

	if (m_isDirty)
	{
		m_isDirty = false;
		Texture *texture = new Texture();
		texture->initWithString(m_text.c_str(), m_textDefinition);

		if (m_textureId)
		{
			glDeleteTextures(1, &m_textureId);
			m_textureId = 0;
		}

		m_textureId = texture->getTextureId();
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);

	// Load the vertex position
	glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
	glVertexAttribPointer(POSITION_LOC, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof (GLfloat), (const void *)NULL);
	glEnableVertexAttribArray(POSITION_LOC);

	// Load the texcoord position
	glBindBuffer(GL_ARRAY_BUFFER, m_texCoordsVBO);
	glVertexAttribPointer(TEXCOORD_LOC, 2, GL_FLOAT,
		GL_FALSE, 2 * sizeof (GLfloat), (const void *)NULL);
	glEnableVertexAttribArray(TEXCOORD_LOC);

	// Bind the index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureId);

	// Set the texture sampler to texture unit to 0
	glUniform1i(m_textureLoc, 0);

	glUniform3f(m_colorLoc, m_color.r, m_color.g, m_color.b);

	glUniformMatrix4fv(m_transformLoc, 1, GL_FALSE, &m_transform[0][0]);

	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glDisableVertexAttribArray(TEXCOORD_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}
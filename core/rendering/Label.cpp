#include "Label.h"
#include <rendering/Texture.h>

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

Label::Label()
	:m_program(0)
	, m_width(0)
	, m_height(0)
	, m_vertexX(1.0f)
	, m_vertexY(1.0f)
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
		"    gl_Position = a_position;                            \n"
		"}";


	const char fShaderStr[] =
		"#version 300 es									 \n"
		"precision mediump float;							 \n"
		"in vec2 v_texCoord;								 \n"
		"out vec4 o_fragColor;								 \n"
		"uniform sampler2D s_texture;                        \n"
		"void main()										 \n"
		"{													 \n"
		"    o_fragColor = vec4(texture( s_texture, v_texCoord ).rgb, 0.0f); \n"
		//"    o_fragColor = vec4(v_texCoord, 0, 1 ); \n"
		"}";                                                 

	// Create the program object
	m_program = esLoadProgram(vShaderStr, fShaderStr);
	m_textureLoc = glGetUniformLocation(m_program, "s_texture");
	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");

	return true;
}

bool Label::initWithString(const char *text, const char *fontName, float fontSize, int width, int height)
{
	init();

	FontDefinition textDefinition;
	textDefinition._fontName = fontName;
	textDefinition._fontSize = fontSize;
	textDefinition._dimensions.width = width;
	textDefinition._dimensions.height = height;
	textDefinition._alignment = TextHAlignment::CENTER;
	textDefinition._vertAlignment = TextVAlignment::CENTER;
	textDefinition._fontFillColor = Color3B(255,255,255);

	m_width = width;
	m_height = height;

	Texture *texture = new Texture();
	texture->initWithString(text, textDefinition);

	m_textureId = texture->getTextureId();

	m_vertexX = m_width * 1.0f / 640;
	m_vertexY = m_height * 1.0f / 480;

	GLfloat vertexPos[3 * 4] =
	{
		 -m_vertexX,  m_vertexY, 0,      // v0
		 -m_vertexX, -m_vertexY, 0,      // v1
		  m_vertexX, -m_vertexY, 0,      // v2
		  m_vertexX,  m_vertexY, 0,      // v3
	};

	GLfloat cubeTex[] =
	{
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	GLuint indices[4] = { 0, 1, 2, 3 };

	// Index buffer for base terrain
	glGenBuffers(1, &m_indicesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Position VBO for base terrain
	glGenBuffers(1, &m_positionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof (GLfloat)* 3, vertexPos, GL_STATIC_DRAW);

	// texCoord VBO for base terrain
	glGenBuffers(1, &m_texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof (GLfloat)* 2, cubeTex, GL_STATIC_DRAW);

	return true;
}

void Label::draw(ESContext *esContext)
{
	glUseProgram(m_program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

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

	// Load the MVP matrix
	glm::mat4 mvp = esContext->perspective_matrix * esContext->camera_matrix;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &mvp[0][0]);

	// Draw the cube
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glDisableVertexAttribArray(TEXCOORD_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}
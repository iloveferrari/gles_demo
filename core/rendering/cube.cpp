#include "cube.h"

#include <glm/gtx/transform.hpp>

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

Cube::Cube()
{

}

Cube::~Cube()
{

}

GLboolean Cube::init()
{
	const char vShaderStr[] =
		"#version 300 es										  \n"
		"uniform mat4 u_mvpMatrix;								  \n"
		"layout(location = 0) in vec4 a_position;				  \n"
		"layout(location = 1) in vec2 a_texCoord;                 \n"
		"out vec2 v_texCoord;						     		  \n"
		"void main()											  \n"
		"{														  \n"
		"    v_texCoord = a_texCoord;                             \n"
		"    gl_Position = u_mvpMatrix * a_position;              \n"
		"}";


	const char fShaderStr[] =
		"#version 300 es                                     \n"
		"precision mediump float;                            \n"
		"in vec2 v_texCoord;                                 \n"
		"out vec4 outColor;                                  \n"
		"uniform sampler2D s_texture;                        \n"
		"void main()                                         \n"
		"{                                                   \n"
		"  outColor = texture( s_texture, v_texCoord );      \n"
		"}                                                   \n";

	// Create the program object
	m_program = esLoadProgram(vShaderStr, fShaderStr);

	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");

	m_textureLoc = glGetUniformLocation(m_program, "s_texture");

	int width, height;

	m_texture = loadTexture("checker.png", &width, &height);

	if (m_program == 0)
	{
		return GL_FALSE;
	}

	GLfloat *vertices, *normals, *texCoords;
	GLuint *indices;

	m_numIndices = genCube(1.0f, &vertices, &normals, &texCoords, &indices);

	// Index buffer for base terrain
	glGenBuffers(1, &m_indicesIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	free(indices);

	// Position VBO for base terrain
	glGenBuffers(1, &m_positionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof (GLfloat)* 3, vertices, GL_STATIC_DRAW);
	free(vertices);

	// normal VBO for base terrain
	glGenBuffers(1, &m_normalsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_normalsVBO);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof (GLfloat)* 3, normals, GL_STATIC_DRAW);
	free(normals);

	// texCoord VBO for base terrain
	glGenBuffers(1, &m_texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof (GLfloat)* 2, texCoords, GL_STATIC_DRAW);
	free(texCoords);

	m_modelMatrix = glm::translate(glm::vec3(60, 80, 80));

	return GL_TRUE;
}

void Cube::draw(ESContext *esContext)
{
	glUseProgram(m_program);

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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesIBO);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	// Set the texture sampler to texture unit to 0
	glUniform1i(m_textureLoc, 0);

	// Load the MVP matrix
	glm::mat4 mvp = esContext->mvp_matrix * m_modelMatrix;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &mvp[0][0]);

	// Draw the cube
	glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glDisableVertexAttribArray(TEXCOORD_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

int Cube::genCube(float scale, GLfloat **vertices, GLfloat **normals, GLfloat **texCoords, GLuint **indices)
{
	int i;
	int numVertices = 24;
	int numIndices = 36;

	GLfloat cubeVerts[] =
	{
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, -0.5f,
		-0.5f, 0.5f, -0.5f,
		-0.5f, 0.5f, 0.5f,
		0.5f, 0.5f, 0.5f,
		0.5f, 0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, 0.5f, -0.5f,
		0.5f, 0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,
		0.5f, 0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,
		-0.5f, 0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, 0.5f,
		0.5f, 0.5f, 0.5f,
		0.5f, 0.5f, -0.5f,
	};

	GLfloat cubeNormals[] =
	{
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
	};

	GLfloat cubeTex[] =
	{
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	// Allocate memory for buffers
	if (vertices != NULL)
	{
		*vertices = (GLfloat *)malloc(sizeof (GLfloat)* 3 * numVertices);
		memcpy(*vertices, cubeVerts, sizeof (cubeVerts));

		for (i = 0; i < numVertices * 3; i++)
		{
			(*vertices)[i] *= scale;
		}
	}

	if (normals != NULL)
	{
		*normals = (GLfloat *)malloc(sizeof (GLfloat)* 3 * numVertices);
		memcpy(*normals, cubeNormals, sizeof (cubeNormals));
	}

	if (texCoords != NULL)
	{
		*texCoords = (GLfloat *)malloc(sizeof (GLfloat)* 2 * numVertices);
		memcpy(*texCoords, cubeTex, sizeof (cubeTex));
	}


	// Generate the indices
	if (indices != NULL)
	{
		GLuint cubeIndices[] =
		{
			0, 2, 1,
			0, 3, 2,
			4, 5, 6,
			4, 6, 7,
			8, 9, 10,
			8, 10, 11,
			12, 15, 14,
			12, 14, 13,
			16, 17, 18,
			16, 18, 19,
			20, 23, 22,
			20, 22, 21
		};

		*indices = (GLuint *)malloc(sizeof (GLuint)* numIndices);
		memcpy(*indices, cubeIndices, sizeof (cubeIndices));
	}

	return numIndices;
}
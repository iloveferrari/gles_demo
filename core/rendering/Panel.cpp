#include "Panel.h"

#include <glm/gtx/transform.hpp>

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

Panel::Panel()
{

}

Panel::~Panel()
{

}

bool Panel::init()
{
	const char vShaderStr[] =
		"#version 300 es                                      \n"
		"uniform mat4 u_mvpMatrix;                            \n"
		"uniform vec3 u_color;                                \n"
		"layout(location = 0) in vec4 a_position;             \n"
		"out vec4 v_color;                                    \n"
		"void main()                                          \n"
		"{                                                    \n"
		"   v_color = vec4(u_color, 1);                       \n"
		"   gl_Position = u_mvpMatrix * a_position;           \n"
		"}                                                    \n";

	const char fShaderStr[] =
		"#version 300 es                                        \n"
		"precision mediump float;                               \n"
		"in vec4 v_color;                                       \n"
		"layout(location = 0) out vec4 outColor;                \n"
		"void main()                                            \n"
		"{                                                      \n"
		"  outColor = v_color;                                  \n"
		"}                                                      \n";

	m_program = esLoadProgram(vShaderStr, fShaderStr);

	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");
	m_colorLoc = glGetUniformLocation(m_program, "u_color");

	m_width = 20560;
	m_height = 20560;

	//m_numIndices = genPanelModelInfo(&vertices, &indices);

	GLfloat vertices[4 * 3] = 
	{ 
		-m_width, 0,  m_height,
		-m_width, 0, -m_height,
		 m_width, 0, -m_height,
		 m_width, 0,  m_height
	};

	GLuint indices[4] = { 0, 1, 2, 3 };

	glGenBuffers(1, &m_indicesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_verticesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_verticesVBO);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 4, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//m_modelMatrix = glm::translate(glm::vec3(-300, 10, -300));

	return true;
}

void Panel::draw(ESContext *esContext)
{
	glUseProgram(m_program);

	glDisable(GL_DEPTH_TEST);

	// Load the vertex position
	glBindBuffer(GL_ARRAY_BUFFER, m_verticesVBO);
	glVertexAttribPointer(POSITION_LOC, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof (GLfloat), (const void *)NULL);
	glEnableVertexAttribArray(POSITION_LOC);

	// Bind the index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);

	// Load the MVP matrix
	glm::mat4 mvp = esContext->mvp_matrix ;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &mvp[0][0]);

	glUniform3f(m_colorLoc, 0.9f, 0.9f, 0.9f);

	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glEnable(GL_DEPTH_TEST);
}

int Panel::genPanelModelInfo(GLfloat **vertices, GLuint **indices)
{
	int numIndices = (m_width - 1) * (m_height - 1) * 2 * 3;
	int numVertices = m_width * m_height;

	// Allocate memory for buffers
	if (vertices != nullptr)
	{
		*vertices = (GLfloat *)malloc(sizeof (GLfloat)* 3 * numVertices);

		for (int i = 0; i < m_height; ++i) // row
		{
			for (int j = 0; j < m_width; ++j) // column
			{
				(*vertices)[3 * (j + i * m_width)] = i;
				(*vertices)[3 * (j + i * m_width) + 1] = 0;
				(*vertices)[3 * (j + i * m_width) + 2] = j;
			}
		}
	}

	// Generate the indices
	if (indices != nullptr)
	{
		*indices = (GLuint *)malloc(sizeof (GLuint)* numIndices);

		for (int i = 0; i < m_height - 1; ++i)
		{
			for (int j = 0; j < m_width - 1; ++j)
			{
				// two triangles per quad
				(*indices)[6 * (j + i * (m_width - 1))] = j + (i)* (m_width);
				(*indices)[6 * (j + i * (m_width - 1)) + 1] = j + (i)* (m_width)+1;
				(*indices)[6 * (j + i * (m_width - 1)) + 2] = j + (i + 1) * (m_width);

				(*indices)[6 * (j + i * (m_width - 1)) + 3] = j + (i + 1)* (m_width);
				(*indices)[6 * (j + i * (m_width - 1)) + 4] = j + (i)* (m_width)+1;
				(*indices)[6 * (j + i * (m_width - 1)) + 5] = j + (i + 1) * (m_width)+1;
			}
		}
	}

	return numIndices;
}
#include "Sky.h"

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

Sky::Sky()
{
	m_theta = 15.0f;
}

Sky::~Sky()
{

}

bool Sky::init()
{
	GLfloat *vertices;
	GLfloat *texCoord;
	GLint   *indices;
	float radius = 102400.0f;

	const char vShaderStr[] =
		"#version 300 es                                      \n"
		"uniform mat4 u_mvpMatrix;                            \n"
		"layout(location = 0) in vec4 a_position;             \n"
		"layout(location = 1) in vec2 a_texCoord;             \n"
		"out vec2 v_texCoord;                                 \n"
		"void main()                                          \n"
		"{                                                    \n"
		"                                                     \n"
		"   v_texCoord = a_texCoord;                          \n"
		"   gl_Position = u_mvpMatrix * a_position;           \n"
		"}                                                    \n";

	const char fShaderStr[] =
		"#version 300 es                                        \n"
		"precision mediump float;                               \n"
		"in vec2 v_texCoord;                                    \n"
		"layout(location = 0) out vec4 outColor;                \n"
		"uniform sampler2D s_texture;                           \n"
		"void main()                                            \n"
		"{                                                      \n"
		"  outColor = texture(s_texture, v_texCoord);           \n"
		"}                                                      \n";

	m_program = esLoadProgram(vShaderStr, fShaderStr);

	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");
	m_textureLoc = glGetUniformLocation(m_program, "s_texture");

	int width, heiht;
	m_textureId = loadTexture("sky.png", &width, &heiht);

	m_numIndices = genSkyModelInfo(radius, &vertices, &texCoord, &indices);

	glGenBuffers(1, &m_indicesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// texCoord VBO for base terrain
	glGenBuffers(1, &m_texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, (360 / m_theta + 1) * (90 / m_theta + 1) * sizeof (GLfloat)* 2, texCoord, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_verticesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_verticesVBO);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * (360 / m_theta + 1) * (90 / m_theta + 1), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete texCoord;
	delete indices;
	delete vertices;

	return true;
}

void Sky::draw(ESContext *esContext)
{
	glUseProgram(m_program);

	glDisable(GL_DEPTH_TEST);

	// Load the vertex position
	glBindBuffer(GL_ARRAY_BUFFER, m_verticesVBO);
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

	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &esContext->mvp_matrix[0][0]);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureId);

	// Set the texture sampler to texture unit to 0
	glUniform1i(m_textureLoc, 0);

	glDrawElements(GL_TRIANGLE_STRIP, m_numIndices, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glDisableVertexAttribArray(TEXCOORD_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glEnable(GL_DEPTH_TEST);
}

int Sky::genSkyModelInfo(float radius, GLfloat **vertices, GLfloat **texCood, GLint **indices)
{
	const float DTOR = 3.1415926535897f / 180.0f;
	int width = 360 / m_theta + 1;
	int height = 90 / m_theta + 1;
	int numVertices = width * height;// 计算顶点数量
	int numIndices = (width - 1) * (height - 1) * 2 * 3;

	if (vertices)
	{
		(*vertices) = new GLfloat[3 * numVertices];
		(*texCood)  = new GLfloat[2 * numVertices];

		int n = 0;
		for (int phi = 90; phi >= 0; phi -= m_theta)// 从顶部开始循环到底部，即循环纬度从90度-0度
		{
			for (int theta = 0; theta <= 360; theta += m_theta)//循环经度从0度-360度
			{
				(*vertices)[3 * n]     = radius * cosf(phi * DTOR) * cosf(theta * DTOR) + 200;
				(*vertices)[3 * n + 1] = radius * sinf(phi * DTOR);
				(*vertices)[3 * n + 2] = radius * cosf(phi * DTOR) * sinf(theta * DTOR) + 200;

				(*texCood)[2 * n]     = 0.5f + 0.5f * cosf(phi * DTOR) * cosf(theta * DTOR);
				(*texCood)[2 * n + 1] = 0.5f + 0.5f * cosf(phi * DTOR) * sinf(theta * DTOR);

				printf("x%f, y%f\n", (*texCood)[2 * n], (*texCood)[2 * n + 1]);

				n++;
			}
		}

		//printf("%d\n", n);
	}
	
	if (indices)
	{
		(*indices) = new GLint[numIndices];

		for (int i = 0; i < height - 1; ++i)
		{
			for (int j = 0; j < width - 1; ++j)
			{
				// two triangles per quad
				(*indices)[6 * (j + i * (width - 1))] = j + (i)* (width);
				(*indices)[6 * (j + i * (width - 1)) + 1] = j + (i)* (width)+1;
				(*indices)[6 * (j + i * (width - 1)) + 2] = j + (i + 1) * (width);

				(*indices)[6 * (j + i * (width - 1)) + 3] = j + (i + 1)* (width);
				(*indices)[6 * (j + i * (width - 1)) + 4] = j + (i) * (width)+1;
				(*indices)[6 * (j + i * (width - 1)) + 5] = j + (i + 1) * (width) + 1;
			}
		}
	}

	return numIndices;
}
#include "Terrain.h"
#include <fstream>
#include <iostream>

using namespace std;

#define POSITION_LOC    0
#define TEXCOORD_LOC    1
#define NORMAL_LOC      2

Terrain::Terrain()
{
	m_step = 2.0f;
	m_minZ = -100.0f;
	m_scale = 0.54f;

	m_indicesVBO = 0;
	m_positionVBO = 0;
}

Terrain::~Terrain()
{

}

void Terrain::init()
{
	GLfloat *positions;
	GLuint *indices;
	GLfloat *texCoords;
	GLfloat *normals;

	const char vShaderStr[] =
		"#version 300 es                                      \n"
		"uniform mat4 u_mvpMatrix;                            \n"
		"uniform vec3 u_lightDirection;                       \n"
		"layout(location = 0) in vec4 a_position;             \n"
		"layout(location = 1) in vec2 a_texCoord;             \n"
		"layout(location = 2) in vec3 a_normal;               \n"
		"out float diffuse;                                   \n"
		"out vec2 v_texCoord;                                 \n"
		"void main()                                          \n"
		"{                                                    \n"
		"                                                     \n"
		"   // compute diffuse lighting                       \n"
		"   diffuse = dot(a_normal, u_lightDirection);        \n"
		"   v_texCoord = a_texCoord;                          \n"
		"   gl_Position = u_mvpMatrix * a_position;           \n"
		"}                                                    \n";

	const char fShaderStr[] =
		"#version 300 es                                        \n"
		"precision mediump float;                               \n"
		"in vec2 v_texCoord;                                    \n"
		"in float diffuse;                                      \n"
		"layout(location = 0) out vec4 outColor;                \n"
		"uniform sampler2D s_texture;                           \n"
		"void main()                                            \n"
		"{                                                      \n"
		"  outColor = texture(s_texture, v_texCoord) * diffuse; \n"
		"}                                                      \n";

	m_program = esLoadProgram(vShaderStr, fShaderStr);

	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");
	m_textureLoc = glGetUniformLocation(m_program, "s_texture");
	m_lightLoc = glGetUniformLocation(m_program, "u_lightDirection");

	unsigned char *buffer = loadBMP("ground.bmp", &m_width, &m_height);
	m_numIndices = genSquareGrid(m_width, &positions, &texCoords, &normals, &indices, buffer);

	int texWidth, texHeight;
	m_textureId = loadTexture("Grass2.png", &texWidth, &texHeight);

	glGenBuffers(1, &m_indicesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// normal VBO for base terrain
	glGenBuffers(1, &m_normalsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_normalsVBO);
	glBufferData(GL_ARRAY_BUFFER, m_width * m_height * sizeof (GLfloat)* 3, normals, GL_STATIC_DRAW);
	free(normals);

	// texCoord VBO for base terrain
	glGenBuffers(1, &m_texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, m_width * m_height * sizeof (GLfloat)* 2, texCoords, GL_STATIC_DRAW);
	free(texCoords);

	glGenBuffers(1, &m_positionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * m_width * m_height, positions, GL_STATIC_DRAW);

	free(indices);
	free(positions);
	delete buffer;
}

void Terrain::draw(ESContext *esContext)
{
	glUseProgram(m_program);

	glEnable(GL_CULL_FACE);

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

	// Load the texcoord position
	glBindBuffer(GL_ARRAY_BUFFER, m_normalsVBO);
	glVertexAttribPointer(NORMAL_LOC, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof (GLfloat), (const void *)NULL);
	glEnableVertexAttribArray(NORMAL_LOC);

	// Bind the index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);

	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &esContext->mvp_matrix[0][0]);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_textureId);

	// Set the texture sampler to texture unit to 0
	glUniform1i(m_textureLoc, 0);

	glUniform3f(m_lightLoc, 0.86f, 0.64f, 0.49f);

	glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glDisableVertexAttribArray(TEXCOORD_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisable(GL_CULL_FACE);
}

int Terrain::genSquareGrid(int size, GLfloat **vertices, GLfloat **texCoord, GLfloat **normals, GLuint **indices, unsigned char *buffer)
{
	int i, j;
	int numIndices = (size - 1) * (size - 1) * 2 * 3;
	int numVertices = size * size;

	// Allocate memory for buffers
	if (vertices != nullptr)
	{
		float stepSize = (float)size - 1;
		*vertices = (GLfloat *)malloc(sizeof (GLfloat)* 3 * numVertices);
		*texCoord = (GLfloat *)malloc(sizeof (GLfloat)* 2 * numVertices);

		for (i = 0; i < size; ++i) // row
		{
			for (j = 0; j < size; ++j) // column
			{
				(*vertices)[3 * (j + i * size)] = i * m_step;
				(*vertices)[3 * (j + i * size) + 1] = m_minZ + m_scale * buffer[j + i * size];
				(*vertices)[3 * (j + i * size) + 2] = j * m_step;

				(*texCoord)[2 * (j + i * size)] = 11.0f / m_width * j;
				(*texCoord)[2 * (j + i * size) + 1] = 11.0f / m_height * i;

				//printf("%d  %f, %d  %f\n", 2 * (j + i * size), 110.0f / m_width * j, 2 * (j + i * size) + 1, 110.0f / m_height * i);
			}
		}
	}

	if (normals != nullptr)
	{
		*normals = (GLfloat *)malloc(sizeof(GLfloat) * 3 * numVertices);

		float xPixelSize = 1 / g_winWidth;
		float yPixelSize = 1 / g_winHeight;
		for (i = 0; i < size; ++i) // row
		{
			for (j = 0; j < size; ++j) // column
			{
				/* update the normal */
				float height = (*vertices)[3 * (j + i * size) + 1];
				glm::vec3 dx = glm::vec3(1, (*vertices)[3 * (j + (i + 1) * size) + 1] - height, 0.0);
				glm::vec3 dy = glm::vec3(0.0, (*vertices)[3 * (j + 1 + i * size) + 1] - height, 1);

				glm::vec3 result = glm::normalize(glm::cross(dy, dx));
				(*normals)[3 * (j + i * size)] = result.x;
				(*normals)[3 * (j + i * size) + 1] = result.y;
				(*normals)[3 * (j + i * size) + 2] = result.z;
			}
		}
	}

	// Generate the indices
	if (indices != nullptr)
	{
		*indices = (GLuint *)malloc(sizeof (GLuint)* numIndices);

		for (i = 0; i < size - 1; ++i)
		{
			for (j = 0; j < size - 1; ++j)
			{
				// two triangles per quad
				(*indices)[6 * (j + i * (size - 1))] = j + (i)* (size);
				(*indices)[6 * (j + i * (size - 1)) + 1] = j + (i)* (size)+1;
				(*indices)[6 * (j + i * (size - 1)) + 2] = j + (i + 1) * (size)+1;

				(*indices)[6 * (j + i * (size - 1)) + 3] = j + (i)* (size);
				(*indices)[6 * (j + i * (size - 1)) + 4] = j + (i + 1) * (size)+1;
				(*indices)[6 * (j + i * (size - 1)) + 5] = j + (i + 1) * (size);
			}
		}
	}

	return numIndices;
}

unsigned char *Terrain::loadBMP(const char *filename, int *width, int *height)
{
	ifstream file(filename, ios::binary | ios::in);
	if (!file)
	{
		cout << "无法打开 " << filename << endl;
		return false;
	}
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	file.read((char*)&bmfh, sizeof(bmfh));
	file.read((char*)&bmih, sizeof(bmih));
	if (bmih.biBitCount != 8)
	{
		cout << filename << "不是8位灰度图 " << bmih.biBitCount << endl;
		return false;
	}
	file.seekg(bmfh.bfOffBits);
	int size = bmih.biHeight * bmih.biWidth;
	unsigned char *buffer = new unsigned char[size];
	file.read((char *)buffer, size);
	file.close();

	m_width = bmih.biWidth;
	m_height = bmih.biHeight;

	return buffer;
}
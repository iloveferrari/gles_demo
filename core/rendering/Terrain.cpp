#include "Terrain.h"
#include <fstream>
#include <iostream>

using namespace std;

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

Terrain::Terrain()
{
	m_step = 2.0f;
	m_minZ = -80.0f;
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

	const char vShaderStr[] =
		"#version 300 es                                      \n"
		"uniform mat4 u_mvpMatrix;                            \n"
		"uniform vec3 u_lightDirection;                       \n"
		"uniform vec3 u_normal;                               \n"
		"layout(location = 0) in vec4 a_position;             \n"
		"out vec4 v_color;                                    \n"
		"void main()                                          \n"
		"{                                                    \n"
		"                                                     \n"
		"   // compute diffuse lighting                       \n"
		//"   float diffuse = dot( normal, u_lightDirection );  \n"
		"   v_color = vec4( 0.5, 0.5, 0.5, 1.0 );             \n"
		"   gl_Position = u_mvpMatrix * a_position;           \n"
		"}                                                    \n";

	const char fShaderStr[] =
		"#version 300 es                                      \n"
		"precision mediump float;                             \n"
		"in vec4 v_color;                                     \n"
		"layout(location = 0) out vec4 outColor;              \n"
		"void main()                                          \n"
		"{                                                    \n"
		"  outColor = v_color;                                \n"
		"}                                                    \n";

	m_program = esLoadProgram(vShaderStr, fShaderStr);

	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");

	unsigned char *buffer = loadBMP("ground.bmp", &m_width, &m_height);
	m_numIndices = genSquareGrid(m_width, &positions, nullptr, &indices, buffer);

	glGenBuffers(1, &m_indicesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof (GLuint), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

	// Load the vertex position
	glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
	glVertexAttribPointer(POSITION_LOC, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof (GLfloat), (const void *)NULL);
	glEnableVertexAttribArray(POSITION_LOC);

	// Bind the index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVBO);

	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &esContext->mvp_matrix[0][0]);

	glDrawElements(GL_LINES, m_numIndices, GL_UNSIGNED_INT, (const void *)NULL);

	glDisableVertexAttribArray(POSITION_LOC);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

int Terrain::genSquareGrid(int size, GLfloat **vertices, GLfloat **normals, GLuint **indices, unsigned char *buffer)
{
	int i, j;
	int numIndices = (size - 1) * (size - 1) * 2 * 3;

	// Allocate memory for buffers
	if (vertices != NULL)
	{
		int numVertices = size * size;
		float stepSize = (float)size - 1;
		*vertices = (GLfloat *)malloc(sizeof (GLfloat)* 3 * numVertices);

		for (i = 0; i < size; ++i) // row
		{
			for (j = 0; j < size; ++j) // column
			{
				(*vertices)[3 * (j + i * size)] = i * m_step;
				(*vertices)[3 * (j + i * size) + 1] = m_minZ + m_scale * buffer[j + i * size];
				(*vertices)[3 * (j + i * size) + 2] = j * m_step;
			}
		}
	}

	// Generate the indices
	if (indices != NULL)
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
#ifndef TERRAIN_H
#define TERRAIN_H

#include <gles_include.h>

class Terrain
{
public:
	Terrain();
	~Terrain();

	void init();
	int genSquareGrid(int size, GLfloat **vertices, GLfloat **texCoord, GLfloat **normals, GLuint **indices, unsigned char *buffer);
	unsigned char *loadBMP(const char *filename, int *width, int *height);
	void draw(ESContext *esContext);
private:
	int m_width;
	int m_height;


	GLuint m_program;

	GLuint m_textureId;
	
	GLint  m_mvpLoc;
	GLint m_textureLoc;
	GLuint m_indicesVBO;
	GLuint m_positionVBO;
	GLuint m_normalsVBO;
	GLuint m_texCoordsVBO;

	int m_numIndices;
	float m_step;
	float m_minZ;
	float m_scale;
};

#endif TERRAIN_H
#ifndef TERRAIN_H
#define TERRAIN_H

#include <gles_include.h>

class Terrain
{
public:
	Terrain();
	~Terrain();

	void init();
	int genSquareGrid(int size, GLfloat **vertices, GLfloat **normals, GLuint **indices, unsigned char *buffer);
	unsigned char *loadBMP(const char *filename, int *width, int *height);
	void draw(ESContext *esContext);
private:
	int m_width;
	int m_height;


	GLuint m_program;
	
	GLint  m_mvpLoc;
	GLuint m_indicesVBO;
	GLuint m_positionVBO;

	int m_numIndices;
	float m_step;
	float m_minZ;
	float m_scale;
};

#endif TERRAIN_H
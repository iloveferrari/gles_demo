#include "triangle.h"

#include <glm/gtx/transform.hpp>

Triangle::Triangle()
{

}

Triangle::~Triangle()
{

}

GLboolean Triangle::init()
{
	const char vShaderStr[] =
		R"( #version 300 es
			struct material_properties 
			{ 
				vec4 ambient_color; 
				vec4 diffuse_color; 
				vec4 specular_color; 
				float specular_exponent;
			};

			uniform material_properties aa;
										 
			uniform mat4 u_mvpMatrix;								
			layout(location = 0) in vec4 a_position;				
			layout(location = 1) in vec4 a_color;				
			out vec4 v_color;										
			void main()										
			{													
				v_color = a_color;                             
				gl_Position = u_mvpMatrix * a_position;      
			})";


	const char fShaderStr[] =
		"#version 300 es            \n"
		"precision mediump float;   \n"
		"in vec4 v_color;           \n"
		"out vec4 o_fragColor;      \n"
		"void main()                \n"
		"{                          \n"
		"    o_fragColor = v_color; \n"
		"}";

	// Create the program object
	m_program = esLoadProgram(vShaderStr, fShaderStr);

	m_mvpLoc = glGetUniformLocation(m_program, "u_mvpMatrix");

	if (m_program == 0)
	{
		return GL_FALSE;
	}

	m_modelMatrix = glm::translate(glm::vec3(60, 80, 80));

	return GL_TRUE;
}

void Triangle::draw(ESContext *esContext)
{
	GLfloat color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	// 3 vertices, with (x,y,z) per-vertex
	GLfloat vertexPos[3 * 3] =
	{
		0.0f,   0.5f, 1.0f, // v0
		-0.5f, -0.5f, 1.0f, // v1
		0.5f, -0.5f, 1.0f  // v2
	};

	glUseProgram(m_program);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertexPos);
	glEnableVertexAttribArray(0);
	glVertexAttrib4fv(1, color);

	// Load the MVP matrix
	glm::mat4 mvp = esContext->mvp_matrix * m_modelMatrix;
	glUniformMatrix4fv(m_mvpLoc, 1, GL_FALSE, &mvp[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);

	CHECK_GL_ERROR_DEBUG();
}